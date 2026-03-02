#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../models/Patient.h"
#include "../models/Encounter.h"
#include "../models/Observation.h"
#include "../models/Medication.h"

struct sqlite3;

namespace ehr {

/**
 * @brief Manages all SQLite database operations for the EHR system.
 *
 * Responsible for:
 *  - Opening / closing the database connection
 *  - Executing the schema (CREATE TABLE statements)
 *  - CRUD operations for Patients, Encounters, Observations,
 *    Medications, and ClinicalAlerts
 */
class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    /* Prevent copy */
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /** Open (or create) the SQLite database at the given path. */
    bool open(const std::string& dbPath);

    /** Close the database connection. */
    void close();

    /** Apply the initial schema (idempotent – uses CREATE TABLE IF NOT EXISTS). */
    bool applySchema();

    /* ------------------------------------------------------------------ */
    /* Patient CRUD                                                        */
    /* ------------------------------------------------------------------ */
    bool                    savePatient(Patient& patient);
    std::optional<Patient>  getPatient(int id);
    std::vector<Patient>    getAllPatients();
    bool                    deletePatient(int id);

    /* ------------------------------------------------------------------ */
    /* Encounter CRUD                                                      */
    /* ------------------------------------------------------------------ */
    bool                      saveEncounter(Encounter& encounter);
    std::optional<Encounter>  getEncounter(int id);
    std::vector<Encounter>    getEncountersByPatient(int patientId);

    /* ------------------------------------------------------------------ */
    /* Observation CRUD                                                    */
    /* ------------------------------------------------------------------ */
    bool                        saveObservation(Observation& obs);
    std::vector<Observation>    getObservationsByEncounter(int encounterId);

    /* ------------------------------------------------------------------ */
    /* Medication CRUD                                                     */
    /* ------------------------------------------------------------------ */
    bool                      saveMedication(Medication& med);
    std::vector<Medication>   getMedicationsByPatient(int patientId);

    /* ------------------------------------------------------------------ */
    /* Clinical Alerts                                                     */
    /* ------------------------------------------------------------------ */
    bool saveClinicalAlert(int patientId, const std::string& alertType,
                           const std::string& description, const std::string& severity);
    std::vector<std::string> getClinicalAlerts(int patientId);

    bool isOpen() const { return db_ != nullptr; }

private:
    sqlite3* db_ = nullptr;

    bool     exec(const std::string& sql);
    int      lastInsertRowId();
};

} // namespace ehr
