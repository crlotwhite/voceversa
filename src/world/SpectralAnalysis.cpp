#include <vector>
#include <algorithm>
#include <cmath>
#include "world/AnalysisUtils.h"

namespace vv {

// Placeholder spectral envelope using simple magnitude smoothing via moving average
std::vector<float> simpleSpectralEnvelope(const std::vector<float>& x, size_t frameSize, size_t hop) {
    if (x.empty()) return {};
    std::vector<float> env;
    for (size_t i = 0; i < x.size(); i += hop) {
        size_t end = std::min(x.size(), i + frameSize);
        float sum = 0.0f; size_t n = 0;
        for (size_t j = i; j < end; ++j) { sum += std::abs(x[j]); ++n; }
        env.push_back(n ? sum / static_cast<float>(n) : 0.0f);
    }
    return env;
}

// Placeholder aperiodicity: 1 - normalized autocorrelation peak
std::vector<float> simpleAperiodicity(const std::vector<float>& x, size_t frameSize, size_t hop) {
    if (x.empty()) return {};
    std::vector<float> ap;
    for (size_t i = 0; i < x.size(); i += hop) {
        size_t end = std::min(x.size(), i + frameSize);
        float mean = 0.0f; size_t n = 0;
        for (size_t j = i; j < end; ++j) { mean += x[j]; ++n; }
        mean = n ? mean / static_cast<float>(n) : 0.0f;
        float num = 0.0f, den = 0.0f;
        for (size_t j = i; j < end; ++j) {
            float v = x[j] - mean; num += v * v; den += std::abs(v);
        }
        float r = (den > 0.0f) ? (num / (den * den)) : 0.0f;
        ap.push_back(std::clamp(1.0f - r, 0.0f, 1.0f));
    }
    return ap;
}

} // namespace vv
