#include "world/WorldSynthesisNode.h"
#include <algorithm>
#include <cmath>
#ifdef VV_USE_WORLD
extern "C" {
#include <world/synthesis.h>
}
#endif

namespace vv {

std::shared_ptr<DataPacket> WorldSynthesisNode::process(const std::shared_ptr<const DataPacket>& input) {
    if (!input) return nullptr;
    // Very naive synthesis: passthrough samples if available, else synthesize sin from f0
    if (!input->samples().empty()) {
        auto out = std::make_shared<DataPacket>(*input);
        // Apply optional post-filters
        if (postFilterEnabled_ && !filters_.empty() && out->channels() >= 1) {
            const uint32_t sr = out->sampleRate();
            const uint16_t ch = out->channels();
            if (!prepared_ || preparedSampleRate_ != sr || preparedChannels_ != ch) {
                for (auto& f : filters_) if (f) { f->prepare(sr, ch); }
                prepared_ = true; preparedSampleRate_ = sr; preparedChannels_ = ch;
            }
            // DataPacket stores interleaved mono/stereo in samples(); compute frames
            size_t frames = ch > 0 ? out->samples().size() / ch : out->samples().size();
            float* data = out->samples().data();
            for (auto& f : filters_) if (f && frames > 0) { f->process(data, frames); }
        }
        return out;
    }
    auto out = std::make_shared<DataPacket>(std::vector<float>{}, input->sampleRate(), input->channels(), input->bitDepth());
    const auto* f0 = input->getFeature("f0");
    if (!f0 || f0->empty()) return out;
    const uint32_t sr = input->sampleRate();

#ifdef VV_USE_WORLD
    // Attempt WORLD resynthesis if we have spectral envelopes and aperiodicity
    const auto* env = input->getFeature("spectral_envelope");
    const auto* ap = input->getFeature("aperiodicity");
    const int f0_length = static_cast<int>(input->getScalar("world_f0_len", 0.0));
    const int fft_size = static_cast<int>(input->getScalar("world_fft_size", 0.0));
    const int bins = static_cast<int>(input->getScalar("world_bins", 0.0));
    if (env && ap && f0_length > 0 && bins > 0 && fft_size > 0 && static_cast<int>(f0->size()) == f0_length && static_cast<int>(env->size()) == f0_length * bins && static_cast<int>(ap->size()) == f0_length * bins) {
        std::vector<double> f0_d(f0->begin(), f0->end());
        std::vector<double> time_axis(f0_length);
        // Use frame period from f0 spacing: assume constant; default to 5 ms
        double frame_period_ms = 5.0;
        if (f0_length > 1) {
            frame_period_ms = 1000.0 * input->getScalar("world_hop_size", static_cast<double>(sr / 200.0)) / static_cast<double>(sr);
        }
        for (int i = 0; i < f0_length; ++i) time_axis[i] = i * frame_period_ms / 1000.0;
        // Rebuild matrices
        std::vector<const double*> spec_rows(f0_length);
        std::vector<const double*> ap_rows(f0_length);
        std::vector<double> spec_d(env->begin(), env->end());
        std::vector<double> ap_d(ap->begin(), ap->end());
        for (int t = 0; t < f0_length; ++t) {
            spec_rows[t] = spec_d.data() + static_cast<size_t>(t) * bins;
            ap_rows[t] = ap_d.data() + static_cast<size_t>(t) * bins;
        }
        // Output length
    int y_length = static_cast<int>(std::ceil((f0_length * frame_period_ms / 1000.0) * static_cast<double>(sr)));
        std::vector<double> y(y_length);
        Synthesis(f0_d.data(), f0_length, spec_rows.data(), ap_rows.data(), fft_size, frame_period_ms, sr, y_length, y.data());
        out->samples().reserve(y.size());
        for (double v : y) out->samples().push_back(static_cast<float>(v));
        // Apply optional post-filters
        if (postFilterEnabled_ && !filters_.empty() && out->channels() >= 1 && !out->samples().empty()) {
            const uint32_t sr2 = out->sampleRate();
            const uint16_t ch2 = out->channels();
            if (!prepared_ || preparedSampleRate_ != sr2 || preparedChannels_ != ch2) {
                for (auto& f : filters_) if (f) { f->prepare(sr2, ch2); }
                prepared_ = true; preparedSampleRate_ = sr2; preparedChannels_ = ch2;
            }
            size_t frames2 = ch2 > 0 ? out->samples().size() / ch2 : out->samples().size();
            float* data2 = out->samples().data();
            for (auto& f : filters_) if (f && frames2 > 0) { f->process(data2, frames2); }
        }
        return out;
    }
#endif

    // Fallback: Synthesize 1 frame per f0 hop, 10ms per hop as placeholder
    const size_t hopSamples = static_cast<size_t>(0.01 * sr);
    float phase = 0.0f;
    for (float f : *f0) {
        size_t N = hopSamples;
        for (size_t n = 0; n < N; ++n) {
            float omega = 2.0f * 3.1415926535f * (f > 0.0f ? f : 100.0f) / static_cast<float>(sr);
            phase += omega;
            out->samples().push_back(std::sin(phase) * 0.1f);
        }
    }
    // Apply optional post-filters
    if (postFilterEnabled_ && !filters_.empty() && out->channels() >= 1 && !out->samples().empty()) {
        const uint32_t sr3 = out->sampleRate();
        const uint16_t ch3 = out->channels();
        if (!prepared_ || preparedSampleRate_ != sr3 || preparedChannels_ != ch3) {
            for (auto& f : filters_) if (f) { f->prepare(sr3, ch3); }
            prepared_ = true; preparedSampleRate_ = sr3; preparedChannels_ = ch3;
        }
        size_t frames3 = ch3 > 0 ? out->samples().size() / ch3 : out->samples().size();
        float* data3 = out->samples().data();
        for (auto& f : filters_) if (f && frames3 > 0) { f->process(data3, frames3); }
    }
    return out;
}

} // namespace vv
