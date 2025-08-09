#pragma once

#include <vector>
#include <complex>
#include <cmath>

namespace vv::fft {

// Minimal Cooley-Tukey radix-2 FFT for power-of-two sizes
// For production, replace with FFTW/KissFFT integration.
class FFTWrapper {
public:
    static void fft(std::vector<std::complex<float>>& a) { transform(a, false); }
    static void ifft(std::vector<std::complex<float>>& a) { transform(a, true); }

private:
    static void transform(std::vector<std::complex<float>>& a, bool inverse) {
        const size_t n = a.size();
        if ((n & (n-1)) != 0) return; // require power of two
        // bit-reverse
        size_t j = 0;
        for (size_t i=1;i<n;i++) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(a[i], a[j]);
        }
        for (size_t len=2; len<=n; len<<=1) {
            float ang = 2.0f * 3.14159265358979323846f / static_cast<float>(len);
            if (inverse) ang = -ang;
            std::complex<float> wlen(std::cos(ang), std::sin(ang));
            for (size_t i=0; i<n; i+=len) {
                std::complex<float> w(1.0f, 0.0f);
                for (size_t j2=0; j2<len/2; ++j2) {
                    auto u = a[i+j2];
                    auto v = a[i+j2+len/2] * w;
                    a[i+j2] = u + v;
                    a[i+j2+len/2] = u - v;
                    w *= wlen;
                }
            }
        }
        if (inverse) {
            for (auto& x : a) x /= static_cast<float>(n);
        }
    }
};

} // namespace vv::fft
