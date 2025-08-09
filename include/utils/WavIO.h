#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstddef>

namespace vv::wavio {

enum class SampleFormat : uint16_t {
    PCM = 1,      // integer PCM (8/16/24/32)
    IEEE_FLOAT = 3
};

struct WavInfo {
    uint32_t sampleRate{48000};
    uint16_t channels{1};
    uint16_t bitsPerSample{16};
    SampleFormat format{SampleFormat::PCM};
    uint64_t dataBytes{0};
};

struct WavData {
    uint32_t sampleRate{48000};
    uint16_t channels{1};
    std::vector<float> samples; // interleaved if channels>1, normalized [-1,1]
};

// Reader with optional streaming (frame-chunked) API
class WavReader {
public:
    WavReader() = default;
    ~WavReader();
    // Open a file and parse headers. Returns false on failure.
    bool open(const std::string& path);
    // Read entire file into out (converts to float [-1,1]). Returns false on failure.
    bool readAll(WavData& out);
    // Streaming read: read up to maxFrames frames into dst (interleaved float). Returns frames read.
    size_t readFrames(float* dst, size_t maxFrames);
    // Seek to the start of data for another pass
    bool rewind();
    // Info accessors
    const WavInfo& info() const { return info_; }
    bool good() const { return good_; }

private:
    struct Impl;
    Impl* p_{nullptr};
    WavInfo info_{};
    bool good_{false};
};

// Writer supporting 16/24/32f output
class WavWriter {
public:
    WavWriter() = default;
    ~WavWriter();
    // Begin a new file. If bitsPerSample==32 and format==IEEE_FLOAT, writes float32. Otherwise integer PCM.
    bool open(const std::string& path, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample = 16, SampleFormat format = SampleFormat::PCM);
    // Append samples (interleaved float [-1,1])
    bool writeFrames(const float* src, size_t frames, float peakNormalize = 0.0f);
    // Finalize header sizes and close
    bool close();

private:
    struct Impl;
    Impl* p_{nullptr};
};

// Convenience helpers (backward compatible)
bool readWav(const std::string& path, WavData& out);
bool writeWav16(const std::string& path, const WavData& in, float peak = 0.99f);
bool writeWav32f(const std::string& path, const WavData& in);

}
