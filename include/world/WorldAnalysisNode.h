#pragma once

#include "core/ISynthesisNode.h"
#include "core/DataPacket.h"
#include <memory>
#include <string>
#include <vector>

namespace vv {

struct WorldParams {
    uint32_t sampleRate{48000};
    size_t frameSize{1024};
    size_t hopSize{256};
};

// Analysis node: extracts F0, spectral envelope, aperiodicity
class WorldAnalysisNode : public ISynthesisNode {
public:
    explicit WorldAnalysisNode(WorldParams p = {}) : params_(p) { setName("WorldAnalysisNode"); }

    bool initialize() override { return true; }

    std::shared_ptr<DataPacket> process(const std::shared_ptr<const DataPacket>& input) override;

    std::vector<std::string> getInputs() const override { return {"wav"}; }
    std::vector<std::string> getOutputs() const override { return {"f0", "spectral_envelope", "aperiodicity"}; }

    void setParams(const WorldParams& p) { params_ = p; }
    const WorldParams& params() const { return params_; }

private:
    WorldParams params_{};
};

} // namespace vv
