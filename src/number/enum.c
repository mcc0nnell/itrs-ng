#include "itrsng/enum.h"

#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool allowed_separator(unsigned char c) {
    return c == ' ' || c == '\t' || c == '-' || c == '(' || c == ')' || c == '.';
}

int itrs_enum_normalize_e164(const char *input, char *aus, size_t aus_size) {
    if (input == NULL || aus == NULL || aus_size < 3) return EINVAL;
    char digits[16] = {0};
    size_t count = 0;
    bool saw_plus = false;
    bool saw_nonspace = false;
    for (const unsigned char *p = (const unsigned char *)input; *p != '\0'; ++p) {
        if (isdigit(*p)) {
            if (count >= 15) return EOVERFLOW;
            digits[count++] = (char)*p;
            saw_nonspace = true;
        } else if (*p == '+') {
            if (saw_plus || saw_nonspace) return EINVAL;
            saw_plus = true;
            saw_nonspace = true;
        } else if (allowed_separator(*p)) {
            continue;
        } else {
            return EINVAL;
        }
    }
    if (count == 0 || digits[0] == '0') return EINVAL;
    if (aus_size < count + 2) return ENOSPC;
    aus[0] = '+';
    memcpy(aus + 1, digits, count);
    aus[count + 1] = '\0';
    return 0;
}

static bool valid_apex(const char *apex) {
    if (apex == NULL || apex[0] == '\0') return false;
    bool label_has_char = false;
    for (const unsigned char *p = (const unsigned char *)apex; *p != '\0'; ++p) {
        if (isalnum(*p) || *p == '-') {
            label_has_char = true;
            continue;
        }
        if (*p == '.') {
            if (!label_has_char && p[1] != '\0') return false;
            label_has_char = false;
            continue;
        }
        return false;
    }
    return label_has_char || apex[strlen(apex) - 1] == '.';
}

int itrs_enum_domain_from_e164(const char *input, const char *apex,
                               char *domain, size_t domain_size) {
    if (domain == NULL || domain_size == 0 || !valid_apex(apex)) return EINVAL;
    char aus[32] = {0};
    int rc = itrs_enum_normalize_e164(input, aus, sizeof(aus));
    if (rc != 0) return rc;

    size_t used = 0;
    const size_t digits = strlen(aus) - 1;
    for (size_t i = 0; i < digits; ++i) {
        if (used + 2 >= domain_size) return ENOSPC;
        domain[used++] = aus[digits - i];
        domain[used++] = '.';
    }
    const size_t apex_len = strlen(apex);
    const bool apex_has_dot = apex_len > 0 && apex[apex_len - 1] == '.';
    const size_t need = used + apex_len + (apex_has_dot ? 0 : 1) + 1;
    if (need > domain_size) return ENOSPC;
    memcpy(domain + used, apex, apex_len);
    used += apex_len;
    if (!apex_has_dot) domain[used++] = '.';
    domain[used] = '\0';
    return 0;
}

static bool service_has_sip(const char *service) {
    if (service == NULL) return false;
    char copy[ITRS_ENUM_SERVICE_MAX];
    const size_t len = strnlen(service, sizeof(copy));
    if (len == 0 || len >= sizeof(copy)) return false;
    for (size_t i = 0; i <= len; ++i) copy[i] = (char)tolower((unsigned char)service[i]);
    char *save = NULL;
    char *token = strtok_r(copy, "+", &save);
    if (token == NULL || strcmp(token, "e2u") != 0) return false;
    while ((token = strtok_r(NULL, "+", &save)) != NULL) {
        char *colon = strchr(token, ':');
        if (colon != NULL) *colon = '\0';
        if (strcmp(token, "sip") == 0) return true;
    }
    return false;
}

static bool terminal_u(const char *flags) {
    return flags != NULL && flags[0] != '\0' && flags[1] == '\0' &&
           (flags[0] == 'u' || flags[0] == 'U');
}

typedef struct indexed_record {
    const itrs_naptr_record_t *r;
    size_t index;
} indexed_record_t;

static int record_cmp(const void *a, const void *b) {
    const indexed_record_t *ra = a, *rb = b;
    if (ra->r->order != rb->r->order) return ra->r->order < rb->r->order ? -1 : 1;
    if (ra->r->preference != rb->r->preference) return ra->r->preference < rb->r->preference ? -1 : 1;
    int c = strcmp(ra->r->service, rb->r->service); if (c != 0) return c;
    c = strcmp(ra->r->regexp, rb->r->regexp); if (c != 0) return c;
    c = strcmp(ra->r->replacement, rb->r->replacement); if (c != 0) return c;
    return ra->index < rb->index ? -1 : (ra->index > rb->index ? 1 : 0);
}

static int split_regexp(const char *expr, char *pattern, size_t pattern_size,
                        char *repl, size_t repl_size) {
    if (expr == NULL || expr[0] == '\0') return EINVAL;
    const char delim = expr[0];
    if (isalnum((unsigned char)delim) || delim == '\\') return EINVAL;
    size_t part = 0, pused = 0, rused = 0;
    bool escaped = false;
    for (size_t i = 1; expr[i] != '\0'; ++i) {
        char c = expr[i];
        if (!escaped && c == delim) {
            part++;
            if (part == 2) {
                if (expr[i + 1] != '\0') return ENOTSUP;
                pattern[pused] = '\0'; repl[rused] = '\0';
                return 0;
            }
            continue;
        }
        char *dst = part == 0 ? pattern : repl;
        size_t *used = part == 0 ? &pused : &rused;
        size_t cap = part == 0 ? pattern_size : repl_size;
        if (part > 1 || *used + 1 >= cap) return ENOSPC;
        dst[(*used)++] = c;
        if (!escaped && c == '\\') escaped = true; else escaped = false;
    }
    return EINVAL;
}

static int expand_replacement(const char *repl, const char *input,
                              const regmatch_t matches[10], char *out, size_t out_size) {
    size_t used = 0;
    for (size_t i = 0; repl[i] != '\0'; ++i) {
        if (repl[i] == '\\' && repl[i + 1] >= '0' && repl[i + 1] <= '9') {
            const int n = repl[++i] - '0';
            if (matches[n].rm_so < 0 || matches[n].rm_eo < matches[n].rm_so) return EINVAL;
            const size_t len = (size_t)(matches[n].rm_eo - matches[n].rm_so);
            if (used + len >= out_size) return ENOSPC;
            memcpy(out + used, input + matches[n].rm_so, len);
            used += len;
        } else {
            if (used + 1 >= out_size) return ENOSPC;
            out[used++] = repl[i];
        }
    }
    out[used] = '\0';
    return 0;
}

static int apply_rule(const char *aus, const itrs_naptr_record_t *record,
                      char *uri, size_t uri_size) {
    char pattern[ITRS_ENUM_REGEXP_MAX] = {0};
    char repl[ITRS_ENUM_URI_MAX] = {0};
    int rc = split_regexp(record->regexp, pattern, sizeof(pattern), repl, sizeof(repl));
    if (rc != 0) return rc;
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) return EINVAL;
    regmatch_t matches[10];
    const int matched = regexec(&regex, aus, 10, matches, 0);
    regfree(&regex);
    if (matched == REG_NOMATCH) return ENOENT;
    if (matched != 0) return EINVAL;
    rc = expand_replacement(repl, aus, matches, uri, uri_size);
    if (rc != 0) return rc;
    if (strncmp(uri, "sip:", 4) != 0 && strncmp(uri, "sips:", 5) != 0) return EINVAL;
    return 0;
}

int itrs_enum_resolve_sip(const char *input, const char *apex,
                          const itrs_naptr_record_t *records, size_t record_count,
                          itrs_enum_result_t *result) {
    if (input == NULL || records == NULL || result == NULL) return EINVAL;
    if (record_count == 0) return ENOENT;
    if (record_count > ITRS_ENUM_MAX_RECORDS) return EOVERFLOW;
    memset(result, 0, sizeof(*result));
    int rc = itrs_enum_normalize_e164(input, result->aus, sizeof(result->aus));
    if (rc != 0) return rc;
    rc = itrs_enum_domain_from_e164(input, apex, result->query_domain, sizeof(result->query_domain));
    if (rc != 0) return rc;

    indexed_record_t sorted[ITRS_ENUM_MAX_RECORDS];
    for (size_t i = 0; i < record_count; ++i) sorted[i] = (indexed_record_t){.r = &records[i], .index = i};
    qsort(sorted, record_count, sizeof(sorted[0]), record_cmp);

    for (size_t i = 0; i < record_count; ++i) {
        const itrs_naptr_record_t *r = sorted[i].r;
        if (!terminal_u(r->flags) || !service_has_sip(r->service) || r->regexp[0] == '\0') continue;
        if (r->replacement[0] != '\0' && strcmp(r->replacement, ".") != 0) continue;
        char uri[ITRS_ENUM_URI_MAX] = {0};
        rc = apply_rule(result->aus, r, uri, sizeof(uri));
        if (rc == ENOENT || rc == EINVAL || rc == ENOTSUP) continue;
        if (rc != 0) return rc;
        memcpy(result->uri, uri, strlen(uri) + 1);
        result->order = r->order;
        result->preference = r->preference;
        result->source_index = sorted[i].index;
        return 0;
    }
    return ENOENT;
}
