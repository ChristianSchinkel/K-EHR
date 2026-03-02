#include "CDSSManager.h"
#include "../utils/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>

/* -------------------------------------------------------------------------
 * Optional CLIPS integration.
 * Define CLIPS_AVAILABLE at compile time to enable the real CLIPS engine.
 * Without it, the built-in fallback rule set is used.
 * ------------------------------------------------------------------------- */
#ifdef CLIPS_AVAILABLE
#  include "../third_party/clips/clips.h"
#endif

namespace ehr {

/* =========================================================================
 * Utility helpers
 * ======================================================================= */

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool contains(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

/* =========================================================================
 * CDSSManager implementation
 * ======================================================================= */

CDSSManager::CDSSManager() = default;

CDSSManager::~CDSSManager() {
#ifdef CLIPS_AVAILABLE
    if (clipsEnv_) {
        DestroyEnvironment(static_cast<Environment>(clipsEnv_));
        clipsEnv_ = nullptr;
    }
#endif
}

bool CDSSManager::initialize(const std::string& rulesDir) {
    rulesDir_ = rulesDir;

#ifdef CLIPS_AVAILABLE
    clipsEnv_ = CreateEnvironment();
    if (!clipsEnv_) {
        LOG_ERROR("CDSSManager: failed to create CLIPS environment.");
        return false;
    }

    /* Load all .clp files found in rulesDir */
    namespace fs = std::filesystem;
    try {
        for (const auto& entry : fs::directory_iterator(rulesDir_)) {
            if (entry.path().extension() == ".clp") {
                int rc = Load(static_cast<Environment>(clipsEnv_),
                              entry.path().string().c_str());
                if (rc != 1) {
                    LOG_WARN("CDSSManager: could not load rule file: " + entry.path().string());
                } else {
                    LOG_INFO("CDSSManager: loaded rule file: " + entry.path().string());
                }
            }
        }
    } catch (const fs::filesystem_error& ex) {
        LOG_WARN(std::string("CDSSManager: rule directory error: ") + ex.what());
    }

    initialized_ = true;
    LOG_INFO("CDSSManager: CLIPS engine initialised (real CLIPS).");
#else
    initialized_ = true;
    LOG_INFO("CDSSManager: initialised with built-in fallback rule engine "
             "(CLIPS_AVAILABLE not defined).");
#endif
    return true;
}

void CDSSManager::assertFact(const std::string& factStr) {
#ifdef CLIPS_AVAILABLE
    if (clipsEnv_) {
        AssertString(static_cast<Environment>(clipsEnv_), factStr.c_str());
    }
#else
    (void)factStr;
#endif
}

std::vector<ClinicalRecommendation> CDSSManager::analyze(
    const Patient&                  patient,
    const std::vector<Encounter>&   encounters,
    const std::vector<Medication>&  medications)
{
    if (!initialized_) {
        LOG_WARN("CDSSManager::analyze called before initialize().");
        return {};
    }

#ifdef CLIPS_AVAILABLE
    /* Reset fact base */
    Reset(static_cast<Environment>(clipsEnv_));

    /* Assert patient fact */
    {
        std::ostringstream ss;
        ss << "(patient (id " << patient.getId()
           << ") (first-name \"" << patient.getFirstName()
           << "\") (last-name \""  << patient.getLastName()
           << "\") (dob \""        << patient.getDob()
           << "\") (gender \""     << patient.getGender() << "\"))";
        assertFact(ss.str());
    }

    /* Assert encounter facts */
    for (const auto& enc : encounters) {
        std::ostringstream ss;
        ss << "(encounter (id " << enc.getId()
           << ") (patient-id "  << enc.getPatientId()
           << ") (type \""      << enc.getType()
           << "\") (reason \""  << enc.getReason() << "\"))";
        assertFact(ss.str());
    }

    /* Assert medication facts */
    for (const auto& med : medications) {
        std::ostringstream ss;
        ss << "(medication (patient-id " << med.patientId
           << ") (name \""               << med.name
           << "\") (status \""           << med.status << "\"))";
        assertFact(ss.str());
    }

    /* Run the inference engine */
    Run(static_cast<Environment>(clipsEnv_), -1LL);

    /* TODO: retrieve recommendations from CLIPS fact base */
    LOG_INFO("CDSSManager: CLIPS inference complete.");
    return {};   /* extend when collecting CLIPS output facts */
#else
    return runBuiltinRules(patient, encounters, medications);
#endif
}

/* =========================================================================
 * Built-in fallback rule engine
 * ======================================================================= */

std::vector<ClinicalRecommendation> CDSSManager::runBuiltinRules(
    const Patient&                 patient,
    const std::vector<Encounter>&  encounters,
    const std::vector<Medication>& medications)
{
    std::vector<ClinicalRecommendation> recs;

    /* ---- Rule 1: Polypharmacy risk ----------------------------------------
     * Fire when a patient has 5 or more active medications.
     * ---------------------------------------------------------------------- */
    int activeMeds = 0;
    for (const auto& m : medications) {
        if (toLower(m.status) == "active") ++activeMeds;
    }
    if (activeMeds >= 5) {
        recs.push_back({
            "risk",
            "Polypharmacy alert: patient has " + std::to_string(activeMeds) +
                " active medications. Review for interactions.",
            "high"
        });
    }

    /* ---- Rule 2: Frequent emergency encounters ----------------------------
     * Fire when patient has ≥ 3 emergency encounters → relapse risk.
     * ---------------------------------------------------------------------- */
    int emergencyCount = 0;
    for (const auto& enc : encounters) {
        if (contains(enc.getType(), "emergency")) ++emergencyCount;
    }
    if (emergencyCount >= 3) {
        recs.push_back({
            "relapse",
            "Relapse risk indicator: " + std::to_string(emergencyCount) +
                " emergency visits recorded. Consider intensive follow-up.",
            "high"
        });
    }

    /* ---- Rule 3: Addiction-related encounter reason -----------------------
     * Flag encounters with substance-use related reasons.
     * ---------------------------------------------------------------------- */
    for (const auto& enc : encounters) {
        const std::string& reason = enc.getReason();
        if (contains(reason, "addiction") ||
            contains(reason, "substance") ||
            contains(reason, "withdrawal") ||
            contains(reason, "overdose"))
        {
            recs.push_back({
                "risk",
                "Substance use disorder encounter detected: \"" + reason +
                    "\". Ensure CDSS addiction protocol is applied.",
                "medium"
            });
            break; /* one alert per patient is enough */
        }
    }

    /* ---- Rule 4: Opioid + benzodiazepine co-prescription -----------------
     * Contraindication: concurrent opioid and benzodiazepine use.
     * ---------------------------------------------------------------------- */
    bool hasOpioid = false, hasBenzo = false;
    for (const auto& m : medications) {
        if (toLower(m.status) != "active") continue;
        const std::string nm = toLower(m.name);
        if (nm.find("morphine") != std::string::npos ||
            nm.find("oxycodone") != std::string::npos ||
            nm.find("hydrocodone") != std::string::npos ||
            nm.find("fentanyl") != std::string::npos ||
            nm.find("codeine") != std::string::npos  ||
            nm.find("tramadol") != std::string::npos)
        {
            hasOpioid = true;
        }
        if (nm.find("diazepam") != std::string::npos   ||
            nm.find("lorazepam") != std::string::npos  ||
            nm.find("alprazolam") != std::string::npos ||
            nm.find("clonazepam") != std::string::npos ||
            nm.find("midazolam") != std::string::npos)
        {
            hasBenzo = true;
        }
    }
    if (hasOpioid && hasBenzo) {
        recs.push_back({
            "contraindication",
            "Opioid + benzodiazepine co-prescription detected. "
            "High risk of respiratory depression. Clinical review required.",
            "critical"
        });
    }

    /* ---- Rule 5: Missing follow-up recommendation -------------------------
     * Recommend follow-up when no encounter in the last 12 months.
     * ---------------------------------------------------------------------- */
    if (!encounters.empty()) {
        std::time_t mostRecent = 0;
        for (const auto& enc : encounters) {
            if (enc.getDate() > mostRecent) mostRecent = enc.getDate();
        }
        double daysSince = std::difftime(std::time(nullptr), mostRecent) / kSecondsPerDay;
        if (daysSince > kDaysPerYear) {
            recs.push_back({
                "recommendation",
                "No encounter recorded in the past 12 months. "
                "Schedule follow-up appointment.",
                "low"
            });
        }
    }

    LOG_INFO("CDSSManager: built-in rules produced " +
             std::to_string(recs.size()) + " recommendation(s) for patient " +
             std::to_string(patient.getId()));

    return recs;
}

} // namespace ehr
