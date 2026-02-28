#include "ConfigManager.h"
#include "../utils/Logger.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ehr {

bool ConfigManager::load(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        LOG_ERROR("ConfigManager: cannot open config file: " + configPath);
        return false;
    }
    try {
        file >> config_;
        loaded_ = true;
        LOG_INFO("ConfigManager: loaded configuration from " + configPath);
        return true;
    } catch (const nlohmann::json::exception& ex) {
        LOG_ERROR(std::string("ConfigManager: JSON parse error: ") + ex.what());
        return false;
    }
}

const nlohmann::json* ConfigManager::resolve(const std::string& key) const {
    if (!loaded_) return nullptr;
    const nlohmann::json* node = &config_;
    std::istringstream ss(key);
    std::string part;
    while (std::getline(ss, part, '.')) {
        if (!node->is_object() || !node->contains(part)) return nullptr;
        node = &(*node)[part];
    }
    return node;
}

std::string ConfigManager::getString(const std::string& key,
                                     const std::string& defaultVal) const {
    const nlohmann::json* n = resolve(key);
    if (n && n->is_string()) return n->get<std::string>();
    return defaultVal;
}

bool ConfigManager::getBool(const std::string& key, bool defaultVal) const {
    const nlohmann::json* n = resolve(key);
    if (n && n->is_boolean()) return n->get<bool>();
    return defaultVal;
}

int ConfigManager::getInt(const std::string& key, int defaultVal) const {
    const nlohmann::json* n = resolve(key);
    if (n && n->is_number_integer()) return n->get<int>();
    return defaultVal;
}

std::string ConfigManager::databasePath()  const { return getString("database.path",  "ehr.db"); }
std::string ConfigManager::cdssRulesPath() const { return getString("cdss.rules_path","cdss/rules"); }
bool        ConfigManager::cdssEnabled()   const { return getBool  ("cdss.enabled",   true); }
std::string ConfigManager::logLevel()      const { return getString("logging.level",   "INFO"); }
std::string ConfigManager::logFile()       const { return getString("logging.file",    ""); }

} // namespace ehr
