#include "world/UtauWorldInterface.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace vv {

namespace {
    template <typename T>
    bool writeAll(std::ofstream& os, const T* data, size_t count) {
        os.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
        return static_cast<bool>(os);
    }
    template <typename T>
    bool readAll(std::ifstream& is, T* data, size_t count) {
        is.read(reinterpret_cast<char*>(data), sizeof(T) * count);
        return static_cast<bool>(is);
    }
}

bool UtauWorldIO::writeF0Txt(const std::string& path, const std::vector<float>& f0) {
    std::ofstream os(path);
    if (!os) return false;
    os.setf(std::ios::fixed); os.precision(8);
    for (float v : f0) os << v << "\n";
    return static_cast<bool>(os);
}

bool UtauWorldIO::readF0Txt(const std::string& path, std::vector<float>& f0) {
    std::ifstream is(path);
    if (!is) return false;
    f0.clear();
    std::string line;
    while (std::getline(is, line)) {
        if (line.empty()) continue;
        try { f0.push_back(std::stof(line)); } catch (...) { f0.push_back(0.0f); }
    }
    return true;
}

bool UtauWorldIO::writeBinMatrix(const std::string& path, const std::vector<float>& flat, int frames, int bins) {
    std::ofstream os(path, std::ios::binary);
    if (!os) return false;
    // store as double row-major for compatibility
    std::vector<double> buf(flat.begin(), flat.end());
    if (!writeAll(os, buf.data(), buf.size())) return false;
    return true;
}

bool UtauWorldIO::readBinMatrix(const std::string& path, std::vector<float>& flat, int& frames, int& bins) {
    std::ifstream is(path, std::ios::binary | std::ios::ate);
    if (!is) return false;
    auto sz = is.tellg();
    if (sz <= 0) return false;
    is.seekg(0, std::ios::beg);
    size_t nDoubles = static_cast<size_t>(sz) / sizeof(double);
    std::vector<double> buf(nDoubles);
    if (!readAll(is, buf.data(), nDoubles)) return false;
    flat.resize(nDoubles);
    for (size_t i = 0; i < nDoubles; ++i) flat[i] = static_cast<float>(buf[i]);
    // frames/bins will be provided by meta json; fall back to 0 if unknown
    if (frames <= 0 || bins <= 0) { frames = 0; bins = 0; }
    return true;
}

bool UtauWorldIO::writeMetaJson(const std::string& path, const UtauWorldMeta& m) {
    std::ofstream os(path);
    if (!os) return false;
    os << "{\n"
       << "  \"sampleRate\": " << m.sampleRate << ",\n"
       << "  \"fftSize\": " << m.fftSize << ",\n"
       << "  \"bins\": " << m.bins << ",\n"
       << "  \"frames\": " << m.frames << ",\n"
       << "  \"framePeriodMs\": " << m.framePeriodMs << ",\n"
       << "  \"sampleCount\": " << m.sampleCount << "\n"
       << "}\n";
    return static_cast<bool>(os);
}

bool UtauWorldIO::readMetaJson(const std::string& path, UtauWorldMeta& m) {
    std::ifstream is(path);
    if (!is) return false;
    std::stringstream ss; ss << is.rdbuf();
    std::string s = ss.str();
    auto findNum = [&](const char* key, double def)->double{
        auto p = s.find(key);
        if (p == std::string::npos) return def;
        p = s.find(":", p);
        if (p == std::string::npos) return def;
        size_t end = s.find_first_of(",}\n", p+1);
        try { return std::stod(s.substr(p+1, end-(p+1))); } catch (...) { return def; }
    };
    m.sampleRate = static_cast<uint32_t>(findNum("\"sampleRate\"", 48000));
    m.fftSize = static_cast<int>(findNum("\"fftSize\"", 0));
    m.bins = static_cast<int>(findNum("\"bins\"", 0));
    m.frames = static_cast<int>(findNum("\"frames\"", 0));
    m.framePeriodMs = findNum("\"framePeriodMs\"", 5.0);
    m.sampleCount = static_cast<size_t>(findNum("\"sampleCount\"", 0));
    return true;
}

} // namespace vv
