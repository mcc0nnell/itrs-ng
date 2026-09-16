#include "itrsng/number.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (0)

static itrs_asl_resource_t make_resource(const char *id, const char *role, const char *scope,
                                         const char *jurisdictions, const char *state, int priority) {
    itrs_asl_resource_t r = {0};
    snprintf(r.id, sizeof(r.id), "%s", id);
    snprintf(r.endpoint, sizeof(r.endpoint), "sip:%s@example.invalid", id);
    snprintf(r.language, sizeof(r.language), "ASL");
    snprintf(r.media, sizeof(r.media), "video,rtt");
    snprintf(r.role, sizeof(r.role), "%s", role);
    snprintf(r.scope, sizeof(r.scope), "%s", scope);
    snprintf(r.jurisdictions, sizeof(r.jurisdictions), "%s", jurisdictions);
    snprintf(r.services, sizeof(r.services), "urn:service:sos");
    snprintf(r.state, sizeof(r.state), "%s", state);
    snprintf(r.authority_source, sizeof(r.authority_source), "fixture-registry");
    snprintf(r.authority_version, sizeof(r.authority_version), "fixture-v1");
    r.priority = priority;
    return r;
}

static itrs_number_request_t request(void) {
    itrs_number_request_t req = {0};
    snprintf(req.request_id, sizeof(req.request_id), "req-1");
    snprintf(req.service, sizeof(req.service), "urn:service:sos");
    snprintf(req.language, sizeof(req.language), "ASL");
    snprintf(req.media, sizeof(req.media), "video");
    snprintf(req.jurisdiction_path, sizeof(req.jurisdiction_path), "US,US-MD,US-MD-FREDERICK");
    snprintf(req.authoritative_psap_id, sizeof(req.authoritative_psap_id), "psap-frederick");
    snprintf(req.authoritative_psap_endpoint, sizeof(req.authoritative_psap_endpoint), "sip:psap-frederick@example.invalid");
    snprintf(req.resource_snapshot, sizeof(req.resource_snapshot), "fixture-v1");
    snprintf(req.policy_version, sizeof(req.policy_version), "baseline-v1");
    return req;
}

static itrs_access_context_t access_context(const char *profile, const char *attachment, const char *ims) {
    itrs_access_context_t ctx = {0};
    snprintf(ctx.observation_id, sizeof(ctx.observation_id), "access-test-001");
    snprintf(ctx.observed_at, sizeof(ctx.observed_at), "2026-09-16T21:45:00Z");
    snprintf(ctx.source_kind, sizeof(ctx.source_kind), "synthetic");
    snprintf(ctx.source_authority, sizeof(ctx.source_authority), "windanvil-test");
    snprintf(ctx.transport, sizeof(ctx.transport), "cellular");
    snprintf(ctx.profile_state, sizeof(ctx.profile_state), "%s", profile);
    snprintf(ctx.attachment, sizeof(ctx.attachment), "%s", attachment);
    snprintf(ctx.ims_state, sizeof(ctx.ims_state), "%s", ims);
    snprintf(ctx.emergency_access_state, sizeof(ctx.emergency_access_state), "unknown");
    ctx.capability_count = 1;
    snprintf(ctx.capabilities[0].name, sizeof(ctx.capabilities[0].name), "rtt");
    snprintf(ctx.capabilities[0].state, sizeof(ctx.capabilities[0].state), "available");
    snprintf(ctx.capabilities[0].basis, sizeof(ctx.capabilities[0].basis), "synthetic");
    return ctx;
}

static void baseline(itrs_asl_resource_t out[3]) {
    out[0] = make_resource("local", "telecommunicator", "local", "US-MD-FREDERICK", "available", 100);
    out[1] = make_resource("regional", "telecommunicator", "regional", "US-MD", "available", 90);
    out[2] = make_resource("bridge", "bridge", "national", "US", "available", 10);
}

static int test_local_selected(void) {
    itrs_asl_resource_t resources[3]; baseline(resources);
    itrs_number_request_t req = request();
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 3, &result) == 0);
    CHECK(result.selected);
    CHECK(strcmp(result.selected_id, "local") == 0);
    CHECK(strcmp(result.authoritative_psap_id, req.authoritative_psap_id) == 0);
    CHECK(strcmp(result.authoritative_psap_endpoint, req.authoritative_psap_endpoint) == 0);
    CHECK(!result.access_context_present);
    return 0;
}

static int test_regional_failover(void) {
    itrs_asl_resource_t resources[3]; baseline(resources);
    snprintf(resources[0].state, sizeof(resources[0].state), "unavailable");
    itrs_number_request_t req = request();
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 3, &result) == 0);
    CHECK(strcmp(result.selected_id, "regional") == 0);
    CHECK(strcmp(result.authoritative_psap_id, "psap-frederick") == 0);
    return 0;
}

static int test_bridge_failover(void) {
    itrs_asl_resource_t resources[3]; baseline(resources);
    snprintf(resources[0].state, sizeof(resources[0].state), "unavailable");
    snprintf(resources[1].state, sizeof(resources[1].state), "unavailable");
    itrs_number_request_t req = request();
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 3, &result) == 0);
    CHECK(strcmp(result.selected_id, "bridge") == 0);
    CHECK(strcmp(result.selected_role, "bridge") == 0);
    CHECK(strcmp(result.authoritative_psap_endpoint, req.authoritative_psap_endpoint) == 0);
    return 0;
}

static int test_input_order_does_not_change_result(void) {
    itrs_asl_resource_t a[3]; baseline(a);
    itrs_asl_resource_t b[3] = {a[2], a[0], a[1]};
    itrs_number_request_t req = request();
    itrs_number_result_t ra, rb;
    CHECK(itrs_number_resolve(&req, a, 3, &ra) == 0);
    CHECK(itrs_number_resolve(&req, b, 3, &rb) == 0);
    CHECK(strcmp(ra.selected_id, rb.selected_id) == 0);
    CHECK(ra.candidate_count == rb.candidate_count);
    for (size_t i = 0; i < ra.candidate_count; ++i) {
        CHECK(strcmp(ra.candidates[i].id, rb.candidates[i].id) == 0);
        CHECK(ra.candidates[i].reason_mask == rb.candidates[i].reason_mask);
    }
    return 0;
}

static int test_wrong_jurisdiction_is_ineligible(void) {
    itrs_asl_resource_t resources[2];
    resources[0] = make_resource("wrong-local", "telecommunicator", "local", "US-CA-LOSANGELES", "available", 1000);
    resources[1] = make_resource("regional", "telecommunicator", "regional", "US-MD", "available", 1);
    itrs_number_request_t req = request();
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 2, &result) == 0);
    CHECK(strcmp(result.selected_id, "regional") == 0);
    CHECK(!result.candidates[1].eligible);
    CHECK(result.candidates[1].reason_mask & ITRS_REASON_JURISDICTION_MISMATCH);
    return 0;
}

static int test_no_eligible_candidate_is_explicit(void) {
    itrs_asl_resource_t resources[1];
    resources[0] = make_resource("down", "telecommunicator", "local", "US-MD-FREDERICK", "unavailable", 100);
    itrs_number_request_t req = request();
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 1, &result) == 0);
    CHECK(!result.selected);
    CHECK(result.candidate_count == 1);
    CHECK(strcmp(result.authoritative_psap_id, req.authoritative_psap_id) == 0);
    return 0;
}

static int test_malformed_resource_is_ineligible(void) {
    itrs_asl_resource_t resources[2];
    resources[0] = make_resource("malformed", "telecommunicator", "local", "US-MD-FREDERICK", "available", 1000);
    resources[0].endpoint[0] = '\0';
    resources[1] = make_resource("regional", "telecommunicator", "regional", "US-MD", "available", 1);
    itrs_number_request_t req = request();
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 2, &result) == 0);
    CHECK(strcmp(result.selected_id, "regional") == 0);
    CHECK(!result.candidates[1].eligible);
    CHECK(result.candidates[1].reason_mask & ITRS_REASON_MALFORMED_RESOURCE);
    return 0;
}

static int test_malformed_request_fails_closed(void) {
    itrs_asl_resource_t resources[3]; baseline(resources);
    itrs_number_request_t req = request();
    req.authoritative_psap_id[0] = '\0';
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 3, &result) != 0);
    return 0;
}

static int test_capacity_fails_closed(void) {
    itrs_asl_resource_t resources[ITRS_NUMBER_MAX_CANDIDATES + 1] = {0};
    itrs_number_request_t req = request();
    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, ITRS_NUMBER_MAX_CANDIDATES + 1, &result) != 0);
    return 0;
}

static int test_access_state_does_not_move_psap_or_selection(void) {
    static const struct {
        const char *profile;
        const char *attachment;
        const char *ims;
    } arms[] = {
        {"active", "home", "registered"},
        {"active", "roaming", "registered"},
        {"inactive", "detached", "unregistered"},
        {"absent", "unknown", "unknown"},
        {"unknown", "unknown", "unknown"},
    };

    for (size_t i = 0; i < sizeof(arms) / sizeof(arms[0]); ++i) {
        itrs_asl_resource_t resources[3]; baseline(resources);
        itrs_number_request_t req = request();
        req.has_access_context = true;
        req.access_context = access_context(arms[i].profile, arms[i].attachment, arms[i].ims);
        snprintf(req.access_context.observation_id, sizeof(req.access_context.observation_id), "access-arm-%zu", i);

        itrs_number_result_t result;
        CHECK(itrs_number_resolve(&req, resources, 3, &result) == 0);
        CHECK(result.selected);
        CHECK(strcmp(result.selected_id, "local") == 0);
        CHECK(strcmp(result.authoritative_psap_id, "psap-frederick") == 0);
        CHECK(strcmp(result.authoritative_psap_endpoint, "sip:psap-frederick@example.invalid") == 0);
        CHECK(result.access_context_present);
        CHECK(result.access_context_valid);
        CHECK(strcmp(result.access_context.profile_state, arms[i].profile) == 0);
    }
    return 0;
}

static int test_malformed_access_context_is_nonfatal(void) {
    itrs_asl_resource_t resources[3]; baseline(resources);
    itrs_number_request_t req = request();
    req.has_access_context = true;
    memset(&req.access_context, 0, sizeof(req.access_context));

    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 3, &result) == 0);
    CHECK(result.selected);
    CHECK(strcmp(result.selected_id, "local") == 0);
    CHECK(strcmp(result.authoritative_psap_id, "psap-frederick") == 0);
    CHECK(result.access_context_present);
    CHECK(!result.access_context_valid);
    return 0;
}

static int test_access_capability_cannot_expand_number_authority(void) {
    itrs_asl_resource_t resources[1];
    resources[0] = make_resource("rtt-only", "telecommunicator", "local", "US-MD-FREDERICK", "available", 100);
    snprintf(resources[0].media, sizeof(resources[0].media), "rtt");

    itrs_number_request_t req = request();
    req.has_access_context = true;
    req.access_context = access_context("active", "home", "registered");
    snprintf(req.access_context.capabilities[0].name, sizeof(req.access_context.capabilities[0].name), "video");

    itrs_number_result_t result;
    CHECK(itrs_number_resolve(&req, resources, 1, &result) == 0);
    CHECK(result.access_context_valid);
    CHECK(!result.selected);
    CHECK(result.candidate_count == 1);
    CHECK(result.candidates[0].reason_mask & ITRS_REASON_MEDIA_MISMATCH);
    CHECK(strcmp(result.authoritative_psap_id, "psap-frederick") == 0);
    return 0;
}

int main(void) {
    int (*tests[])(void) = {
        test_local_selected,
        test_regional_failover,
        test_bridge_failover,
        test_input_order_does_not_change_result,
        test_wrong_jurisdiction_is_ineligible,
        test_no_eligible_candidate_is_explicit,
        test_malformed_resource_is_ineligible,
        test_malformed_request_fails_closed,
        test_capacity_fails_closed,
        test_access_state_does_not_move_psap_or_selection,
        test_malformed_access_context_is_nonfatal,
        test_access_capability_cannot_expand_number_authority,
    };
    const char *names[] = {
        "local selected", "regional failover", "bridge failover", "deterministic input order",
        "jurisdiction eligibility", "explicit no candidate", "malformed resource rejected",
        "malformed request rejected", "capacity fails closed", "access state preserves authority",
        "malformed access is nonfatal", "access cannot expand authority"
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        int rc = tests[i]();
        if (rc != 0) return rc;
        printf("ok %zu - %s\n", i + 1, names[i]);
    }
    return 0;
}
