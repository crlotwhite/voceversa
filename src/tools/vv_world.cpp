#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cmath>

#include "core/DataPacket.h"
#include "world/WorldAnalysisNode.h"
#include "world/WorldSynthesisNode.h"
#include "world/UtauWorldInterface.h"
#include "utils/WavIO.h"

using namespace vv;

static void print_usage() {
    std::cout << "Usage:\n"
              << "  vv_world analyze <wav> <out_dir>\n"
              << "  vv_world synth <out_dir> <out_wav>\n"
              << "Notes: analyze writes f0.txt, sp.bin, ap.bin, meta.json under out_dir.\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(); return 1; }
    std::string cmd = argv[1];

    if (cmd == "analyze") {
        if (argc < 4) { print_usage(); return 1; }
        std::string wav = argv[2];
        std::string outdir = argv[3];
        // Read WAV
        vv::wavio::WavData wd; if (!vv::wavio::readWav(wav, wd)) { std::cerr << "Failed to read WAV\n"; return 2; }
        auto in = std::make_shared<DataPacket>(wd.samples, wd.sampleRate, wd.channels, 32);
        WorldParams params; params.sampleRate = wd.sampleRate; params.hopSize = static_cast<size_t>(0.005 * wd.sampleRate); params.frameSize = 1024;
        WorldAnalysisNode analysis(params); analysis.initialize();
        auto analyzed = analysis.process(in);
        if (!analyzed) { std::cerr << "Analysis failed\n"; return 3; }
        // Prepare outputs
        const auto* f0 = analyzed->getFeature("f0");
        const auto* sp = analyzed->getFeature("spectral_envelope");
        const auto* ap = analyzed->getFeature("aperiodicity");
        if (!f0 || !sp || !ap) { std::cerr << "Missing analysis features\n"; return 3; }
        UtauWorldMeta meta; meta.sampleRate = wd.sampleRate;
        meta.fftSize = static_cast<int>(analyzed->getScalar("world_fft_size", 0));
        meta.bins = static_cast<int>(analyzed->getScalar("world_bins", meta.fftSize>0? meta.fftSize/2+1:0));
        meta.frames = static_cast<int>(analyzed->getScalar("world_f0_len", f0->size()));
        meta.framePeriodMs = 1000.0 * analyzed->getScalar("world_hop_size", params.hopSize) / static_cast<double>(wd.sampleRate);
        meta.sampleCount = wd.samples.size();
        // Ensure outdir exists (best effort)
        std::string mkdirCmd = std::string("mkdir -p \"") + outdir + "\"";
        (void)std::system(mkdirCmd.c_str());
        // Write files
        if (!UtauWorldIO::writeF0Txt(outdir+"/f0.txt", *f0)) { std::cerr << "Write f0.txt failed\n"; return 4; }
        if (!UtauWorldIO::writeBinMatrix(outdir+"/sp.bin", *sp, meta.frames, meta.bins)) { std::cerr << "Write sp.bin failed\n"; return 4; }
        if (!UtauWorldIO::writeBinMatrix(outdir+"/ap.bin", *ap, meta.frames, meta.bins)) { std::cerr << "Write ap.bin failed\n"; return 4; }
        if (!UtauWorldIO::writeMetaJson(outdir+"/meta.json", meta)) { std::cerr << "Write meta.json failed\n"; return 4; }
        std::cout << "Analysis complete: " << outdir << "\n";
        return 0;
    } else if (cmd == "synth") {
        if (argc < 4) { print_usage(); return 1; }
        std::string dir = argv[2];
        std::string outwav = argv[3];
        std::vector<float> f0, sp, ap;
        int frames = 0, bins = 0;
        UtauWorldMeta meta;
        if (!UtauWorldIO::readF0Txt(dir + "/f0.txt", f0)) { std::cerr << "Failed to read f0.txt\n"; return 3; }
        if (!UtauWorldIO::readBinMatrix(dir + "/sp.bin", sp, frames, bins)) { std::cerr << "Failed to read sp.bin\n"; return 3; }
        if (!UtauWorldIO::readBinMatrix(dir + "/ap.bin", ap, frames, bins)) { std::cerr << "Failed to read ap.bin\n"; return 3; }
        if (!UtauWorldIO::readMetaJson(dir + "/meta.json", meta)) { std::cerr << "Failed to read meta.json\n"; return 3; }

        auto pkt = std::make_shared<DataPacket>(std::vector<float>{}, meta.sampleRate, 1, 32);
        pkt->setFeature("f0", std::move(f0));
        pkt->setFeature("spectral_envelope", std::move(sp));
        pkt->setFeature("aperiodicity", std::move(ap));
        pkt->setScalar("world_fft_size", meta.fftSize);
        pkt->setScalar("world_bins", meta.bins);
        pkt->setScalar("world_f0_len", meta.frames);
        pkt->setScalar("world_hop_size", meta.framePeriodMs * meta.sampleRate / 1000.0);

        WorldSynthesisNode synth;
        synth.initialize();
        auto out = synth.process(pkt);
        if (!out) { std::cerr << "Synthesis failed\n"; return 4; }
        vv::wavio::WavData wd; wd.sampleRate = meta.sampleRate; wd.channels = 1; wd.samples = out->samples();
        if (!vv::wavio::writeWav16(outwav, wd)) { std::cerr << "Failed to write WAV\n"; return 5; }
        std::cout << "Wrote: " << outwav << "\n";
        return 0;
    }

    print_usage();
    return 1;
}
