#include "core/IPlatformIO.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace vv {

namespace {
struct Deleter {
    void operator()(std::byte* p) const noexcept { delete[] p; }
};
}

class LinuxPlatformIO : public IPlatformIO {
public:
    std::vector<std::byte> readFile(const std::string& path) override {
        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        if (!ifs) return {};
        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        if (size <= 0) return {};
        std::vector<std::byte> buf(static_cast<size_t>(size));
        ifs.read(reinterpret_cast<char*>(buf.data()), size);
        if (!ifs) return {};
        return buf;
    }

    bool writeFile(const std::string& path, const std::vector<std::byte>& data) override {
        std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
        if (!ofs) return false;
        ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        return ofs.good();
    }

    MemoryBlock allocateMemory(size_t bytes) override {
        auto* raw = new std::byte[bytes]{};
        return MemoryBlock{ std::unique_ptr<std::byte[], void(*)(std::byte*)>(raw, [](std::byte* p){ delete[] p; }), bytes };
    }

    void deallocateMemory(MemoryBlock& block) override {
        if (block.ptr) {
            block.ptr.reset();
            block.size = 0;
        }
    }
};

std::unique_ptr<IPlatformIO> makePlatformIO() {
    return std::make_unique<LinuxPlatformIO>();
}

} // namespace vv
