#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum itrs_a11y_severity {
    ITRS_A11Y_SEVERITY_NONE = 0,
    ITRS_A11Y_SEVERITY_LOW = 1,
    ITRS_A11Y_SEVERITY_MEDIUM = 2,
    ITRS_A11Y_SEVERITY_HIGH = 3,
    ITRS_A11Y_SEVERITY_CRITICAL = 4
} itrs_a11y_severity_t;

typedef struct itrs_a11y_score {
    uint16_t score_tenths;
    itrs_a11y_severity_t severity;
} itrs_a11y_score_t;

/**
 * Parse and score an A11YV 1.0 vector.
 *
 * Example:
 * A11YV:1.0/BL:T/TC:C/ALT:N/FQ:A/PS:P/AU:I
 *
 * score_tenths uses 0..100 to represent 0.0..10.0.
 * The score is a remediation-priority signal, not a WCAG conformance score.
 * Returns 0 on success or EINVAL for a malformed/unsupported vector.
 */
int itrs_a11y_score_vector(const char *vector, itrs_a11y_score_t *result);

const char *itrs_a11y_severity_name(itrs_a11y_severity_t severity);

#ifdef __cplusplus
}
#endif
