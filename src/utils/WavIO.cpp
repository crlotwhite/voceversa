#include "utils/WavIO.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdint>

namespace vv::wavio {

#pragma pack(push,1)
struct RiffHeader { char riff[4]; uint32_t size; char wave[4]; };
struct ChunkHeader { char id[4]; uint32_t size; };
struct FmtPCM { uint16_t format; uint16_t channels; uint32_t sampleRate; uint32_t byteRate; uint16_t blockAlign; uint16_t bitsPerSample; };
#pragma pack(pop)

static bool readAll(std::ifstream& is, void* dst, size_t n) { is.read(reinterpret_cast<char*>(dst), n); return static_cast<bool>(is); }

bool readWav(const std::string& path, WavData& out) {
    std::ifstream is(path, std::ios::binary);
    if (!is) return false;
    RiffHeader rh{}; if (!readAll(is, &rh, sizeof(rh))) return false;
    if (std::strncmp(rh.riff, "RIFF", 4) || std::strncmp(rh.wave, "WAVE", 4)) return false;
    FmtPCM fmt{}; bool haveFmt=false; std::vector<uint8_t> data;
    while (is) {
        ChunkHeader ch{}; if (!readAll(is, &ch, sizeof(ch))) break;
        if (!std::strncmp(ch.id, "fmt ", 4)) {
            if (ch.size < sizeof(FmtPCM)) return false;
            if (!readAll(is, &fmt, sizeof(FmtPCM))) return false;
            if (ch.size > sizeof(FmtPCM)) is.seekg(ch.size - sizeof(FmtPCM), std::ios::cur);
            haveFmt = true;
        } else if (!std::strncmp(ch.id, "data", 4)) {
            data.resize(ch.size);
            if (!readAll(is, data.data(), ch.size)) return false;
        } else {
            is.seekg(ch.size, std::ios::cur);
        }
    }
    if (!haveFmt || data.empty()) return false;
    out.sampleRate = fmt.sampleRate; out.channels = fmt.channels;
    size_t frames = 0;
    if (fmt.format == 1 /*PCM*/ && fmt.bitsPerSample == 16) {
        frames = data.size() / (out.channels * 2);
        out.samples.resize(frames * out.channels);
        const int16_t* p = reinterpret_cast<const int16_t*>(data.data());
        for (size_t i=0;i<frames*out.channels;++i) out.samples[i] = std::clamp(p[i] / 32768.0f, -1.0f, 1.0f);
        return true;
    } else if (fmt.format == 3 /*IEEE float*/ && fmt.bitsPerSample == 32) {
        frames = data.size() / (out.channels * 4);
        out.samples.resize(frames * out.channels);
        const float* p = reinterpret_cast<const float*>(data.data());
        for (size_t i=0;i<frames*out.channels;++i) out.samples[i] = std::clamp(p[i], -1.0f, 1.0f);
        return true;
    }
    return false;
}

bool writeWav16(const std::string& path, const WavData& in, float peak) {
    std::ofstream os(path, std::ios::binary);
    if (!os) return false;
    uint32_t frames = static_cast<uint32_t>(in.samples.size() / in.channels);
    uint32_t dataBytes = frames * in.channels * 2;
    RiffHeader rh{{'R','I','F','F'}, 36 + dataBytes, {'W','A','V','E'}};
    os.write(reinterpret_cast<const char*>(&rh), sizeof(rh));
    ChunkHeader fmtH{{'f','m','t',' '}, sizeof(FmtPCM)}; os.write(reinterpret_cast<const char*>(&fmtH), sizeof(fmtH));
    FmtPCM fmt{1, in.channels, in.sampleRate, in.sampleRate*in.channels*2, static_cast<uint16_t>(in.channels*2), 16};
    os.write(reinterpret_cast<const char*>(&fmt), sizeof(fmt));
    ChunkHeader dataH{{'d','a','t','a'}, dataBytes}; os.write(reinterpret_cast<const char*>(&dataH), sizeof(dataH));
    float maxa = 1e-9f; for (float v: in.samples) maxa = std::max(maxa, std::abs(v));
    float scale = (maxa>0) ? (peak/maxa) : 1.0f;
    for (float v: in.samples) {
        int16_t s = static_cast<int16_t>(std::clamp(v*scale, -1.0f, 1.0f) * 32767.0f);
        os.write(reinterpret_cast<const char*>(&s), sizeof(s));
    }
    return static_cast<bool>(os);
}

} // namespace vv::wavio
