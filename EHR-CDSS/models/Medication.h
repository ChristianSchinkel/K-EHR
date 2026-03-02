#pragma once

#include <string>
#include <ctime>

namespace ehr {

/**
 * @brief Represents a medication order / prescription.
 */
struct Medication {
    int         id          = 0;
    int         patientId   = 0;
    std::string rxCode;      ///< RxNorm or local code
    std::string name;
    std::string dosage;
    std::string frequency;
    std::string route;
    std::string status;      ///< active | stopped | completed
    std::time_t startDate   = 0;
    std::time_t endDate     = 0;

    Medication() = default;
    Medication(int pid, std::string code, std::string n,
               std::string dose, std::string freq, std::string rt)
        : patientId(pid)
        , rxCode(std::move(code))
        , name(std::move(n))
        , dosage(std::move(dose))
        , frequency(std::move(freq))
        , route(std::move(rt))
        , status("active")
        , startDate(std::time(nullptr))
    {}
};

} // namespace ehr
