#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <any>
#include <typeindex>
#include <functional>
#include <chrono>
#include <atomic>

namespace dataflow {
namespace core {

/**
 * @class DataPacket
 * @brief Container for data that flows through the processing system
 * 
 * This class provides a generic container for different types of data.
 * It uses type erasure to store any data type and provides metadata 
 * for tracking the packet through the system.
 */
class DataPacket {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    
    /**
     * @brief Constructor
     * @param id Unique identifier for this packet
     */
    explicit DataPacket(std::string id = "");
    
    /**
     * @brief Gets the unique ID of this packet
     * @return The packet ID
     */
    const std::string& getId() const { return id_; }
    
    /**
     * @brief Sets data of a specific type in the packet
     * @tparam T Type of data
     * @param key Key for accessing this data
     * @param value The data value
     */
    template<typename T>
    void setData(const std::string& key, T&& value) {
        std::type_index typeId = std::type_index(typeid(T));
        
        // Store type information
        typeMap_[key] = typeId;
        
        // Store the actual data
        dataMap_[key] = std::forward<T>(value);
        
        // Update metadata
        lastModified_ = std::chrono::high_resolution_clock::now();
    }
    
    /**
     * @brief Gets data of a specific type from the packet
     * @tparam T Expected type of the data
     * @param key Key to look up
     * @return The data value, or nullopt if not found or wrong type
     */
    template<typename T>
    std::optional<T> getData(const std::string& key) const {
        // Check if key exists
        auto dataIt = dataMap_.find(key);
        if (dataIt == dataMap_.end()) {
            return std::nullopt;
        }
        
        // Verify type
        auto typeIt = typeMap_.find(key);
        if (typeIt == typeMap_.end() || typeIt->second != std::type_index(typeid(T))) {
            return std::nullopt;
        }
        
        try {
            return std::any_cast<T>(dataIt->second);
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }
    
    /**
     * @brief Checks if the packet contains data with the specified key
     * @param key Key to check
     * @return true if the key exists, false otherwise
     */
    bool hasKey(const std::string& key) const;
    
    /**
     * @brief Gets the time the packet was created
     * @return Creation timestamp
     */
    TimePoint getCreationTime() const { return creationTime_; }
    
    /**
     * @brief Gets the time the packet was last modified
     * @return Last modification timestamp
     */
    TimePoint getLastModifiedTime() const { return lastModified_; }
    
    /**
     * @brief Gets all keys in the packet
     * @return Vector of all keys
     */
    std::vector<std::string> getKeys() const;
    
    /**
     * @brief Creates a deep copy of this packet
     * @return A new packet with copies of all data
     */
    std::shared_ptr<DataPacket> clone() const;
    
    /**
     * @brief Generates a unique ID for a packet
     * @return A unique string ID
     */
    static std::string generateUniqueId();

private:
    std::string id_;
    TimePoint creationTime_;
    TimePoint lastModified_;
    
    // Type-erased storage
    std::unordered_map<std::string, std::any> dataMap_;
    std::unordered_map<std::string, std::type_index> typeMap_;
    
    static std::atomic<uint64_t> nextId_;
};

} // namespace core
} // namespace dataflow
