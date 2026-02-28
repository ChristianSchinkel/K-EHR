#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "Observation.h"

namespace ehr {

/**
 * @brief Represents a clinical encounter (visit, admission, etc.)
 */
class Encounter {
public:
    Encounter() = default;
    Encounter(int patientId, std::string type,
              std::string reason, std::string providerId);

    /* Accessors */
    int               getId()         const { return id_; }
    int               getPatientId()  const { return patientId_; }
    const std::string& getType()       const { return type_; }
    const std::string& getReason()     const { return reason_; }
    const std::string& getProviderId() const { return providerId_; }
    const std::string& getNotes()      const { return notes_; }
    std::time_t        getDate()       const { return date_; }
    const std::vector<Observation>& getObservations() const { return observations_; }

    /* Mutators */
    void setId(int id)                           { id_         = id; }
    void setPatientId(int pid)                   { patientId_  = pid; }
    void setType(const std::string& v)           { type_       = v; }
    void setReason(const std::string& v)         { reason_     = v; }
    void setProviderId(const std::string& v)     { providerId_ = v; }
    void setNotes(const std::string& v)          { notes_      = v; }
    void setDate(std::time_t t)                  { date_       = t; }
    void addObservation(const Observation& obs)  { observations_.push_back(obs); }

private:
    int                     id_         = 0;
    int                     patientId_  = 0;
    std::string             type_;       ///< outpatient | inpatient | emergency
    std::string             reason_;
    std::string             providerId_;
    std::string             notes_;
    std::time_t             date_       = 0;
    std::vector<Observation> observations_;
};

} // namespace ehr
