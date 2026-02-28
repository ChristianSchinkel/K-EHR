;;; =============================================================================
;;; EHR-CDSS – Addiction & Substance-Use Clinical Decision Support Rules
;;; CLIPS 6.4 rule file
;;;
;;; Fact templates expected from CDSSManager::analyze():
;;;
;;;   (patient (id ?id) (first-name ?fn) (last-name ?ln)
;;;            (dob ?dob) (gender ?g))
;;;
;;;   (encounter (id ?eid) (patient-id ?pid)
;;;              (type ?t) (reason ?r))
;;;
;;;   (medication (patient-id ?pid) (name ?name) (status ?s))
;;;
;;; Recommendations are asserted as:
;;;   (recommendation (patient-id ?pid) (type ?type)
;;;                   (message ?msg) (severity ?sev))
;;; =============================================================================

;;; ---------------------------------------------------------------------------
;;; Deftemplate declarations
;;; ---------------------------------------------------------------------------

(deftemplate patient
    (slot id          (type INTEGER))
    (slot first-name  (type STRING))
    (slot last-name   (type STRING))
    (slot dob         (type STRING))
    (slot gender      (type STRING)))

(deftemplate encounter
    (slot id         (type INTEGER))
    (slot patient-id (type INTEGER))
    (slot type       (type STRING))
    (slot reason     (type STRING)))

(deftemplate medication
    (slot patient-id (type INTEGER))
    (slot name       (type STRING))
    (slot status     (type STRING)))

(deftemplate recommendation
    (slot patient-id (type INTEGER))
    (slot type       (type STRING))
    (slot message    (type STRING))
    (slot severity   (type STRING)))

;;; ---------------------------------------------------------------------------
;;; Rule 1 – Substance-use related encounter
;;; ---------------------------------------------------------------------------
(defrule substance-use-encounter
    "Flag any encounter with an addiction or substance-use related reason."
    (encounter (patient-id ?pid)
               (reason ?r&:(or (str-index "addiction"  ?r)
                               (str-index "substance"  ?r)
                               (str-index "withdrawal" ?r)
                               (str-index "overdose"   ?r))))
    (not (recommendation (patient-id ?pid) (type "risk")))
=>
    (assert (recommendation
        (patient-id ?pid)
        (type "risk")
        (message (str-cat "Substance use disorder encounter: " ?r
                          ". Apply addiction care protocol."))
        (severity "medium"))))

;;; ---------------------------------------------------------------------------
;;; Rule 2 – Relapse risk: multiple emergency visits
;;; ---------------------------------------------------------------------------
(defrule relapse-risk-emergency
    "Three or more emergency encounters indicates relapse risk."
    (encounter (patient-id ?pid) (type "emergency"))
    (encounter (patient-id ?pid) (type "emergency") (id ?id2))
    (encounter (patient-id ?pid) (type "emergency") (id ?id3&:(> ?id3 ?id2)))
    (not (recommendation (patient-id ?pid) (type "relapse")))
=>
    (assert (recommendation
        (patient-id ?pid)
        (type "relapse")
        (message "Relapse risk: 3+ emergency visits. Intensify care management.")
        (severity "high"))))

;;; ---------------------------------------------------------------------------
;;; Rule 3 – Opioid + benzodiazepine contraindication
;;; ---------------------------------------------------------------------------
(defrule opioid-benzo-contraindication
    "Concurrent opioid and benzodiazepine prescriptions – high respiratory risk."
    (medication (patient-id ?pid) (status "active")
                (name ?opioid&:(or (str-index "morphine"    ?opioid)
                                   (str-index "oxycodone"   ?opioid)
                                   (str-index "hydrocodone" ?opioid)
                                   (str-index "fentanyl"    ?opioid)
                                   (str-index "tramadol"    ?opioid)
                                   (str-index "codeine"     ?opioid))))
    (medication (patient-id ?pid) (status "active")
                (name ?benzo&:(or (str-index "diazepam"    ?benzo)
                                  (str-index "lorazepam"   ?benzo)
                                  (str-index "alprazolam"  ?benzo)
                                  (str-index "clonazepam"  ?benzo)
                                  (str-index "midazolam"   ?benzo))))
    (not (recommendation (patient-id ?pid) (type "contraindication")))
=>
    (assert (recommendation
        (patient-id ?pid)
        (type "contraindication")
        (message (str-cat "Opioid (" ?opioid ") + benzodiazepine (" ?benzo
                          ") co-prescription. High respiratory depression risk."))
        (severity "critical"))))

;;; ---------------------------------------------------------------------------
;;; Rule 4 – Polypharmacy risk (≥ 5 active medications)
;;; ---------------------------------------------------------------------------
(defrule polypharmacy-risk
    "Five or more concurrent active medications – review for interactions."
    (medication (patient-id ?pid) (status "active") (name ?m1))
    (medication (patient-id ?pid) (status "active") (name ?m2&:(neq ?m2 ?m1)))
    (medication (patient-id ?pid) (status "active") (name ?m3&:(and (neq ?m3 ?m1)(neq ?m3 ?m2))))
    (medication (patient-id ?pid) (status "active") (name ?m4&:(and (neq ?m4 ?m1)(neq ?m4 ?m2)(neq ?m4 ?m3))))
    (medication (patient-id ?pid) (status "active") (name ?m5&:(and (neq ?m5 ?m1)(neq ?m5 ?m2)(neq ?m5 ?m3)(neq ?m5 ?m4))))
    (not (recommendation (patient-id ?pid) (type "risk") (message ?x&:(str-index "Polypharmacy" ?x))))
=>
    (assert (recommendation
        (patient-id ?pid)
        (type "risk")
        (message "Polypharmacy: 5+ active medications detected. Review for drug interactions.")
        (severity "high"))))

;;; ---------------------------------------------------------------------------
;;; Rule 5 – Methadone treatment recommendation for opioid dependence
;;; ---------------------------------------------------------------------------
(defrule methadone-recommendation
    "Recommend methadone/MAT when opioid dependence encounter is present and
     no maintenance therapy (methadone/buprenorphine) is currently prescribed."
    (encounter  (patient-id ?pid) (reason ?r&:(str-index "opioid" ?r)))
    (not (medication (patient-id ?pid) (status "active")
                     (name ?n&:(or (str-index "methadone"     ?n)
                                   (str-index "buprenorphine" ?n)
                                   (str-index "naltrexone"    ?n)))))
    (not (recommendation (patient-id ?pid) (type "recommendation")))
=>
    (assert (recommendation
        (patient-id ?pid)
        (type "recommendation")
        (message "Opioid dependence encounter without MAT. Consider methadone, buprenorphine, or naltrexone.")
        (severity "medium"))))
