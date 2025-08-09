#pragma once

#include <memory>
#include <string>
#include <vector>

namespace vv {

class DataPacket; // forward decl

// Base abstract node for synthesis/processing graph
class ISynthesisNode {
public:
    virtual ~ISynthesisNode() = default;

    // Lifecycle
    virtual bool initialize() = 0;

    // Process single packet input -> output
    virtual std::shared_ptr<DataPacket> process(const std::shared_ptr<const DataPacket>& input) = 0;

    // Introspection
    virtual std::vector<std::string> getInputs() const = 0;
    virtual std::vector<std::string> getOutputs() const = 0;

    // Identity & state
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }

    void setId(std::string id) { id_ = std::move(id); }
    void setName(std::string name) { name_ = std::move(name); }

protected:
    std::string id_;
    std::string name_;
};

} // namespace vv
