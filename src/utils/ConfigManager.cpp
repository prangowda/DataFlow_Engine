#include "../../include/utils/ConfigManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

namespace dataflow {
namespace utils {

ConfigManager::ConfigManager(const std::string& configFile)
    : configFile_(configFile) {
    
    if (!configFile_.empty()) {
        loadFromFile(configFile_);
    }
}

ConfigManager::~ConfigManager() {
    // Save to file if one was provided
    if (!configFile_.empty()) {
        try {
            saveToFile(configFile_);
        } catch (const std::exception& e) {
            std::cerr << "Error saving configuration: " << e.what() << std::endl;
        }
    }
}

bool ConfigManager::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string currentJson;
    
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        currentJson += line;
    }
    
    // Parse JSON content
    // In a real implementation, use a proper JSON library
    // This is a simplified version for demo purposes
    
    // Simple regex-based JSON parsing (very limited)
    std::regex keyValuePattern("\"([^\"]+)\"\\s*:\\s*([^,}]+)");
    
    auto begin = std::sregex_iterator(currentJson.begin(), currentJson.end(), keyValuePattern);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::smatch match = *i;
        std::string key = match[1].str();
        std::string value = match[2].str();
        
        // Trim value
        value.erase(0, value.find_first_not_of(" \t\n\r\""));
        value.erase(value.find_last_not_of(" \t\n\r\"") + 1);
        
        parseJsonValue(value, key);
    }
    
    return true;
}

bool ConfigManager::saveToFile(const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    
    // Get all keys for iteration
    auto keys = getAllKeys();
    
    for (size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys[i];
        file << "  \"" << key << "\": " << serializeValue(key);
        
        if (i < keys.size() - 1) {
            file << ",";
        }
        
        file << "\n";
    }
    
    file << "}\n";
    
    return true;
}

bool ConfigManager::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return valueMap_.find(key) != valueMap_.end();
}

bool ConfigManager::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = valueMap_.find(key);
    if (it == valueMap_.end()) {
        return false;
    }
    
    valueMap_.erase(it);
    typeMap_.erase(key);
    
    notifyObservers(key);
    return true;
}

std::vector<std::string> ConfigManager::getAllKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> keys;
    keys.reserve(valueMap_.size());
    
    for (const auto& pair : valueMap_) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

int ConfigManager::registerObserver(const std::string& key, 
                                 std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int id = nextObserverId_++;
    Observer observer{id, key, std::move(callback)};
    observers_.push_back(std::move(observer));
    
    return id;
}

bool ConfigManager::unregisterObserver(int observerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(observers_.begin(), observers_.end(),
        [observerId](const Observer& obs) {
            return obs.id == observerId;
        });
    
    if (it == observers_.end()) {
        return false;
    }
    
    observers_.erase(it);
    return true;
}

void ConfigManager::notifyObservers(const std::string& key) {
    // Copy observers to avoid issues if callbacks modify the list
    std::vector<Observer> observersCopy;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        observersCopy.reserve(observers_.size());
        for (const auto& obs : observers_) {
            if (obs.key == key || obs.key.empty()) {
                observersCopy.push_back(obs);
            }
        }
    }
    
    // Notify outside of lock
    for (const auto& obs : observersCopy) {
        try {
            obs.callback(key);
        } catch (const std::exception& e) {
            std::cerr << "Error in observer callback: " << e.what() << std::endl;
        }
    }
}

bool ConfigManager::parseJsonValue(const std::string& jsonValue, const std::string& key) {
    // Simple type detection and parsing
    if (jsonValue == "true") {
        setValue<bool>(key, true);
        return true;
    } else if (jsonValue == "false") {
        setValue<bool>(key, false);
        return true;
    } else if (jsonValue.front() == '"' && jsonValue.back() == '"') {
        // String (remove quotes)
        setValue<std::string>(key, jsonValue.substr(1, jsonValue.length() - 2));
        return true;
    } else {
        // Try as number
        try {
            // Check if it's a floating point
            if (jsonValue.find('.') != std::string::npos) {
                setValue<double>(key, std::stod(jsonValue));
            } else {
                setValue<int>(key, std::stoi(jsonValue));
            }
            return true;
        } catch (...) {
            // Not a recognized type
            return false;
        }
    }
}

std::string ConfigManager::serializeValue(const std::string& key) const {
    auto typeIt = typeMap_.find(key);
    if (typeIt == typeMap_.end()) {
        return "null";
    }
    
    auto valueIt = valueMap_.find(key);
    if (valueIt == valueMap_.end()) {
        return "null";
    }
    
    const std::type_index& type = typeIt->second;
    const std::any& value = valueIt->second;
    
    // Handle different types
    if (type == std::type_index(typeid(int))) {
        return std::to_string(std::any_cast<int>(value));
    } else if (type == std::type_index(typeid(double))) {
        return std::to_string(std::any_cast<double>(value));
    } else if (type == std::type_index(typeid(bool))) {
        return std::any_cast<bool>(value) ? "true" : "false";
    } else if (type == std::type_index(typeid(std::string))) {
        return "\"" + std::any_cast<std::string>(value) + "\"";
    } else {
        return "null";
    }
}

} // namespace utils
} // namespace dataflow
