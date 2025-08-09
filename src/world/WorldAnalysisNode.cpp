#include "world/WorldAnalysisNode.h"
#include "world/AnalysisUtils.h"
#include <algorithm>
#include <cmath>

namespace vv {

std::shared_ptr<DataPacket> WorldAnalysisNode::process(const std::shared_ptr<const DataPacket>& input) {
    if (!input) return nullptr;
    auto out = std::make_shared<DataPacket>(*input);
    const auto& x = input->samples();
    auto f0 = estimateF0(x, input->sampleRate(), params_.hopSize);
    auto env = simpleSpectralEnvelope(x, params_.frameSize, params_.hopSize);
    auto ap = simpleAperiodicity(x, params_.frameSize, params_.hopSize);
    out->setFeature("f0", std::move(f0));
    out->setFeature("spectral_envelope", std::move(env));
    out->setFeature("aperiodicity", std::move(ap));
    return out;
}

} // namespace vv
