#include <cassert>
#include <iostream>
#include <cmath>
#include <memory>
#include <vector>

#include "core/DataPacket.h"
#include "world/WorldAnalysisNode.h"
#include "world/WorldSynthesisNode.h"
#include "core/DummyGainFilter.h"

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
    // First, ensure bypass yields some samples and doesn't alter when filter disabled
    auto out = synth.process(analyzed);
    assert(out);
    assert(out->samples().size() > 0);

    // Snapshot output for equality check under bypass
    auto baseline = *out;

    // Add a gain filter but keep post-filter disabled -> no change expected
    synth.addPostFilter(std::make_shared<DummyGainFilter>(0.5f));
    synth.enablePostFilter(false);
    auto out_bypass = synth.process(analyzed);
    assert(out_bypass);
    // Compare a few samples within a small epsilon
    size_t M = std::min<size_t>(baseline.samples().size(), out_bypass->samples().size());
    size_t checkN = std::min<size_t>(M, 256);
    for (size_t i = 0; i < checkN; ++i) {
        float diff = std::fabs(baseline.samples()[i] - out_bypass->samples()[i]);
        assert(diff < 1e-6f);
    }

    // Enable post-filter -> gain should change amplitude
    synth.enablePostFilter(true);
    auto out_gain = synth.process(analyzed);
    assert(out_gain);
    // RMS should drop roughly by gain (0.5)
    auto rms = [](const std::vector<float>& v){ double s=0; for(float x:v) s+=x*x; return std::sqrt(s/(v.empty()?1:v.size())); };
    double r0 = rms(baseline.samples());
    double r1 = rms(out_gain->samples());
    assert(r1 < r0 * 0.9); // allow tolerance; should be significantly lower

    std::cout << "world tests passed\n";
    return 0;
}
