#include <cassert>
#include <iostream>
#include <cmath>
#include <memory>
#include <vector>

#include "core/DataPacket.h"
#include "world/WorldAnalysisNode.h"
#include "world/WorldSynthesisNode.h"

using namespace vv;

int main() {
    // Create a simple sine wave as input
    const uint32_t sr = 48000;
    const float freq = 220.0f;
    const float sec = 0.25f;
    const size_t N = static_cast<size_t>(sec * sr);
    std::vector<float> x; x.reserve(N);
    float phase = 0.0f;
    for (size_t n = 0; n < N; ++n) {
        phase += 2.0f * 3.1415926535f * freq / static_cast<float>(sr);
        x.push_back(std::sin(phase) * 0.25f);
    }
    auto in = std::make_shared<DataPacket>(x, sr, 1, 32);

    WorldParams params; params.sampleRate = sr; params.frameSize = 1024; params.hopSize = 256;
    WorldAnalysisNode analysis(params);
    analysis.setId("world_analysis");
    assert(analysis.initialize());
    auto analyzed = analysis.process(in);
    assert(analyzed);
    assert(analyzed->hasFeature("f0"));
    assert(analyzed->hasFeature("spectral_envelope"));
    assert(analyzed->hasFeature("aperiodicity"));

    WorldSynthesisNode synth;
    synth.setId("world_synth");
    assert(synth.initialize());
    // For this placeholder pipeline, synthesis returns passthrough; ensure we at least get some samples
    auto out = synth.process(analyzed);
    assert(out);
    assert(out->samples().size() > 0);

    std::cout << "world tests passed\n";
    return 0;
}
