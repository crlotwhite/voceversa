#include "utils/WavIO.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <limits>
#include <cstddef>

namespace vv::wavio {

#pragma pack(push,1)
struct RiffHeader { char riff[4]; uint32_t size; char wave[4]; };
struct ChunkHeader { char id[4]; uint32_t size; };
struct FmtPCM { uint16_t format; uint16_t channels; uint32_t sampleRate; uint32_t byteRate; uint16_t blockAlign; uint16_t bitsPerSample; };
#pragma pack(pop)

static bool readBytes(std::istream& is, void* dst, size_t n) { is.read(reinterpret_cast<char*>(dst), n); return static_cast<bool>(is); }
static void writeBytes(std::ostream& os, const void* src, size_t n) { os.write(reinterpret_cast<const char*>(src), n); }

// ----------------- Helpers -----------------
static float i24_to_float(const uint8_t* p3) {
    int32_t v = (int32_t(p3[2]) << 24) | (int32_t(p3[1]) << 16) | (int32_t(p3[0]) << 8);
    v >>= 8; // sign extend
    return std::clamp(v / 8388608.0f, -1.0f, 1.0f);
}
static void float_to_i24(float x, uint8_t* out3) {
    float cl = std::clamp(x, -1.0f, 1.0f);
    int32_t v = static_cast<int32_t>(cl * 8388607.0f);
    out3[0] = uint8_t(v & 0xFF);
    out3[1] = uint8_t((v >> 8) & 0xFF);
    out3[2] = uint8_t((v >> 16) & 0xFF);
}

// ----------------- WavReader -----------------
struct WavReader::Impl {
    std::ifstream is;
    FmtPCM fmt{};
    std::streampos dataPos{0};
    uint32_t dataSize{0};
};

WavReader::~WavReader() { if (p_) { p_->is.close(); delete p_; p_ = nullptr; } }

bool WavReader::open(const std::string& path) {
    if (p_) { p_->is.close(); delete p_; p_ = nullptr; }
    p_ = new Impl();
    p_->is.open(path, std::ios::binary);
    if (!p_->is) return false;
    RiffHeader rh{}; if (!readBytes(p_->is, &rh, sizeof(rh))) return false;
    if (std::strncmp(rh.riff, "RIFF", 4) || std::strncmp(rh.wave, "WAVE", 4)) return false;
    bool haveFmt=false; bool haveData=false; ChunkHeader ch{};
    while (readBytes(p_->is, &ch, sizeof(ch))) {
        if (!std::strncmp(ch.id, "fmt ", 4)) {
            if (ch.size < sizeof(FmtPCM)) return false;
            if (!readBytes(p_->is, &p_->fmt, sizeof(FmtPCM))) return false;
            if (ch.size > sizeof(FmtPCM)) p_->is.seekg(ch.size - sizeof(FmtPCM), std::ios::cur);
            haveFmt = true;
        } else if (!std::strncmp(ch.id, "data", 4)) {
            p_->dataPos = p_->is.tellg();
            p_->dataSize = ch.size;
            p_->is.seekg(ch.size, std::ios::cur);
            haveData = true;
        } else {
            p_->is.seekg(ch.size, std::ios::cur);
        }
    }
    if (!haveFmt || !haveData) return false;
    info_.sampleRate = p_->fmt.sampleRate;
    info_.channels = p_->fmt.channels;
    info_.bitsPerSample = p_->fmt.bitsPerSample;
    info_.format = static_cast<SampleFormat>(p_->fmt.format);
    info_.dataBytes = p_->dataSize;
    good_ = true;
    return rewind();
}

bool WavReader::rewind() {
    if (!p_) return false;
    p_->is.clear();
    p_->is.seekg(p_->dataPos, std::ios::beg);
    return static_cast<bool>(p_->is);
}

size_t WavReader::readFrames(float* dst, size_t maxFrames) {
    if (!p_) return 0;
    const uint16_t ch = p_->fmt.channels;
    const uint16_t bps = p_->fmt.bitsPerSample;
    const uint16_t fmt = p_->fmt.format;
    size_t framesRead = 0;
    if (fmt == 1 && bps == 16) {
        const size_t want = maxFrames * ch;
        std::vector<int16_t> tmp(want);
        p_->is.read(reinterpret_cast<char*>(tmp.data()), tmp.size()*sizeof(int16_t));
        size_t got = p_->is.gcount() / sizeof(int16_t);
        for (size_t i=0;i<got;++i) dst[i] = std::clamp(tmp[i] / 32768.0f, -1.0f, 1.0f);
        framesRead = got / ch;
    } else if (fmt == 1 && bps == 24) {
        const size_t wantBytes = maxFrames * ch * 3;
        std::vector<uint8_t> tmp(wantBytes);
        p_->is.read(reinterpret_cast<char*>(tmp.data()), tmp.size());
        size_t gotBytes = p_->is.gcount();
        size_t got = gotBytes / 3;
        for (size_t i=0;i<got; ++i) dst[i] = i24_to_float(&tmp[i*3]);
        framesRead = got / ch;
    } else if (fmt == 3 && bps == 32) {
        const size_t want = maxFrames * ch;
        std::vector<float> tmp(want);
        p_->is.read(reinterpret_cast<char*>(tmp.data()), tmp.size()*sizeof(float));
        size_t got = p_->is.gcount() / sizeof(float);
        for (size_t i=0;i<got;++i) dst[i] = std::clamp(tmp[i], -1.0f, 1.0f);
        framesRead = got / ch;
    } else {
        return 0; // unsupported
    }
    return framesRead;
}

bool WavReader::readAll(WavData& out) {
    if (!p_) return false;
    const uint16_t ch = p_->fmt.channels;
    const uint16_t bps = p_->fmt.bitsPerSample;
    const uint16_t fmt = p_->fmt.format;
    out.sampleRate = p_->fmt.sampleRate; out.channels = ch;
    const size_t frames = p_->dataSize / (ch * (bps/8));
    out.samples.resize(static_cast<size_t>(frames) * ch);
    if (!rewind()) return false;
    if (fmt == 1 && bps == 16) {
        std::vector<int16_t> tmp(frames * ch);
    if (!readBytes(p_->is, tmp.data(), tmp.size()*sizeof(int16_t))) return false;
        for (size_t i=0;i<tmp.size();++i) out.samples[i] = std::clamp(tmp[i] / 32768.0f, -1.0f, 1.0f);
        return true;
    } else if (fmt == 1 && bps == 24) {
        std::vector<uint8_t> tmp(frames * ch * 3);
    if (!readBytes(p_->is, tmp.data(), tmp.size())) return false;
        for (size_t i=0;i<frames*ch;++i) out.samples[i] = i24_to_float(&tmp[i*3]);
        return true;
    } else if (fmt == 3 && bps == 32) {
        std::vector<float> tmp(frames * ch);
    if (!readBytes(p_->is, tmp.data(), tmp.size()*sizeof(float))) return false;
        for (size_t i=0;i<tmp.size();++i) out.samples[i] = std::clamp(tmp[i], -1.0f, 1.0f);
        return true;
    }
    return false;
}

// ----------------- WavWriter -----------------
struct WavWriter::Impl {
    std::ofstream os;
    FmtPCM fmt{};
    uint32_t dataBytes{0};
    std::streampos riffSizePos{0};
    std::streampos dataSizePos{0};
    bool opened{false};
};

WavWriter::~WavWriter() { if (p_) { close(); delete p_; p_ = nullptr; } }

bool WavWriter::open(const std::string& path, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample, SampleFormat format) {
    if (p_) { close(); delete p_; p_ = nullptr; }
    p_ = new Impl();
    p_->os.open(path, std::ios::binary);
    if (!p_->os) return false;
    const uint16_t fmtc = static_cast<uint16_t>(format);
    FmtPCM f{fmtc, channels, sampleRate, sampleRate*channels*(bitsPerSample/8), static_cast<uint16_t>(channels*(bitsPerSample/8)), bitsPerSample};
    p_->fmt = f;
    // RIFF header with placeholder sizes
    RiffHeader rh{{'R','I','F','F'}, 0, {'W','A','V','E'}};
    writeBytes(p_->os, &rh, sizeof(rh));
    p_->riffSizePos = p_->os.tellp(); p_->riffSizePos -= static_cast<std::streamoff>(sizeof(rh) - offsetof(RiffHeader, size));
    ChunkHeader fmtH{{'f','m','t',' '}, sizeof(FmtPCM)}; writeBytes(p_->os, &fmtH, sizeof(fmtH));
    writeBytes(p_->os, &f, sizeof(f));
    ChunkHeader dataH{{'d','a','t','a'}, 0};
    writeBytes(p_->os, &dataH, sizeof(dataH));
    p_->dataSizePos = p_->os.tellp(); p_->dataSizePos -= static_cast<std::streamoff>(sizeof(uint32_t));
    p_->opened = true;
    return true;
}

bool WavWriter::writeFrames(const float* src, size_t frames, float peakNormalize) {
    if (!p_ || !p_->opened) return false;
    const uint16_t ch = p_->fmt.channels;
    const uint16_t bps = p_->fmt.bitsPerSample;
    const uint16_t fmt = p_->fmt.format;
    size_t samples = frames * ch;
    if (fmt == 3 && bps == 32) {
        // write float32 directly (clamped)
        std::vector<float> tmp(samples);
        for (size_t i=0;i<samples;++i) tmp[i] = std::clamp(src[i], -1.0f, 1.0f);
    writeBytes(p_->os, tmp.data(), tmp.size()*sizeof(float));
        p_->dataBytes += static_cast<uint32_t>(tmp.size()*sizeof(float));
        return true;
    }
    // integer PCM path, optional normalization
    float scale = 1.0f;
    if (peakNormalize > 0.0f) {
        float maxa = 1e-9f; for (size_t i=0;i<samples;++i) maxa = std::max(maxa, std::abs(src[i]));
        if (maxa > 0.0f) scale = std::min(peakNormalize/maxa, 1.0f);
    }
    if (bps == 16) {
        for (size_t i=0;i<samples;++i) {
            int16_t s = static_cast<int16_t>(std::clamp(src[i]*scale, -1.0f, 1.0f) * 32767.0f);
            writeBytes(p_->os, &s, sizeof(s));
        }
        p_->dataBytes += static_cast<uint32_t>(samples * sizeof(int16_t));
        return true;
    } else if (bps == 24) {
        uint8_t bytes[3];
    for (size_t i=0;i<samples;++i) { float_to_i24(src[i]*scale, bytes); writeBytes(p_->os, bytes, 3); }
        p_->dataBytes += static_cast<uint32_t>(samples * 3);
        return true;
    } else if (bps == 32) {
        // integer 32-bit
        for (size_t i=0;i<samples;++i) {
            int32_t s = static_cast<int32_t>(std::clamp(src[i]*scale, -1.0f, 1.0f) * 2147483647.0f);
            writeBytes(p_->os, &s, sizeof(s));
        }
        p_->dataBytes += static_cast<uint32_t>(samples * sizeof(int32_t));
        return true;
    }
    return false;
}

bool WavWriter::close() {
    if (!p_ || !p_->opened) return false;
    // patch sizes
    std::ostream& os = p_->os;
    std::streampos endPos = os.tellp();
    uint32_t riffSize = 36 + p_->dataBytes;
    // write data chunk size
    os.seekp(p_->dataSizePos, std::ios::beg); writeBytes(os, &p_->dataBytes, sizeof(uint32_t));
    // write riff size
    os.seekp(offsetof(RiffHeader, size), std::ios::beg); writeBytes(os, &riffSize, sizeof(uint32_t));
    // restore to end and close
    os.seekp(endPos, std::ios::beg);
    p_->os.close();
    p_->opened = false;
    return true;
}

// ----------------- Backward-compat helpers -----------------
bool readWav(const std::string& path, WavData& out) {
    WavReader r; if (!r.open(path)) return false; return r.readAll(out);
}

bool writeWav16(const std::string& path, const WavData& in, float peak) {
    WavWriter w; if (!w.open(path, in.sampleRate, in.channels, 16, SampleFormat::PCM)) return false; bool ok = w.writeFrames(in.samples.data(), in.samples.size()/in.channels, peak) && w.close(); return ok;
}

bool writeWav32f(const std::string& path, const WavData& in) {
    WavWriter w; if (!w.open(path, in.sampleRate, in.channels, 32, SampleFormat::IEEE_FLOAT)) return false; bool ok = w.writeFrames(in.samples.data(), in.samples.size()/in.channels) && w.close(); return ok;
}

} // namespace vv::wavio
