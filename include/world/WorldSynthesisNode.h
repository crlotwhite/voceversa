#pragma once

#include "core/ISynthesisNode.h"
#include "core/DataPacket.h"
#include "core/IPostFilter.h"
#include <memory>
#include <string>
#include <vector>

namespace vv {

// Synthesis node: reconstructs waveform from features
class WorldSynthesisNode : public ISynthesisNode {
public:
    WorldSynthesisNode() { setName("WorldSynthesisNode"); }

    bool initialize() override { return true; }

    std::shared_ptr<DataPacket> process(const std::shared_ptr<const DataPacket>& input) override;

    std::vector<std::string> getInputs() const override { return {"f0", "spectral_envelope", "aperiodicity"}; }
    std::vector<std::string> getOutputs() const override { return {"wav"}; }

    // Post-filter chain controls (optional; default disabled = bypass)
    void enablePostFilter(bool enabled) { postFilterEnabled_ = enabled; }
    bool isPostFilterEnabled() const { return postFilterEnabled_; }

    void clearPostFilters() { filters_.clear(); prepared_ = false; }
    void addPostFilter(std::shared_ptr<IPostFilter> filter) { if (filter) filters_.push_back(std::move(filter)); }

private:
    bool postFilterEnabled_{false};
    bool prepared_{false};
    uint32_t preparedSampleRate_{0};
    uint16_t preparedChannels_{0};
    std::vector<std::shared_ptr<IPostFilter>> filters_;
};

} // namespace vv
