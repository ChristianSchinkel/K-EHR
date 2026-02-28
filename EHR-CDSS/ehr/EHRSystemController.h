#pragma once

#include <optional>
#include <string>
#include <vector>
#include "../models/Patient.h"
#include "../models/Encounter.h"
#include "../models/Observation.h"
#include "../models/Medication.h"
#include "../cdss/CDSSManager.h"

namespace ehr {

/**
 * @brief Top-level controller that orchestrates the EHR system components.
 *
 * Responsibilities:
 *  - Bootstrap sequence: config → database → CDSS
 *  - Patient management (save, retrieve)
 *  - Encounter / observation / medication management
 *  - Trigger CDSS inference and persist alerts
 */
class EHRSystemController {
public:
    EHRSystemController();
    ~EHRSystemController();

    EHRSystemController(const EHRSystemController&) = delete;
    EHRSystemController& operator=(const EHRSystemController&) = delete;

    /**
     * @brief Initialise all subsystems using the given config file.
     * @return true on success.
     */
    bool initialize(const std::string& configPath);

    /* ------------------------------------------------------------------ */
    /* Patient management                                                  */
    /* ------------------------------------------------------------------ */
    bool savePatient(Patient& patient);
    std::optional<Patient> getPatient(int id);
    std::vector<Patient>   getAllPatients();

    /* ------------------------------------------------------------------ */
    /* Encounter management                                                */
    /* ------------------------------------------------------------------ */
    bool saveEncounter(Encounter& encounter);
    std::vector<Encounter> getEncountersByPatient(int patientId);

    /* ------------------------------------------------------------------ */
    /* Observation management                                              */
    /* ------------------------------------------------------------------ */
    bool saveObservation(Observation& obs);

    /* ------------------------------------------------------------------ */
    /* Medication management                                               */
    /* ------------------------------------------------------------------ */
    bool saveMedication(Medication& med);
    std::vector<Medication> getMedicationsByPatient(int patientId);

    /* ------------------------------------------------------------------ */
    /* Clinical Decision Support                                           */
    /* ------------------------------------------------------------------ */
    /**
     * Run CDSS inference for a patient, persist alerts, and return them.
     */
    std::vector<ClinicalRecommendation> runCDSS(int patientId);

    /** Return persisted alerts for a patient. */
    std::vector<std::string> getAlerts(int patientId);

private:
    bool dbReady_   = false;
    bool cdssReady_ = false;
};

} // namespace ehr
