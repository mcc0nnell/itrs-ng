#include "itrsng/number.h"

#include <stdio.h>
#include <string.h>

static itrs_asl_resource_t resource(const char *id, const char *endpoint, const char *role,
                                    const char *scope, const char *jurisdictions, const char *state,
                                    int priority) {
    itrs_asl_resource_t r = {0};
    snprintf(r.id, sizeof(r.id), "%s", id);
    snprintf(r.endpoint, sizeof(r.endpoint), "%s", endpoint);
    snprintf(r.language, sizeof(r.language), "ASL");
    snprintf(r.media, sizeof(r.media), "video,rtt");
    snprintf(r.role, sizeof(r.role), "%s", role);
    snprintf(r.scope, sizeof(r.scope), "%s", scope);
    snprintf(r.jurisdictions, sizeof(r.jurisdictions), "%s", jurisdictions);
    snprintf(r.services, sizeof(r.services), "urn:service:sos");
    snprintf(r.state, sizeof(r.state), "%s", state);
    snprintf(r.authority_source, sizeof(r.authority_source), "synthetic-registry");
    snprintf(r.authority_version, sizeof(r.authority_version), "snapshot-001");
    r.priority = priority;
    return r;
}

int main(int argc, char **argv) {
    const char *local_state = "available";
    const char *regional_state = "available";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--local-down") == 0) local_state = "unavailable";
        else if (strcmp(argv[i], "--regional-down") == 0) regional_state = "unavailable";
    }

    itrs_number_request_t req = {0};
    snprintf(req.request_id, sizeof(req.request_id), "demo-001");
    snprintf(req.service, sizeof(req.service), "urn:service:sos");
    snprintf(req.language, sizeof(req.language), "ASL");
    snprintf(req.media, sizeof(req.media), "video");
    snprintf(req.jurisdiction_path, sizeof(req.jurisdiction_path), "US,US-MD,US-MD-FREDERICK");
    snprintf(req.authoritative_psap_id, sizeof(req.authoritative_psap_id), "psap-frederick");
    snprintf(req.authoritative_psap_endpoint, sizeof(req.authoritative_psap_endpoint), "sip:psap-frederick@example.invalid");
    snprintf(req.resource_snapshot, sizeof(req.resource_snapshot), "snapshot-001");
    snprintf(req.policy_version, sizeof(req.policy_version), "baseline-001");

    itrs_asl_resource_t resources[] = {
        resource("asl-local", "sip:asl-local@example.invalid", "telecommunicator", "local", "US-MD-FREDERICK", local_state, 100),
        resource("asl-regional", "sip:asl-regional@example.invalid", "telecommunicator", "regional", "US-MD", regional_state, 90),
        resource("vrs-bridge", "sip:vrs-bridge@example.invalid", "bridge", "national", "US", "available", 10),
    };

    itrs_number_result_t result;
    int rc = itrs_number_resolve(&req, resources, sizeof(resources) / sizeof(resources[0]), &result);
    if (rc != 0) return rc;

    printf("authoritative_psap=%s <%s>\n", result.authoritative_psap_id, result.authoritative_psap_endpoint);
    printf("selected=%s <%s> role=%s scope=%s\n",
           result.selected ? result.selected_id : "none",
           result.selected ? result.selected_endpoint : "",
           result.selected ? result.selected_role : "",
           result.selected ? result.selected_scope : "");
    for (size_t i = 0; i < result.candidate_count; ++i) {
        char reasons[512];
        itrs_number_format_reasons(result.candidates[i].reason_mask, reasons, sizeof(reasons));
        printf("candidate[%zu]=%s eligible=%s class=%d priority=%d reasons=%s\n",
               i, result.candidates[i].id, result.candidates[i].eligible ? "true" : "false",
               result.candidates[i].class_rank, result.candidates[i].priority, reasons);
    }
    return 0;
}
