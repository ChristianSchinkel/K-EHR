#pragma once

#include <string>
#include <vector>
#include "../models/Patient.h"
#include "../models/Encounter.h"
#include "../models/Medication.h"

namespace ehr {

/**
 * @brief Recommendation produced by the CDSS inference engine.
 */
struct ClinicalRecommendation {
    std::string type;        ///< risk | contraindication | relapse | recommendation
    std::string message;
    std::string severity;    ///< low | medium | high | critical
};

/**
 * @brief Manages CLIPS-based Clinical Decision Support.
 *
 * Workflow:
 *  1. Initialize CLIPS environment
 *  2. Load .clp rule files from the configured rules directory
 *  3. Receive patient/encounter/medication data
 *  4. Assert facts into CLIPS
 *  5. Run inference (Run())
 *  6. Collect and return recommendations
 *
 * When CLIPS_AVAILABLE is not defined (build without the real CLIPS library),
 * the engine falls back to a built-in rule set implemented in C++.
 */
class CDSSManager {
public:
    CDSSManager();
    ~CDSSManager();

    CDSSManager(const CDSSManager&) = delete;
    CDSSManager& operator=(const CDSSManager&) = delete;

    /** Initialise the CLIPS environment and load rules from rulesDir. */
    bool initialize(const std::string& rulesDir);

    /** Run inference for a patient and return recommendations. */
    std::vector<ClinicalRecommendation> analyze(
        const Patient&                  patient,
        const std::vector<Encounter>&   encounters,
        const std::vector<Medication>&  medications);

    bool isInitialized() const { return initialized_; }

private:
    bool        initialized_ = false;
    std::string rulesDir_;

    /* CLIPS environment pointer (void* to avoid including clips.h in header) */
    void* clipsEnv_ = nullptr;

    /* Helper – assert a CLIPS fact string */
    void assertFact(const std::string& factStr);

    /* Built-in fallback rules (used when CLIPS is unavailable) */
    std::vector<ClinicalRecommendation> runBuiltinRules(
        const Patient&                 patient,
        const std::vector<Encounter>&  encounters,
        const std::vector<Medication>& medications);

    static constexpr double kSecondsPerDay = 86400.0;
    static constexpr double kDaysPerYear   = 365.0;
};

} // namespace ehr
