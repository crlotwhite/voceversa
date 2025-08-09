#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <future>
#include <thread>

#include "core/ISynthesisNode.h"

namespace vv {

class ComputationGraph {
public:
    using NodePtr = std::shared_ptr<ISynthesisNode>;

    bool addNode(const NodePtr& node) {
        if (!node || nodes_.count(node->id())) return false;
        nodes_[node->id()] = node;
        adj_[node->id()] = {};
        return true;
    }

    bool removeNode(const std::string& id) {
        if (!nodes_.count(id)) return false;
        nodes_.erase(id);
        adj_.erase(id);
        for (auto& [k, nbrs] : adj_) {
            nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), id), nbrs.end());
        }
        return true;
    }

    bool connectNodes(const std::string& from, const std::string& to) {
        if (!nodes_.count(from) || !nodes_.count(to)) return false;
        // prevent duplicates
        auto& v = adj_[from];
        if (std::find(v.begin(), v.end(), to) == v.end()) v.push_back(to);
        return true;
    }

    // Kahn's algorithm for topological sort. Returns empty on cycle.
    std::vector<std::string> topologicalOrder() const {
        // Build indegree
        std::unordered_map<std::string, int> indeg;
        for (const auto& [id, _] : nodes_) indeg[id] = 0;
        for (const auto& [u, nbrs] : adj_) for (const auto& v : nbrs) ++indeg[v];

        std::vector<std::string> q; q.reserve(indeg.size());
        for (const auto& [id, d] : indeg) if (d == 0) q.push_back(id);
        std::vector<std::string> order; order.reserve(indeg.size());
        size_t head = 0;
        auto adjCopy = adj_;
        while (head < q.size()) {
            auto u = q[head++];
            order.push_back(u);
            for (const auto& v : adjCopy[u]) {
                if (--indeg[v] == 0) q.push_back(v);
            }
        }
        if (order.size() != nodes_.size()) return {}; // cycle
        return order;
    }

    const std::unordered_map<std::string, NodePtr>& nodes() const { return nodes_; }

    // Minimal parallel execute: runs initialize() then process() for each node when deps are ready.
    // Data routing: if multiple predecessors, takes the first predecessor's output.
    // Returns true on full execution, false if cycle or failure.
    bool execute(const std::shared_ptr<const DataPacket>& input,
                 std::unordered_map<std::string, std::shared_ptr<DataPacket>>& outputs,
                 unsigned maxThreads = std::thread::hardware_concurrency()) const
    {
        auto order = topologicalOrder();
        if (order.empty() && !nodes_.empty()) return false; // cycle
        // Build predecessors map
        std::unordered_map<std::string, std::vector<std::string>> pred;
        for (const auto& [u, nbrs] : adj_) for (const auto& v : nbrs) pred[v].push_back(u);

        // Thread-limited async execution using a simple semaphore via futures queue
        std::vector<std::future<void>> inflight;
        if (maxThreads == 0) maxThreads = 1;
        std::atomic<bool> ok{true};
        outputs.clear();
        for (const auto& id : order) {
            if (!ok.load()) break;
            // Wait if too many inflight
            while (inflight.size() >= maxThreads) {
                inflight.front().get();
                inflight.erase(inflight.begin());
            }
            // capture by value needed items
            auto nodeIt = nodes_.find(id);
            if (nodeIt == nodes_.end()) { ok = false; break; }
            auto node = nodeIt->second;
            inflight.emplace_back(std::async(std::launch::async, [&, node, id]() {
                if (!node->initialize()) { ok = false; return; }
                std::shared_ptr<const DataPacket> in;
                auto pit = pred.find(id);
                if (pit == pred.end() || pit->second.empty()) in = input; // source nodes
                else {
                    // pick first predecessor's output if present
                    auto& plist = pit->second;
                    for (const auto& p : plist) {
                        auto oit = outputs.find(p);
                        if (oit != outputs.end()) { in = oit->second; break; }
                    }
                    if (!in) in = input; // fallback
                }
                auto out = node->process(in);
                if (!out) { ok = false; return; }
                outputs[id] = std::move(out);
            }));
        }
        // drain remaining
        for (auto& f : inflight) f.get();
        if (!ok.load()) return false;
        return outputs.size() == nodes_.size();
    }

private:
    std::unordered_map<std::string, NodePtr> nodes_{};
    std::unordered_map<std::string, std::vector<std::string>> adj_{}; // from -> [to]
};

} // namespace vv
