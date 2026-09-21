#include "itrsng/accessibility.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static void expect(const char *vector, unsigned score_tenths, const char *severity) {
    itrs_a11y_score_t score = {0};
    assert(itrs_a11y_score_vector(vector, &score) == 0);
    assert(score.score_tenths == score_tenths);
    assert(strcmp(itrs_a11y_severity_name(score.severity), severity) == 0);
}

int main(void) {
    expect("A11YV:1.0/BL:N/TC:A/ALT:E/FQ:R/PS:M/AU:N", 0u, "none");
    expect("A11YV:1.0/BL:T/TC:E/ALT:N/FQ:A/PS:P/AU:I", 100u, "critical");
    expect("A11YV:1.0/BL:T/TC:C/ALT:N/FQ:A/PS:P/AU:I", 95u, "critical");
    expect("A11YV:1.0/TC:C/BL:T/ALT:N/FQ:A/PS:P/AU:I", 95u, "critical");

    itrs_a11y_score_t score = {0};
    assert(itrs_a11y_score_vector("A11YV:1.0/BL:T/TC:C", &score) == EINVAL);
    assert(itrs_a11y_score_vector("A11YV:1.0/BL:X/TC:C/ALT:N/FQ:A/PS:P/AU:I", &score) == EINVAL);
    assert(itrs_a11y_score_vector("CVSS:4.0/AV:N", &score) == EINVAL);

    puts("itrs-a11y-vector-test: ok");
    return 0;
}
