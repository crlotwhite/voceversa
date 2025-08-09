#include "world/WorldAnalysisNode.h"
#include "world/AnalysisUtils.h"
#include <algorithm>
#include <cmath>
#ifdef VV_USE_WORLD
// WORLD headers
extern "C" {
#include <world/dio.h>
#include <world/harvest.h>
#include <world/cheaptrick.h>
#include <world/d4c.h>
}
#endif

namespace vv {

std::shared_ptr<DataPacket> WorldAnalysisNode::process(const std::shared_ptr<const DataPacket>& input) {
    if (!input) return nullptr;
    auto out = std::make_shared<DataPacket>(*input);
    const auto& x = input->samples();
    const uint32_t sr = input->sampleRate();
#ifdef VV_USE_WORLD
    if (!x.empty()) {
        // Parameters
        const double frame_period_ms = 1000.0 * static_cast<double>(params_.hopSize) / static_cast<double>(sr);
        // F0 using Harvest (robust) or DIO
    int x_length = static_cast<int>(x.size());
        std::vector<double> x_d(x.begin(), x.end());
    int f0_length = 1;
    std::vector<double> time_axis;
    std::vector<double> f0_d;
        HarvestOption hopts; InitializeHarvestOption(&hopts);
        hopts.frame_period = frame_period_ms;
        hopts.f0_floor = 50.0; hopts.f0_ceil = 1100.0;
    f0_length = GetSamplesForHarvest(sr, x_length, hopts.frame_period);
    time_axis.resize(f0_length); f0_d.resize(f0_length);
        Harvest(x_d.data(), x_length, sr, &hopts, time_axis.data(), f0_d.data());

        // Spectral envelope (CheapTrick)
        CheapTrickOption copts; InitializeCheapTrickOption(sr, &copts);
        copts.f0_floor = 50.0;
        int fft_size = GetFFTSizeForCheapTrick(sr, &copts);
        // out matrices: f0_length x (fft_size/2+1)
        int freq_bins = fft_size / 2 + 1;
        std::vector<double> spectrogram(static_cast<size_t>(f0_length) * freq_bins);
        std::vector<double*> spec_rows(f0_length);
        for (int t = 0; t < f0_length; ++t) spec_rows[t] = spectrogram.data() + static_cast<size_t>(t) * freq_bins;
        CheapTrick(x_d.data(), x_length, sr, time_axis.data(), f0_d.data(), f0_length, &copts, spec_rows.data());

        // Aperiodicity (D4C)
        D4COption d4c_opts; InitializeD4COption(&d4c_opts);
        std::vector<double> ap_mat(static_cast<size_t>(f0_length) * freq_bins);
        std::vector<double*> ap_rows(f0_length);
        for (int t = 0; t < f0_length; ++t) ap_rows[t] = ap_mat.data() + static_cast<size_t>(t) * freq_bins;
        D4C(x_d.data(), x_length, sr, time_axis.data(), f0_d.data(), f0_length, fft_size, &d4c_opts, ap_rows.data());

        // Flatten to float features. We also store meta: fft_size, bins, hopSize
        std::vector<float> f0(f0_length);
        for (int i = 0; i < f0_length; ++i) f0[i] = static_cast<float>(f0_d[i]);
        std::vector<float> env; env.reserve(static_cast<size_t>(f0_length) * freq_bins);
        for (int t = 0; t < f0_length; ++t) {
            for (int k = 0; k < freq_bins; ++k) env.push_back(static_cast<float>(spec_rows[t][k]));
        }
        std::vector<float> ap; ap.reserve(static_cast<size_t>(f0_length) * freq_bins);
        for (int t = 0; t < f0_length; ++t) {
            for (int k = 0; k < freq_bins; ++k) ap.push_back(static_cast<float>(ap_rows[t][k]));
        }
        out->setFeature("f0", std::move(f0));
        out->setFeature("spectral_envelope", std::move(env));
        out->setFeature("aperiodicity", std::move(ap));
        out->setScalar("world_fft_size", static_cast<double>(fft_size));
        out->setScalar("world_bins", static_cast<double>(freq_bins));
        out->setScalar("world_f0_len", static_cast<double>(f0_length));
        out->setScalar("world_hop_size", static_cast<double>(params_.hopSize));
        return out;
    }
#endif
    // Fallback simple implementations
    auto f0 = estimateF0(x, sr, params_.hopSize);
    auto env = simpleSpectralEnvelope(x, params_.frameSize, params_.hopSize);
    auto ap = simpleAperiodicity(x, params_.frameSize, params_.hopSize);
    out->setFeature("f0", std::move(f0));
    out->setFeature("spectral_envelope", std::move(env));
    out->setFeature("aperiodicity", std::move(ap));
    return out;
}

} // namespace vv
