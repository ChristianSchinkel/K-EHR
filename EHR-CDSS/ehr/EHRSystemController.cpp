#include "EHRSystemController.h"
#include "../config_manager/ConfigManager.h"
#include "../database/DatabaseManager.h"
#include "../cdss/CDSSManager.h"
#include "../utils/Logger.h"

namespace ehr {

/* =========================================================================
 * Module-level singletons owned by EHRSystemController
 * (kept as static locals to guarantee correct destruction order)
 * ======================================================================= */
static DatabaseManager& db() {
    static DatabaseManager instance;
    return instance;
}

static CDSSManager& cdss() {
    static CDSSManager instance;
    return instance;
}

/* =========================================================================
 * Lifecycle
 * ======================================================================= */

EHRSystemController::EHRSystemController() = default;
EHRSystemController::~EHRSystemController() { db().close(); }

bool EHRSystemController::initialize(const std::string& configPath) {
    /* 1. Load configuration */
    auto& cfg = ConfigManager::instance();
    if (!cfg.load(configPath)) {
        LOG_WARN("EHRSystemController: could not load config; using defaults.");
    }

    /* 2. Configure logger */
    const std::string lvl = cfg.logLevel();
    if      (lvl == "DEBUG") Logger::instance().setLevel(Logger::Level::DEBUG);
    else if (lvl == "WARN")  Logger::instance().setLevel(Logger::Level::WARN);
    else if (lvl == "ERROR") Logger::instance().setLevel(Logger::Level::ERR);
    else                     Logger::instance().setLevel(Logger::Level::INFO);

    const std::string logFile = cfg.logFile();
    if (!logFile.empty()) Logger::instance().setLogFile(logFile);

    /* 3. Open and initialise database */
    if (!db().open(cfg.databasePath())) {
        LOG_ERROR("EHRSystemController: database open failed.");
        return false;
    }
    if (!db().applySchema()) {
        LOG_ERROR("EHRSystemController: schema application failed.");
        return false;
    }
    dbReady_ = true;

    /* 4. Initialise CDSS (optional – system runs without it) */
    if (cfg.cdssEnabled()) {
        if (cdss().initialize(cfg.cdssRulesPath())) {
            cdssReady_ = true;
        } else {
            LOG_WARN("EHRSystemController: CDSS initialisation failed; "
                     "CDS features will be unavailable.");
        }
    } else {
        LOG_INFO("EHRSystemController: CDSS disabled by configuration.");
    }

    LOG_INFO("EHRSystemController: initialisation complete.");
    return true;
}

/* =========================================================================
 * Patient management
 * ======================================================================= */

bool EHRSystemController::savePatient(Patient& patient) {
    return db().savePatient(patient);
}

std::optional<Patient> EHRSystemController::getPatient(int id) {
    return db().getPatient(id);
}

std::vector<Patient> EHRSystemController::getAllPatients() {
    return db().getAllPatients();
}

/* =========================================================================
 * Encounter management
 * ======================================================================= */

bool EHRSystemController::saveEncounter(Encounter& encounter) {
    return db().saveEncounter(encounter);
}

std::vector<Encounter> EHRSystemController::getEncountersByPatient(int patientId) {
    return db().getEncountersByPatient(patientId);
}

/* =========================================================================
 * Observation management
 * ======================================================================= */

bool EHRSystemController::saveObservation(Observation& obs) {
    return db().saveObservation(obs);
}

/* =========================================================================
 * Medication management
 * ======================================================================= */

bool EHRSystemController::saveMedication(Medication& med) {
    return db().saveMedication(med);
}

std::vector<Medication> EHRSystemController::getMedicationsByPatient(int patientId) {
    return db().getMedicationsByPatient(patientId);
}

/* =========================================================================
 * CDSS
 * ======================================================================= */

std::vector<ClinicalRecommendation> EHRSystemController::runCDSS(int patientId) {
    if (!cdssReady_) {
        LOG_WARN("EHRSystemController::runCDSS: CDSS not ready.");
        return {};
    }

    auto patientOpt = db().getPatient(patientId);
    if (!patientOpt) {
        LOG_ERROR("EHRSystemController::runCDSS: patient not found id=" +
                  std::to_string(patientId));
        return {};
    }

    auto encounters  = db().getEncountersByPatient(patientId);
    auto medications = db().getMedicationsByPatient(patientId);

    auto recs = cdss().analyze(*patientOpt, encounters, medications);

    /* Persist alerts */
    for (const auto& rec : recs) {
        db().saveClinicalAlert(patientId, rec.type, rec.message, rec.severity);
    }

    return recs;
}

std::vector<std::string> EHRSystemController::getAlerts(int patientId) {
    return db().getClinicalAlerts(patientId);
}

} // namespace ehr
