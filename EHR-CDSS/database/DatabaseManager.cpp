#include "DatabaseManager.h"
#include "../utils/Logger.h"
#include <sqlite3.h>
#include <sstream>

namespace ehr {

/* =========================================================================
 * Helpers
 * ======================================================================= */

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::exec(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("DatabaseManager SQL error: " + std::string(errMsg ? errMsg : "unknown"));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

int DatabaseManager::lastInsertRowId() {
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

/* =========================================================================
 * Connection management
 * ======================================================================= */

bool DatabaseManager::open(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        LOG_ERROR("DatabaseManager: cannot open database: " + dbPath);
        db_ = nullptr;
        return false;
    }
    exec("PRAGMA foreign_keys = ON;");
    exec("PRAGMA journal_mode = WAL;");
    LOG_INFO("DatabaseManager: opened database at " + dbPath);
    return true;
}

void DatabaseManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        LOG_INFO("DatabaseManager: database closed.");
    }
}

/* =========================================================================
 * Schema
 * ======================================================================= */

bool DatabaseManager::applySchema() {
    const std::string schema = R"SQL(
        CREATE TABLE IF NOT EXISTS Patients (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            firstName  TEXT NOT NULL,
            lastName   TEXT NOT NULL,
            dob        TEXT NOT NULL,
            gender     TEXT NOT NULL,
            mrn        TEXT UNIQUE NOT NULL
        );

        CREATE TABLE IF NOT EXISTS Encounters (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            patientId   INTEGER NOT NULL REFERENCES Patients(id) ON DELETE CASCADE,
            type        TEXT NOT NULL,
            reason      TEXT,
            providerId  TEXT,
            notes       TEXT,
            date        INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS Observations (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            encounterId  INTEGER NOT NULL REFERENCES Encounters(id) ON DELETE CASCADE,
            code         TEXT NOT NULL,
            description  TEXT,
            value        TEXT NOT NULL,
            unit         TEXT,
            recordedAt   INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS Medications (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            patientId   INTEGER NOT NULL REFERENCES Patients(id) ON DELETE CASCADE,
            rxCode      TEXT NOT NULL,
            name        TEXT NOT NULL,
            dosage      TEXT,
            frequency   TEXT,
            route       TEXT,
            status      TEXT DEFAULT 'active',
            startDate   INTEGER NOT NULL,
            endDate     INTEGER
        );

        CREATE TABLE IF NOT EXISTS ClinicalAlerts (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            patientId   INTEGER NOT NULL REFERENCES Patients(id) ON DELETE CASCADE,
            alertType   TEXT NOT NULL,
            description TEXT NOT NULL,
            severity    TEXT NOT NULL,
            createdAt   INTEGER NOT NULL
        );
    )SQL";

    return exec(schema);
}

/* =========================================================================
 * Patient CRUD – uses parameterised statements to prevent SQL injection
 * ======================================================================= */

bool DatabaseManager::savePatient(Patient& patient) {
    sqlite3_stmt* stmt = nullptr;
    if (patient.getId() == 0) {
        const char* sql =
            "INSERT INTO Patients (firstName, lastName, dob, gender, mrn) "
            "VALUES (?, ?, ?, ?, ?);";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, patient.getFirstName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, patient.getLastName().c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, patient.getDob().c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, patient.getGender().c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, patient.getMrn().c_str(),       -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            LOG_ERROR("DatabaseManager: failed to insert patient.");
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        patient.setId(lastInsertRowId());
        LOG_INFO("Patient saved with id=" + std::to_string(patient.getId()));
    } else {
        const char* sql =
            "UPDATE Patients SET firstName=?, lastName=?, dob=?, gender=?, mrn=? "
            "WHERE id=?;";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, patient.getFirstName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, patient.getLastName().c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, patient.getDob().c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, patient.getGender().c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, patient.getMrn().c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (stmt, 6, patient.getId());
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            LOG_ERROR("DatabaseManager: failed to update patient.");
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        LOG_INFO("Patient updated id=" + std::to_string(patient.getId()));
    }
    return true;
}

std::optional<Patient> DatabaseManager::getPatient(int id) {
    const char* sql =
        "SELECT id,firstName,lastName,dob,gender,mrn FROM Patients WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(stmt, 1, id);
    std::optional<Patient> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Patient p;
        p.setId(sqlite3_column_int(stmt, 0));
        p.setFirstName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        p.setLastName (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        p.setDob      (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        p.setGender   (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        p.setMrn      (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        result = p;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Patient> DatabaseManager::getAllPatients() {
    std::vector<Patient> patients;
    const char* sql = "SELECT id,firstName,lastName,dob,gender,mrn FROM Patients;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return patients;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Patient p;
        p.setId(sqlite3_column_int(stmt, 0));
        p.setFirstName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        p.setLastName (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        p.setDob      (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        p.setGender   (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        p.setMrn      (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        patients.push_back(p);
    }
    sqlite3_finalize(stmt);
    return patients;
}

bool DatabaseManager::deletePatient(int id) {
    const char* sql = "DELETE FROM Patients WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

/* =========================================================================
 * Encounter CRUD
 * ======================================================================= */

bool DatabaseManager::saveEncounter(Encounter& encounter) {
    sqlite3_stmt* stmt = nullptr;
    if (encounter.getId() == 0) {
        const char* sql =
            "INSERT INTO Encounters (patientId,type,reason,providerId,notes,date) "
            "VALUES (?,?,?,?,?,?);";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int  (stmt, 1, encounter.getPatientId());
        sqlite3_bind_text (stmt, 2, encounter.getType().c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 3, encounter.getReason().c_str(),     -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 4, encounter.getProviderId().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 5, encounter.getNotes().c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 6, static_cast<sqlite3_int64>(encounter.getDate()));
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            LOG_ERROR("DatabaseManager: failed to insert encounter.");
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        encounter.setId(lastInsertRowId());
        LOG_INFO("Encounter saved with id=" + std::to_string(encounter.getId()));
    } else {
        const char* sql =
            "UPDATE Encounters SET type=?,reason=?,providerId=?,notes=?,date=? WHERE id=?;";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text (stmt, 1, encounter.getType().c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 2, encounter.getReason().c_str(),     -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 3, encounter.getProviderId().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 4, encounter.getNotes().c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(encounter.getDate()));
        sqlite3_bind_int  (stmt, 6, encounter.getId());
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            LOG_ERROR("DatabaseManager: failed to update encounter.");
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    return true;
}

std::optional<Encounter> DatabaseManager::getEncounter(int id) {
    const char* sql =
        "SELECT id,patientId,type,reason,providerId,notes,date "
        "FROM Encounters WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(stmt, 1, id);
    std::optional<Encounter> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Encounter e;
        e.setId        (sqlite3_column_int(stmt, 0));
        e.setPatientId (sqlite3_column_int(stmt, 1));
        e.setType      (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        e.setReason    (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        e.setProviderId(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        e.setNotes     (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        e.setDate      (static_cast<std::time_t>(sqlite3_column_int64(stmt, 6)));
        result = e;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Encounter> DatabaseManager::getEncountersByPatient(int patientId) {
    std::vector<Encounter> encounters;
    const char* sql =
        "SELECT id,patientId,type,reason,providerId,notes,date "
        "FROM Encounters WHERE patientId=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return encounters;
    sqlite3_bind_int(stmt, 1, patientId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Encounter e;
        e.setId        (sqlite3_column_int(stmt, 0));
        e.setPatientId (sqlite3_column_int(stmt, 1));
        e.setType      (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        e.setReason    (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        e.setProviderId(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        e.setNotes     (reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        e.setDate      (static_cast<std::time_t>(sqlite3_column_int64(stmt, 6)));
        encounters.push_back(e);
    }
    sqlite3_finalize(stmt);
    return encounters;
}

/* =========================================================================
 * Observation CRUD
 * ======================================================================= */

bool DatabaseManager::saveObservation(Observation& obs) {
    const char* sql =
        "INSERT INTO Observations (encounterId,code,description,value,unit,recordedAt) "
        "VALUES (?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int  (stmt, 1, obs.encounterId);
    sqlite3_bind_text (stmt, 2, obs.code.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, obs.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, obs.value.c_str(),       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, obs.unit.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, static_cast<sqlite3_int64>(obs.recordedAt));
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        LOG_ERROR("DatabaseManager: failed to insert observation.");
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    obs.id = lastInsertRowId();
    return true;
}

std::vector<Observation> DatabaseManager::getObservationsByEncounter(int encounterId) {
    std::vector<Observation> obs;
    const char* sql =
        "SELECT id,encounterId,code,description,value,unit,recordedAt "
        "FROM Observations WHERE encounterId=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return obs;
    sqlite3_bind_int(stmt, 1, encounterId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Observation o;
        o.id          = sqlite3_column_int(stmt, 0);
        o.encounterId = sqlite3_column_int(stmt, 1);
        o.code        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        o.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        o.value       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        o.unit        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        o.recordedAt  = static_cast<std::time_t>(sqlite3_column_int64(stmt, 6));
        obs.push_back(o);
    }
    sqlite3_finalize(stmt);
    return obs;
}

/* =========================================================================
 * Medication CRUD
 * ======================================================================= */

bool DatabaseManager::saveMedication(Medication& med) {
    sqlite3_stmt* stmt = nullptr;
    if (med.id == 0) {
        const char* sql =
            "INSERT INTO Medications "
            "(patientId,rxCode,name,dosage,frequency,route,status,startDate) "
            "VALUES (?,?,?,?,?,?,?,?);";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int  (stmt, 1, med.patientId);
        sqlite3_bind_text (stmt, 2, med.rxCode.c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 3, med.name.c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 4, med.dosage.c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 5, med.frequency.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 6, med.route.c_str(),     -1, SQLITE_TRANSIENT);
        sqlite3_bind_text (stmt, 7, med.status.c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 8, static_cast<sqlite3_int64>(med.startDate));
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            LOG_ERROR("DatabaseManager: failed to insert medication.");
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        med.id = lastInsertRowId();
        LOG_INFO("Medication saved id=" + std::to_string(med.id));
    } else {
        const char* sql =
            "UPDATE Medications SET status=?, endDate=? WHERE id=?;";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text (stmt, 1, med.status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(med.endDate));
        sqlite3_bind_int  (stmt, 3, med.id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            LOG_ERROR("DatabaseManager: failed to update medication.");
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    return true;
}

std::vector<Medication> DatabaseManager::getMedicationsByPatient(int patientId) {
    std::vector<Medication> meds;
    const char* sql =
        "SELECT id,patientId,rxCode,name,dosage,frequency,route,status,startDate,endDate "
        "FROM Medications WHERE patientId=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return meds;
    sqlite3_bind_int(stmt, 1, patientId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Medication m;
        m.id          = sqlite3_column_int(stmt, 0);
        m.patientId   = sqlite3_column_int(stmt, 1);
        m.rxCode      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        m.name        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        m.dosage      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        m.frequency   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        m.route       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        m.status      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        m.startDate   = static_cast<std::time_t>(sqlite3_column_int64(stmt, 8));
        m.endDate     = static_cast<std::time_t>(sqlite3_column_int64(stmt, 9));
        meds.push_back(m);
    }
    sqlite3_finalize(stmt);
    return meds;
}

/* =========================================================================
 * Clinical Alerts
 * ======================================================================= */

bool DatabaseManager::saveClinicalAlert(int patientId,
                                        const std::string& alertType,
                                        const std::string& description,
                                        const std::string& severity) {
    const char* sql =
        "INSERT INTO ClinicalAlerts (patientId,alertType,description,severity,createdAt) "
        "VALUES (?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int  (stmt, 1, patientId);
    sqlite3_bind_text (stmt, 2, alertType.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, description.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, severity.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(std::time(nullptr)));
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) LOG_ERROR("DatabaseManager: failed to insert clinical alert.");
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<std::string> DatabaseManager::getClinicalAlerts(int patientId) {
    std::vector<std::string> alerts;
    const char* sql =
        "SELECT alertType, description, severity FROM ClinicalAlerts "
        "WHERE patientId=? ORDER BY createdAt DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return alerts;
    sqlite3_bind_int(stmt, 1, patientId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string entry =
            "[" + std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))) + "] " +
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))) + ": " +
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        alerts.push_back(entry);
    }
    sqlite3_finalize(stmt);
    return alerts;
}

} // namespace ehr
