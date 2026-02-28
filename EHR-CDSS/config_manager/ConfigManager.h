#pragma once

#include <string>
#include "../third_party/json/json.hpp"

namespace ehr {

/**
 * @brief Loads and provides read access to application configuration.
 *
 * The configuration is stored in a JSON file (config/config.json).
 * ConfigManager is a singleton that must be initialised once at startup.
 *
 * Example config.json:
 * {
 *   "database": { "path": "ehr.db" },
 *   "cdss":     { "enabled": true, "rules_path": "cdss/rules" },
 *   "logging":  { "level": "INFO", "file": "ehr.log" },
 *   "ui":       { "theme": "light" }
 * }
 */
class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    /** Load configuration from a JSON file. Returns true on success. */
    bool load(const std::string& configPath);

    /* ------------------------------------------------------------------ */
    /* Generic accessor – returns default if the key path is not found     */
    /* ------------------------------------------------------------------ */
    std::string getString(const std::string& key,
                          const std::string& defaultVal = "") const;
    bool        getBool  (const std::string& key,
                          bool               defaultVal = false) const;
    int         getInt   (const std::string& key,
                          int                defaultVal = 0) const;

    /* ------------------------------------------------------------------ */
    /* Convenience typed accessors                                         */
    /* ------------------------------------------------------------------ */
    std::string databasePath()  const;
    std::string cdssRulesPath() const;
    bool        cdssEnabled()   const;
    std::string logLevel()      const;
    std::string logFile()       const;

    const nlohmann::json& raw() const { return config_; }

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    nlohmann::json config_;
    bool           loaded_ = false;

    /* Resolve a dot-separated key path, e.g. "database.path" */
    const nlohmann::json* resolve(const std::string& key) const;
};

} // namespace ehr
