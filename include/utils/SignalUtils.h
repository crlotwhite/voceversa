#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace vv::signal {

inline void hanning(std::vector<float>& w) {
    const size_t N = w.size();
    for (size_t n=0;n<N;++n) w[n] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979323846 * n / (N-1)));
}
inline void hamming(std::vector<float>& w) {
    const size_t N = w.size();
    for (size_t n=0;n<N;++n) w[n] = 0.54f - 0.46f * std::cos(2.0f * 3.14159265358979323846 * n / (N-1));
}
inline void blackman(std::vector<float>& w) {
    const size_t N = w.size();
    const float a0=0.42f,a1=0.5f,a2=0.08f;
    for (size_t n=0;n<N;++n) {
        float t = 2.0f * 3.14159265358979323846f * n / (N-1);
        w[n] = a0 - a1*std::cos(t) + a2*std::cos(2.0f*t);
    }
}

// Simple linear resampler (ratio = outSR/inSR)
inline std::vector<float> resampleLinear(const std::vector<float>& x, float ratio) {
    if (x.empty() || ratio <= 0.0f) return {};
    size_t outN = static_cast<size_t>(std::ceil(x.size() * ratio));
    std::vector<float> y(outN);
    for (size_t i=0; i<outN; ++i) {
        float pos = i / ratio; // position in input
        size_t i0 = static_cast<size_t>(pos);
        float frac = pos - i0;
        size_t i1 = std::min(i0 + 1, x.size()-1);
        y[i] = x[i0]*(1.0f-frac) + x[i1]*frac;
    }
    return y;
}

} // namespace vv::signal
