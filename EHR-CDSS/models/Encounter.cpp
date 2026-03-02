#include "Encounter.h"
#include <ctime>

namespace ehr {

Encounter::Encounter(int patientId, std::string type,
                     std::string reason, std::string providerId)
    : patientId_(patientId)
    , type_(std::move(type))
    , reason_(std::move(reason))
    , providerId_(std::move(providerId))
    , date_(std::time(nullptr))
{}

} // namespace ehr
