#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vv {

// Placeholder implementations standing in for DIO/Harvest, CheapTrick, D4C
std::vector<float> estimateF0(const std::vector<float>& x, uint32_t sr, size_t hop);
std::vector<float> simpleSpectralEnvelope(const std::vector<float>& x, size_t frameSize, size_t hop);
std::vector<float> simpleAperiodicity(const std::vector<float>& x, size_t frameSize, size_t hop);

} // namespace vv
