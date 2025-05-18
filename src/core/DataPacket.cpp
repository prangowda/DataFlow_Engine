#include "../../include/core/DataPacket.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace dataflow {
namespace core {

std::atomic<uint64_t> DataPacket::nextId_{0};

DataPacket::DataPacket(std::string id)
    : creationTime_(std::chrono::high_resolution_clock::now()),
      lastModified_(creationTime_) {
    
    if (id.empty()) {
        id_ = generateUniqueId();
    } else {
        id_ = std::move(id);
    }
}

bool DataPacket::hasKey(const std::string& key) const {
    return dataMap_.find(key) != dataMap_.end();
}

std::vector<std::string> DataPacket::getKeys() const {
    std::vector<std::string> keys;
    keys.reserve(dataMap_.size());
    
    for (const auto& pair : dataMap_) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

std::shared_ptr<DataPacket> DataPacket::clone() const {
    auto newPacket = std::make_shared<DataPacket>(id_ + "_clone");
    
    // Copy all data
    newPacket->dataMap_ = dataMap_;
    newPacket->typeMap_ = typeMap_;
    
    return newPacket;
}

std::string DataPacket::generateUniqueId() {
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = nowMs.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    
    // Combine with atomic counter for uniqueness
    uint64_t id = nextId_++;
    
    // Generate random hex suffix for additional uniqueness
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 15);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << "pkt_" << value << "_" << id << "_";
    
    // Add a random suffix
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << distrib(gen);
    }
    
    return ss.str();
}

} // namespace core
} // namespace dataflow
