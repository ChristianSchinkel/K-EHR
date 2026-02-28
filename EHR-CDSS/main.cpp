/**
 * @file main.cpp
 * @brief EHR-CDSS system entry point.
 *
 * Demonstrates the full data-flow:
 *  1. Load configuration from config/config.json
 *  2. Initialise SQL database
 *  3. Save a patient record
 *  4. Save encounters, observations, and medications
 *  5. Run CDSS inference
 *  6. Store and display clinical alerts
 */

#include <iostream>
#include <string>

#include "ehr/EHRSystemController.h"
#include "models/Patient.h"
#include "models/Encounter.h"
#include "models/Observation.h"
#include "models/Medication.h"

int main() {
    std::cout << "========================================\n";
    std::cout << "  EHR-CDSS – Clinical Decision Support\n";
    std::cout << "========================================\n\n";

    ehr::EHRSystemController ctrl;

    /* ------------------------------------------------------------------
     * 1. Initialise subsystems
     * ------------------------------------------------------------------ */
    if (!ctrl.initialize("config/config.json")) {
        std::cerr << "[FATAL] System initialisation failed.\n";
        return 1;
    }

    /* ------------------------------------------------------------------
     * 2. Create and save a patient
     * ------------------------------------------------------------------ */
    ehr::Patient patient("John", "Doe", "1985-06-15", "male", "MRN-001");
    if (!ctrl.savePatient(patient)) {
        std::cerr << "[ERROR] Failed to save patient.\n";
        return 1;
    }
    std::cout << "Patient saved: " << patient.fullName()
              << " (id=" << patient.getId() << ")\n";

    /* ------------------------------------------------------------------
     * 3. Add an encounter
     * ------------------------------------------------------------------ */
    ehr::Encounter encounter(patient.getId(), "emergency",
                             "opioid withdrawal symptoms", "DR-42");
    if (!ctrl.saveEncounter(encounter)) {
        std::cerr << "[ERROR] Failed to save encounter.\n";
        return 1;
    }
    std::cout << "Encounter saved (id=" << encounter.getId() << ")\n";

    /* ------------------------------------------------------------------
     * 4. Add an observation to the encounter
     * ------------------------------------------------------------------ */
    ehr::Observation obs(encounter.getId(), "8480-6",
                         "Systolic blood pressure", "145", "mmHg");
    if (!ctrl.saveObservation(obs)) {
        std::cerr << "[ERROR] Failed to save observation.\n";
        return 1;
    }
    std::cout << "Observation saved: " << obs.description
              << " = " << obs.value << " " << obs.unit << "\n";

    /* ------------------------------------------------------------------
     * 5. Add medications
     * ------------------------------------------------------------------ */
    ehr::Medication med1(patient.getId(), "1049502", "oxycodone",
                         "5mg", "twice daily", "oral");
    ehr::Medication med2(patient.getId(), "2530", "diazepam",
                         "10mg", "once daily", "oral");
    ehr::Medication med3(patient.getId(), "6809", "metformin",
                         "500mg", "twice daily", "oral");
    ehr::Medication med4(patient.getId(), "41493", "lisinopril",
                         "10mg", "once daily", "oral");
    ehr::Medication med5(patient.getId(), "41493", "atorvastatin",
                         "20mg", "once daily", "oral");

    for (auto* med : { &med1, &med2, &med3, &med4, &med5 }) {
        if (!ctrl.saveMedication(*med)) {
            std::cerr << "[ERROR] Failed to save medication: " << med->name << "\n";
            return 1;
        }
        std::cout << "Medication saved: " << med->name << "\n";
    }

    /* ------------------------------------------------------------------
     * 6. Run CDSS inference
     * ------------------------------------------------------------------ */
    std::cout << "\n--- Running CDSS inference ---\n";
    auto recommendations = ctrl.runCDSS(patient.getId());

    if (recommendations.empty()) {
        std::cout << "No recommendations generated.\n";
    } else {
        std::cout << recommendations.size() << " recommendation(s):\n";
        for (const auto& rec : recommendations) {
            std::cout << "  [" << rec.severity << "] " << rec.type
                      << ": " << rec.message << "\n";
        }
    }

    /* ------------------------------------------------------------------
     * 7. Retrieve persisted alerts from the database
     * ------------------------------------------------------------------ */
    std::cout << "\n--- Persisted Clinical Alerts ---\n";
    auto alerts = ctrl.getAlerts(patient.getId());
    if (alerts.empty()) {
        std::cout << "No alerts stored.\n";
    } else {
        for (const auto& alert : alerts) {
            std::cout << "  " << alert << "\n";
        }
    }

    std::cout << "\n[DONE] EHR-CDSS demonstration complete.\n";
    return 0;
}
