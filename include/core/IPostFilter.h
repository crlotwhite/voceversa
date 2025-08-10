#pragma once

#include <cstddef>
#include <cstdint>

namespace vv {

// Interface for optional post-processing filters applied after synthesis.
// Implementations should be lightweight and real-time friendly.
class IPostFilter {
public:
    virtual ~IPostFilter() = default;

    // Prepare internal state for given audio format. Called before process().
    virtual void prepare(uint32_t sampleRate, uint16_t channels) = 0;

    // Reset internal state (e.g., clear history buffers).
    virtual void reset() = 0;

    // In-place processing on interleaved buffer for the given number of frames.
    // Channels match the value passed to prepare().
    virtual void process(float* interleaved, size_t frames) = 0;
};

} // namespace vv
