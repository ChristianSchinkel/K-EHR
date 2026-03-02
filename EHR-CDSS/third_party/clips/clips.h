/*
 * CLIPS stub header for EHR-CDSS.
 *
 * This file provides a minimal interface that mirrors the CLIPS 6.4 public API.
 * When the real CLIPS library is available, replace this file with the actual
 * clips.h from the CLIPS distribution and link against the CLIPS static library.
 *
 * CLIPS home: https://www.clipsrules.net/
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque environment handle */
typedef void* Environment;

/* Fact handle */
typedef struct clipsFactStruct {
    int dummy;
} CLIPSFact;

/* -----------------------------------------------------------------------
 * Environment lifecycle
 * --------------------------------------------------------------------- */
static inline Environment CreateEnvironment(void) { return nullptr; }
static inline int          DestroyEnvironment(Environment /*env*/) { return 1; }

/* -----------------------------------------------------------------------
 * Rule / fact loading
 * --------------------------------------------------------------------- */
static inline int Load(Environment /*env*/, const char* /*filename*/) { return 1; }
static inline int AssertString(Environment /*env*/, const char* /*factStr*/) { return 1; }

/* -----------------------------------------------------------------------
 * Inference engine
 * --------------------------------------------------------------------- */
static inline long long Run(Environment /*env*/, long long /*runLimit*/) { return 0; }

/* -----------------------------------------------------------------------
 * Fact evaluation helpers (stub only – real CLIPS exposes many more)
 * --------------------------------------------------------------------- */
static inline void Reset(Environment /*env*/) {}
static inline void Clear(Environment /*env*/) {}

#ifdef __cplusplus
} /* extern "C" */
#endif
