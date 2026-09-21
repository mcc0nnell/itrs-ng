#include "itrsng/accessibility.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct metric_spec {
    const char *name;
    const char *values;
    const uint8_t *scores;
} metric_spec_t;

static const uint8_t BL_SCORES[] = {0u, 1u, 2u, 3u};
static const uint8_t TC_SCORES[] = {0u, 1u, 2u, 3u};
static const uint8_t ALT_SCORES[] = {0u, 1u, 2u, 3u};
static const uint8_t FQ_SCORES[] = {0u, 1u, 2u, 3u};
static const uint8_t PS_SCORES[] = {0u, 1u, 2u, 3u};
static const uint8_t AU_SCORES[] = {0u, 1u, 2u, 3u};

static const metric_spec_t SPECS[] = {
    {"BL", "NFPT", BL_SCORES},
    {"TC", "ASCE", TC_SCORES},
    {"ALT", "EDRN", ALT_SCORES},
    {"FQ", "RCFA", FQ_SCORES},
    {"PS", "MISP", PS_SCORES},
    {"AU", "NRDI", AU_SCORES},
};

static int metric_index(const char *name, size_t name_len) {
    for (size_t i = 0; i < sizeof(SPECS) / sizeof(SPECS[0]); ++i) {
        if (strlen(SPECS[i].name) == name_len && strncmp(SPECS[i].name, name, name_len) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static int value_score(const metric_spec_t *spec, char value, uint8_t *out) {
    const char *p = strchr(spec->values, value);
    if (!p) return EINVAL;
    size_t index = (size_t)(p - spec->values);
    *out = spec->scores[index];
    return 0;
}

static itrs_a11y_severity_t severity_for(uint16_t score_tenths) {
    if (score_tenths == 0u) return ITRS_A11Y_SEVERITY_NONE;
    if (score_tenths <= 39u) return ITRS_A11Y_SEVERITY_LOW;
    if (score_tenths <= 69u) return ITRS_A11Y_SEVERITY_MEDIUM;
    if (score_tenths <= 89u) return ITRS_A11Y_SEVERITY_HIGH;
    return ITRS_A11Y_SEVERITY_CRITICAL;
}

int itrs_a11y_score_vector(const char *vector, itrs_a11y_score_t *result) {
    static const char prefix[] = "A11YV:1.0/";
    static const uint8_t weights[] = {4u, 2u, 3u, 1u, 1u, 3u};
    const uint16_t max_weighted = 42u;

    if (!vector || !result || strncmp(vector, prefix, sizeof(prefix) - 1u) != 0) return EINVAL;

    uint8_t values[sizeof(SPECS) / sizeof(SPECS[0])] = {0};
    bool seen[sizeof(SPECS) / sizeof(SPECS[0])] = {false};
    const char *cursor = vector + sizeof(prefix) - 1u;

    while (*cursor) {
        const char *colon = strchr(cursor, ':');
        if (!colon || colon == cursor) return EINVAL;
        const char *slash = strchr(colon + 1, '/');
        size_t value_len = slash ? (size_t)(slash - (colon + 1)) : strlen(colon + 1);
        if (value_len != 1u) return EINVAL;

        int index = metric_index(cursor, (size_t)(colon - cursor));
        if (index < 0 || seen[index]) return EINVAL;

        int rc = value_score(&SPECS[index], colon[1], &values[index]);
        if (rc != 0) return rc;
        seen[index] = true;

        if (!slash) {
            cursor += strlen(cursor);
            break;
        }
        cursor = slash + 1;
        if (!*cursor) return EINVAL;
    }

    uint16_t weighted = 0u;
    for (size_t i = 0; i < sizeof(SPECS) / sizeof(SPECS[0]); ++i) {
        if (!seen[i]) return EINVAL;
        weighted += (uint16_t)values[i] * weights[i];
    }

    uint16_t score_tenths = (uint16_t)((weighted * 100u + (max_weighted / 2u)) / max_weighted);
    if (score_tenths > 100u) score_tenths = 100u;
    result->score_tenths = score_tenths;
    result->severity = severity_for(score_tenths);
    return 0;
}

const char *itrs_a11y_severity_name(itrs_a11y_severity_t severity) {
    switch (severity) {
        case ITRS_A11Y_SEVERITY_NONE: return "none";
        case ITRS_A11Y_SEVERITY_LOW: return "low";
        case ITRS_A11Y_SEVERITY_MEDIUM: return "medium";
        case ITRS_A11Y_SEVERITY_HIGH: return "high";
        case ITRS_A11Y_SEVERITY_CRITICAL: return "critical";
        default: return "unknown";
    }
}
