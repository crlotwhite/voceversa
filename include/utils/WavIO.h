#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vv::wavio {

struct WavData {
    uint32_t sampleRate{48000};
    uint16_t channels{1};
    std::vector<float> samples; // interleaved if channels>1, normalized [-1,1]
};

// Minimal PCM16/Float32 WAV reader/writer
bool readWav(const std::string& path, WavData& out);
bool writeWav16(const std::string& path, const WavData& in, float peak = 0.99f);

}
