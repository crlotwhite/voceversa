#pragma once

#include "core/ISynthesisNode.h"
#include "core/DataPacket.h"
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
};

} // namespace vv
