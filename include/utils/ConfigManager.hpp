#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <typeindex>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>
#include <optional>

namespace dataflow {
namespace utils {

/**
 * @class ConfigManager
 * @brief Manages configuration settings for the application
 * 
 * Provides a centralized location for all configuration settings.
 * Supports loading from and saving to JSON files, as well as
 * dynamic configuration changes at runtime.
 */
class ConfigManager {
public:
    /**
     * @brief Constructor
     * @param configFile Path to configuration file
     */
    explicit ConfigManager(const std::string& configFile = "");
    
    /**
     * @brief Destructor
     */
    ~ConfigManager();
    
    /**
     * @brief Load configuration from a file
     * @param filePath Path to the configuration file
     * @return true if successful, false otherwise
     */
    bool loadFromFile(const std::string& filePath);
    
    /**
     * @brief Save configuration to a file
     * @param filePath Path to save the configuration to
     * @return true if successful, false otherwise
     */
    bool saveToFile(const std::string& filePath);
    
    /**
     * @brief Set a configuration value
     * @tparam T Type of the value
     * @param key Configuration key
     * @param value Value to set
     */
    template<typename T>
    void setValue(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Store type information
        typeMap_[key] = std::type_index(typeid(T));
        
        // Store value
        valueMap_[key] = value;
        
        // Notify observers
        notifyObservers(key);
    }
    
    /**
     * @brief Get a configuration value
     * @tparam T Expected type of the value
     * @param key Configuration key
     * @param defaultValue Default value to return if key not found
     * @return The configuration value, or defaultValue if not found
     */
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if key exists
        auto valueIt = valueMap_.find(key);
        if (valueIt == valueMap_.end()) {
            return defaultValue;
        }
        
        // Verify type
        auto typeIt = typeMap_.find(key);
        if (typeIt == typeMap_.end() || typeIt->second != std::type_index(typeid(T))) {
            return defaultValue;
        }
        
        try {
            return std::any_cast<T>(valueIt->second);
        } catch (const std::bad_any_cast&) {
            return defaultValue;
        }
    }
    
    /**
     * @brief Check if a configuration key exists
     * @param key Configuration key
     * @return true if the key exists, false otherwise
     */
    bool hasKey(const std::string& key) const;
    
    /**
     * @brief Remove a configuration key
     * @param key Configuration key
     * @return true if removed, false if not found
     */
    bool removeKey(const std::string& key);
    
    /**
     * @brief Get all configuration keys
     * @return Vector of all keys
     */
    std::vector<std::string> getAllKeys() const;
    
    /**
     * @brief Register an observer for configuration changes
     * @param key Configuration key to observe
     * @param callback Function to call when the value changes
     * @return Unique ID for the observer
     */
    int registerObserver(const std::string& key, std::function<void(const std::string&)> callback);
    
    /**
     * @brief Unregister an observer
     * @param observerId ID returned from registerObserver
     * @return true if unregistered, false if not found
     */
    bool unregisterObserver(int observerId);

private:
    struct Observer {
        int id;
        std::string key;
        std::function<void(const std::string&)> callback;
    };
    
    std::string configFile_;
    std::unordered_map<std::string, std::any> valueMap_;
    std::unordered_map<std::string, std::type_index> typeMap_;
    
    std::vector<Observer> observers_;
    int nextObserverId_{0};
    
    mutable std::mutex mutex_;
    
    void notifyObservers(const std::string& key);
    bool parseJsonValue(const std::string& jsonValue, const std::string& key);
    std::string serializeValue(const std::string& key) const;
};

} // namespace utils
} // namespace dataflow
