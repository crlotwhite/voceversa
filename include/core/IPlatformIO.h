#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace vv {

struct MemoryBlock {
    std::unique_ptr<std::byte[], void(*)(std::byte*)> ptr{nullptr, nullptr};
    size_t size{0};
};

class IPlatformIO {
public:
    virtual ~IPlatformIO() = default;

    virtual std::vector<std::byte> readFile(const std::string& path) = 0;
    virtual bool writeFile(const std::string& path, const std::vector<std::byte>& data) = 0;

    virtual MemoryBlock allocateMemory(size_t bytes) = 0;
    virtual void deallocateMemory(MemoryBlock& block) = 0;
};

// Factory (simple)
std::unique_ptr<IPlatformIO> makePlatformIO();

} // namespace vv
