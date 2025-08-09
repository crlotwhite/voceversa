#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace vv {

// Generic audio/data packet
class DataPacket {
public:
    using Clock = std::chrono::steady_clock;

    DataPacket() = default;

    explicit DataPacket(std::vector<float> samples,
                        uint32_t sampleRate = 48000,
                        uint16_t channels = 1,
                        uint16_t bitDepth = 32,
                        Clock::time_point ts = Clock::now())
        : samples_(std::move(samples)),
          sampleRate_(sampleRate),
          channels_(channels),
          bitDepth_(bitDepth),
          timestamp_(ts) {}

    // Copy/move defaulted for deep copy semantics on vector
    DataPacket(const DataPacket&) = default;
    DataPacket(DataPacket&&) noexcept = default;
    DataPacket& operator=(const DataPacket&) = default;
    DataPacket& operator=(DataPacket&&) noexcept = default;

    const std::vector<float>& samples() const { return samples_; }
    std::vector<float>& samples() { return samples_; }

    uint32_t sampleRate() const { return sampleRate_; }
    uint16_t channels() const { return channels_; }
    uint16_t bitDepth() const { return bitDepth_; }

    Clock::time_point timestamp() const { return timestamp_; }

    // Feature/metadata attachments for analysis pipelines
    void setFeature(const std::string& key, std::vector<float> values) {
        features_[key] = std::move(values);
    }
    bool hasFeature(const std::string& key) const {
        return features_.find(key) != features_.end();
    }
    const std::vector<float>* getFeature(const std::string& key) const {
        auto it = features_.find(key);
        return it == features_.end() ? nullptr : &it->second;
    }
    std::vector<float>* getFeature(const std::string& key) {
        auto it = features_.find(key);
        return it == features_.end() ? nullptr : &it->second;
    }

    void setScalar(const std::string& key, double value) {
        scalars_[key] = value;
    }
    bool hasScalar(const std::string& key) const {
        return scalars_.find(key) != scalars_.end();
    }
    double getScalar(const std::string& key, double def = 0.0) const {
        auto it = scalars_.find(key);
        return it == scalars_.end() ? def : it->second;
    }

    // Serialization placeholders (simple form)
    std::string serialize() const {
        // very simple csv-like format: sr,channels,bitdepth;count;values...
        std::string out = std::to_string(sampleRate_) + "," + std::to_string(channels_) + "," + std::to_string(bitDepth_) + ";" + std::to_string(samples_.size()) + ";";
        for (size_t i = 0; i < samples_.size(); ++i) {
            out += std::to_string(samples_[i]);
            if (i + 1 < samples_.size()) out += ",";
        }
        return out;
    }
    static DataPacket deserialize(const std::string& data) {
        // naive parser for the serialize() format
        DataPacket pkt; // defaults
        auto firstSemi = data.find(';');
        auto secondSemi = data.find(';', firstSemi + 1);
        if (firstSemi == std::string::npos || secondSemi == std::string::npos) return pkt;
        auto header = data.substr(0, firstSemi);
        auto countStr = data.substr(firstSemi + 1, secondSemi - firstSemi - 1);
        size_t count = static_cast<size_t>(std::stoul(countStr));
        // header sr,channels,bitdepth
        size_t p1 = header.find(',');
        size_t p2 = header.find(',', p1 + 1);
        if (p1 != std::string::npos && p2 != std::string::npos) {
            pkt.sampleRate_ = static_cast<uint32_t>(std::stoul(header.substr(0, p1)));
            pkt.channels_ = static_cast<uint16_t>(std::stoul(header.substr(p1 + 1, p2 - p1 - 1)));
            pkt.bitDepth_ = static_cast<uint16_t>(std::stoul(header.substr(p2 + 1)));
        }
        pkt.samples_.clear(); pkt.samples_.reserve(count);
        auto values = data.substr(secondSemi + 1);
        size_t start = 0;
        while (start < values.size() && pkt.samples_.size() < count) {
            size_t comma = values.find(',', start);
            auto token = values.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
            try { pkt.samples_.push_back(std::stof(token)); } catch (...) { pkt.samples_.push_back(0.0f); }
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
        // pad if fewer parsed
        while (pkt.samples_.size() < count) pkt.samples_.push_back(0.0f);
        return pkt;
    }

private:
    std::vector<float> samples_{};
    uint32_t sampleRate_{48000};
    uint16_t channels_{1};
    uint16_t bitDepth_{32};
    Clock::time_point timestamp_{Clock::now()};
    std::unordered_map<std::string, std::vector<float>> features_{};
    std::unordered_map<std::string, double> scalars_{};
};

} // namespace vv
