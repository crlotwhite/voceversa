#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cassert>

namespace vv::audio {

// Interleaved multi-channel audio buffer (float by default)
template <typename T = float>
class AudioBuffer {
public:
    AudioBuffer() = default;
    AudioBuffer(uint16_t channels, size_t frames) { resize(channels, frames); }

    void resize(uint16_t channels, size_t frames) {
        channels_ = channels; frames_ = frames; data_.assign(static_cast<size_t>(channels)*frames, T{});
    }
    void clear() { std::fill(data_.begin(), data_.end(), T{}); }

    uint16_t channels() const { return channels_; }
    size_t frames() const { return frames_; }
    size_t samples() const { return data_.size(); }

    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }
    T& sample(size_t frame, uint16_t ch) { return data_[frame*channels_ + ch]; }
    const T& sample(size_t frame, uint16_t ch) const { return data_[frame*channels_ + ch]; }

    // Apply gain in-place
    void applyGain(T g) { for (auto& s : data_) s = static_cast<T>(s * g); }

    // Mix another buffer into this one with given gain (sizes must match)
    void mixFrom(const AudioBuffer& other, T g = static_cast<T>(1)) {
        assert(other.channels_ == channels_ && other.frames_ == frames_);
        for (size_t i=0;i<data_.size();++i) data_[i] = static_cast<T>(data_[i] + other.data_[i] * g);
    }

    // Convert to mono by averaging channels (returns new buffer)
    AudioBuffer toMono() const {
        if (channels_ == 1) return *this;
        AudioBuffer mono(1, frames_);
        for (size_t f=0; f<frames_; ++f) {
            double acc = 0.0; for (uint16_t c=0;c<channels_;++c) acc += sample(f,c);
            mono.sample(f,0) = static_cast<T>(acc / channels_);
        }
        return mono;
    }

    // Convert mono to stereo by duplicating (returns new buffer)
    AudioBuffer toStereo() const {
        if (channels_ == 2) return *this;
        assert(channels_ == 1);
        AudioBuffer st(2, frames_);
        for (size_t f=0; f<frames_; ++f) { T v = sample(f,0); st.sample(f,0) = v; st.sample(f,1) = v; }
        return st;
    }

private:
    uint16_t channels_{1};
    size_t frames_{0};
    std::vector<T> data_{}; // interleaved
};

} // namespace vv::audio
