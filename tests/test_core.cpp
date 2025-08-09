#include <cassert>
#include <iostream>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "core/DataPacket.h"
#include "core/ComputationGraph.h"
#include "core/IPlatformIO.h"
#include "core/ISynthesisNode.h"
#include "utils/WavIO.h"
#include "utils/SignalUtils.h"
#include "utils/FFTWrapper.h"

using namespace vv;

// A simple mock node that copies input and scales samples
class GainNode : public ISynthesisNode {
public:
    explicit GainNode(float g, std::string id) : gain_(g) { setId(std::move(id)); setName("Gain"); }
    bool initialize() override { return true; }
    std::shared_ptr<DataPacket> process(const std::shared_ptr<const DataPacket>& input) override {
        auto out = std::make_shared<DataPacket>(*input);
        for (auto& s : out->samples()) s *= gain_;
        return out;
    }
    std::vector<std::string> getInputs() const override { return {"in"}; }
    std::vector<std::string> getOutputs() const override { return {"out"}; }
private:
    float gain_;
};

int main() {
    // DataPacket basic behavior
    std::vector<float> samples = {0.5f, -0.5f, 1.0f};
    DataPacket pkt(samples, 48000, 1, 32);
    assert(pkt.samples().size() == samples.size());

    // PlatformIO
    auto io = makePlatformIO();
    auto mem = io->allocateMemory(1024);
    assert(mem.ptr != nullptr && mem.size == 1024);
    io->deallocateMemory(mem);
    assert(mem.size == 0);

    // Graph topology and processing order
    ComputationGraph g;
    auto n1 = std::make_shared<GainNode>(2.0f, "n1");
    auto n2 = std::make_shared<GainNode>(0.5f, "n2");
    assert(g.addNode(n1));
    assert(g.addNode(n2));
    assert(g.connectNodes("n1", "n2"));

    auto order = g.topologicalOrder();
    // n1 should come before n2
    assert(order.size() == 2);
    auto it1 = std::find(order.begin(), order.end(), "n1");
    auto it2 = std::find(order.begin(), order.end(), "n2");
    assert(it1 < it2);

    // Process via mock nodes
    auto in = std::make_shared<DataPacket>(samples);
    auto out1 = n1->process(in);
    auto out2 = n2->process(out1);
    // overall gain 2.0 * 0.5 = 1.0 -> expect equality to input
    for (size_t i = 0; i < samples.size(); ++i) {
        assert(std::abs(out2->samples()[i] - samples[i]) < 1e-6f);
    }

    std::cout << "core tests passed\n";

    // Basic WAV roundtrip (16-bit)
    {
        vv::wavio::WavData wd; wd.sampleRate = 48000; wd.channels = 1; wd.samples = {0.0f, 0.5f, -0.5f, 1.0f, -1.0f};
        assert(vv::wavio::writeWav16("/tmp/vv_test16.wav", wd));
        vv::wavio::WavData rd; assert(vv::wavio::readWav("/tmp/vv_test16.wav", rd));
        assert(rd.sampleRate == wd.sampleRate && rd.channels == wd.channels && rd.samples.size() == wd.samples.size());
    }

    // Window functions size check and energy sanity
    {
        std::vector<float> w(128);
        vv::signal::hanning(w); float sum1=0; for (auto v: w) sum1+=v; assert(sum1>0);
        vv::signal::hamming(w); float sum2=0; for (auto v: w) sum2+=v; assert(sum2>0);
        vv::signal::blackman(w); float sum3=0; for (auto v: w) sum3+=v; assert(sum3>0);
    }

    // FFT invertibility
    {
        const size_t N = 256;
        std::vector<std::complex<float>> x(N);
    const float PI = 3.14159265358979323846f;
    for (size_t n=0;n<N;++n) x[n] = std::complex<float>(std::sin(2*PI*n/N), 0.0f);
        auto X = x; vv::fft::FFTWrapper::fft(X); vv::fft::FFTWrapper::ifft(X);
        for (size_t n=0;n<N;++n) assert(std::abs(X[n].real() - x[n].real()) < 1e-3f);
    }

    std::cout << "audio utils tests passed\n";
    return 0;
}
