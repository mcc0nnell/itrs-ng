#include "itrsng/number.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void copy_string(char *dst, size_t dst_size, const char *src) {
    if (dst_size == 0) return;
    if (!src) src = "";
    size_t len = 0;
    while (len + 1 < dst_size && src[len] != '\0') len++;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static bool token_equal(const char *start, size_t len, const char *token) {
    while (len > 0 && isspace((unsigned char)*start)) { start++; len--; }
    while (len > 0 && isspace((unsigned char)start[len - 1])) len--;
    return strlen(token) == len && strncmp(start, token, len) == 0;
}

static bool csv_has(const char *csv, const char *token) {
    if (!csv || !token || !*token) return false;
    const char *cur = csv;
    while (*cur) {
        const char *end = strchr(cur, ',');
        size_t len = end ? (size_t)(end - cur) : strlen(cur);
        if (token_equal(cur, len, token)) return true;
        if (!end) break;
        cur = end + 1;
    }
    return false;
}

static bool csv_intersects(const char *a, const char *b) {
    if (!a || !b) return false;
    const char *cur = a;
    while (*cur) {
        const char *end = strchr(cur, ',');
        size_t len = end ? (size_t)(end - cur) : strlen(cur);
        char token[ITRS_NUMBER_TOKEN_MAX];
        if (len >= sizeof(token)) len = sizeof(token) - 1;
        memcpy(token, cur, len);
        token[len] = '\0';
        char *start = token;
        while (*start && isspace((unsigned char)*start)) start++;
        char *tail = start + strlen(start);
        while (tail > start && isspace((unsigned char)tail[-1])) *--tail = '\0';
        if (*start && csv_has(b, start)) return true;
        if (!end) break;
        cur = end + 1;
    }
    return false;
}

static int scope_rank(const char *scope) {
    if (strcmp(scope, "local") == 0) return 0;
    if (strcmp(scope, "regional") == 0) return 1;
    if (strcmp(scope, "national") == 0) return 2;
    return 9;
}

static int role_rank(const char *role) {
    if (strcmp(role, "telecommunicator") == 0) return 0;
    if (strcmp(role, "interpreter") == 0) return 10;
    if (strcmp(role, "bridge") == 0) return 20;
    return 30;
}

static int candidate_compare(const void *lhs, const void *rhs) {
    const itrs_number_candidate_t *a = lhs;
    const itrs_number_candidate_t *b = rhs;
    if (a->eligible != b->eligible) return a->eligible ? -1 : 1;
    if (a->eligible && a->class_rank != b->class_rank) return a->class_rank < b->class_rank ? -1 : 1;
    if (a->eligible && a->priority != b->priority) return a->priority > b->priority ? -1 : 1;
    return strcmp(a->id, b->id);
}

static bool valid_scope(const char *scope) {
    return strcmp(scope, "local") == 0 || strcmp(scope, "regional") == 0 || strcmp(scope, "national") == 0;
}

static bool valid_role(const char *role) {
    return strcmp(role, "telecommunicator") == 0 || strcmp(role, "interpreter") == 0 || strcmp(role, "bridge") == 0;
}

static bool valid_request(const itrs_number_request_t *request) {
    return request->request_id[0] && request->service[0] && request->language[0] && request->media[0] &&
           request->jurisdiction_path[0] && request->authoritative_psap_id[0] &&
           request->authoritative_psap_endpoint[0] && request->resource_snapshot[0] && request->policy_version[0];
}

static uint32_t evaluate(const itrs_number_request_t *request, const itrs_asl_resource_t *resource, bool *eligible) {
    uint32_t mask = 0;
    bool ok = true;

    if (!resource->id[0] || !resource->endpoint[0] || !valid_role(resource->role) || !valid_scope(resource->scope) ||
        !resource->authority_source[0] || !resource->authority_version[0]) {
        mask |= ITRS_REASON_MALFORMED_RESOURCE;
        ok = false;
    }

    if (strcmp(resource->state, "available") == 0) mask |= ITRS_REASON_AVAILABLE;
    else { mask |= ITRS_REASON_UNAVAILABLE; ok = false; }

    if (strcmp(resource->language, request->language) == 0) mask |= ITRS_REASON_LANGUAGE_MATCH;
    else { mask |= ITRS_REASON_LANGUAGE_MISMATCH; ok = false; }

    if (csv_has(resource->media, request->media)) mask |= ITRS_REASON_MEDIA_MATCH;
    else { mask |= ITRS_REASON_MEDIA_MISMATCH; ok = false; }

    if (csv_has(resource->services, request->service)) mask |= ITRS_REASON_SERVICE_MATCH;
    else { mask |= ITRS_REASON_SERVICE_MISMATCH; ok = false; }

    if (strcmp(resource->scope, "national") == 0 || csv_intersects(resource->jurisdictions, request->jurisdiction_path)) {
        mask |= ITRS_REASON_JURISDICTION_MATCH;
    } else {
        mask |= ITRS_REASON_JURISDICTION_MISMATCH;
        ok = false;
    }

    *eligible = ok;
    return mask;
}

int itrs_number_resolve(const itrs_number_request_t *request,
                        const itrs_asl_resource_t *resources,
                        size_t resource_count,
                        itrs_number_result_t *result) {
    if (!request || !result || (resource_count > 0 && !resources)) return EINVAL;
    if (!valid_request(request)) return EINVAL;
    if (resource_count > ITRS_NUMBER_MAX_CANDIDATES) return EOVERFLOW;

    memset(result, 0, sizeof(*result));
    copy_string(result->request_id, sizeof(result->request_id), request->request_id);
    copy_string(result->authoritative_psap_id, sizeof(result->authoritative_psap_id), request->authoritative_psap_id);
    copy_string(result->authoritative_psap_endpoint, sizeof(result->authoritative_psap_endpoint), request->authoritative_psap_endpoint);
    copy_string(result->resource_snapshot, sizeof(result->resource_snapshot), request->resource_snapshot);
    copy_string(result->policy_version, sizeof(result->policy_version), request->policy_version);
    result->access_context_present = request->has_access_context;
    if (request->has_access_context) {
        result->access_context_valid = itrs_access_context_is_valid(&request->access_context);
        if (result->access_context_valid) result->access_context = request->access_context;
    }
    result->candidate_count = resource_count;

    for (size_t i = 0; i < resource_count; ++i) {
        const itrs_asl_resource_t *resource = &resources[i];
        itrs_number_candidate_t *candidate = &result->candidates[i];
        copy_string(candidate->id, sizeof(candidate->id), resource->id);
        copy_string(candidate->endpoint, sizeof(candidate->endpoint), resource->endpoint);
        copy_string(candidate->role, sizeof(candidate->role), resource->role);
        copy_string(candidate->scope, sizeof(candidate->scope), resource->scope);
        copy_string(candidate->authority_source, sizeof(candidate->authority_source), resource->authority_source);
        copy_string(candidate->authority_version, sizeof(candidate->authority_version), resource->authority_version);
        candidate->priority = resource->priority;
        candidate->class_rank = role_rank(resource->role) + scope_rank(resource->scope);
        candidate->reason_mask = evaluate(request, resource, &candidate->eligible);
    }

    qsort(result->candidates, result->candidate_count, sizeof(result->candidates[0]), candidate_compare);
    if (result->candidate_count > 0 && result->candidates[0].eligible) {
        const itrs_number_candidate_t *selected = &result->candidates[0];
        result->selected = true;
        copy_string(result->selected_id, sizeof(result->selected_id), selected->id);
        copy_string(result->selected_endpoint, sizeof(result->selected_endpoint), selected->endpoint);
        copy_string(result->selected_role, sizeof(result->selected_role), selected->role);
        copy_string(result->selected_scope, sizeof(result->selected_scope), selected->scope);
    }
    return 0;
}

typedef struct reason_name { uint32_t bit; const char *name; } reason_name_t;

int itrs_number_format_reasons(uint32_t mask, char *buffer, size_t buffer_size) {
    static const reason_name_t names[] = {
        {ITRS_REASON_AVAILABLE, "available"},
        {ITRS_REASON_LANGUAGE_MATCH, "language-match"},
        {ITRS_REASON_MEDIA_MATCH, "media-match"},
        {ITRS_REASON_SERVICE_MATCH, "service-match"},
        {ITRS_REASON_JURISDICTION_MATCH, "jurisdiction-match"},
        {ITRS_REASON_UNAVAILABLE, "unavailable"},
        {ITRS_REASON_LANGUAGE_MISMATCH, "language-mismatch"},
        {ITRS_REASON_MEDIA_MISMATCH, "media-mismatch"},
        {ITRS_REASON_SERVICE_MISMATCH, "service-mismatch"},
        {ITRS_REASON_JURISDICTION_MISMATCH, "jurisdiction-mismatch"},
        {ITRS_REASON_MALFORMED_RESOURCE, "malformed-resource"},
    };
    if (!buffer || buffer_size == 0) return EINVAL;
    buffer[0] = '\0';
    size_t used = 0;
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if ((mask & names[i].bit) == 0) continue;
        int written = snprintf(buffer + used, buffer_size - used, "%s%s", used ? "," : "", names[i].name);
        if (written < 0 || (size_t)written >= buffer_size - used) return EOVERFLOW;
        used += (size_t)written;
    }
    return 0;
}
