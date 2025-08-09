#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace vv {

struct UtauWorldMeta {
    uint32_t sampleRate{48000};
    int fftSize{0};
    int bins{0};
    int frames{0};
    double framePeriodMs{5.0};
    size_t sampleCount{0};
};

// Simple UTAU-compatible WORLD IO (text f0 + binary sp/ap as double, row-major [T x bins])
namespace UtauWorldIO {
    // Write one value per line (text)
    bool writeF0Txt(const std::string& path, const std::vector<float>& f0);
    bool readF0Txt(const std::string& path, std::vector<float>& f0);

    // Binary little-endian double matrices (row-major T*bins)
    bool writeBinMatrix(const std::string& path, const std::vector<float>& flat, int frames, int bins);
    bool readBinMatrix(const std::string& path, std::vector<float>& flat, int& frames, int& bins);

    // Metadata JSON for safe round-trip
    bool writeMetaJson(const std::string& path, const UtauWorldMeta& m);
    bool readMetaJson(const std::string& path, UtauWorldMeta& m);
}

} // namespace vv
