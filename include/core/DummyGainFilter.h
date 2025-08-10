#pragma once

#include "core/IPostFilter.h"
#include <algorithm>

namespace vv {

class DummyGainFilter : public IPostFilter {
public:
    explicit DummyGainFilter(float gain = 1.0f) : gain_(gain) {}

    void setGain(float g) { gain_ = g; }
    float gain() const { return gain_; }

    void prepare(uint32_t /*sampleRate*/, uint16_t /*channels*/) override {}
    void reset() override {}
    void process(float* interleaved, size_t frames) override {
        if (!interleaved || frames == 0) return;
        for (size_t i = 0; i < frames; ++i) interleaved[i] *= gain_;
    }

private:
    float gain_;
};

} // namespace vv
