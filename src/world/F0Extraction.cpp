#include <cmath>
#include <vector>
#include <algorithm>
#include "world/AnalysisUtils.h"

namespace vv {

// Extremely simplified pitch extractor: autocorrelation-based, placeholder for DIO/Harvest
std::vector<float> estimateF0(const std::vector<float>& x, uint32_t sr, size_t hop) {
    const float fmin = 50.0f, fmax = 500.0f;
    size_t minLag = static_cast<size_t>(sr / fmax);
    size_t maxLag = static_cast<size_t>(sr / fmin);
    if (x.empty() || minLag >= x.size()) return {};
    std::vector<float> f0;
    for (size_t i = 0; i + maxLag < x.size(); i += hop) {
        float best = 0.0f; size_t bestLag = 0;
        for (size_t lag = minLag; lag <= maxLag; ++lag) {
            float sum = 0.0f;
            for (size_t j = 0; j < maxLag && i + j + lag < x.size(); ++j) {
                sum += x[i + j] * x[i + j + lag];
            }
            if (sum > best) { best = sum; bestLag = lag; }
        }
        if (bestLag > 0) f0.push_back(static_cast<float>(sr) / static_cast<float>(bestLag));
        else f0.push_back(0.0f);
    }
    return f0;
}

} // namespace vv
