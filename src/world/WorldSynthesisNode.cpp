#include "world/WorldSynthesisNode.h"
#include <algorithm>
#include <cmath>

namespace vv {

std::shared_ptr<DataPacket> WorldSynthesisNode::process(const std::shared_ptr<const DataPacket>& input) {
    if (!input) return nullptr;
    // Very naive synthesis: passthrough samples if available, else synthesize sin from f0
    if (!input->samples().empty()) {
        return std::make_shared<DataPacket>(*input);
    }
    auto out = std::make_shared<DataPacket>(std::vector<float>{}, input->sampleRate(), input->channels(), input->bitDepth());
    const auto* f0 = input->getFeature("f0");
    if (!f0 || f0->empty()) return out;
    // Synthesize 1 frame per f0 hop, 10ms per hop as placeholder
    const uint32_t sr = input->sampleRate();
    const size_t hopSamples = static_cast<size_t>(0.01 * sr);
    float phase = 0.0f;
    for (float f : *f0) {
        size_t N = hopSamples;
        for (size_t n = 0; n < N; ++n) {
            float omega = 2.0f * 3.1415926535f * (f > 0.0f ? f : 100.0f) / static_cast<float>(sr);
            phase += omega;
            out->samples().push_back(std::sin(phase) * 0.1f); // quiet tone
        }
    }
    return out;
}

} // namespace vv
