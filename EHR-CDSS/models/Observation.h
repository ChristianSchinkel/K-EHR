#pragma once

#include <string>
#include <vector>
#include <ctime>

namespace ehr {

/**
 * @brief Represents a clinical observation (vital sign, lab result, etc.)
 */
struct Observation {
    int         id          = 0;
    int         encounterId = 0;
    std::string code;        ///< LOINC or local code
    std::string description;
    std::string value;
    std::string unit;
    std::time_t recordedAt  = 0;

    Observation() = default;
    Observation(int encId, std::string code_, std::string desc,
                std::string val, std::string unit_)
        : encounterId(encId)
        , code(std::move(code_))
        , description(std::move(desc))
        , value(std::move(val))
        , unit(std::move(unit_))
        , recordedAt(std::time(nullptr))
    {}
};

} // namespace ehr
