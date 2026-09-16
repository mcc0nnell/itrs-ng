#include "itrs_number_service.h"

#include <celix_bundle_activator.h>
#include <celix_compiler.h>
#include <stdio.h>
#include <string.h>

typedef struct activator_data {
    long tracker_id;
} activator_data_t;

static void use_number(void *handle CELIX_UNUSED, void *svc) {
    itrs_number_service_t *number = svc;
    itrs_number_request_t req = {0};
    snprintf(req.request_id, sizeof(req.request_id), "celix-smoke-001");
    snprintf(req.service, sizeof(req.service), "urn:service:sos");
    snprintf(req.language, sizeof(req.language), "ASL");
    snprintf(req.media, sizeof(req.media), "video");
    snprintf(req.jurisdiction_path, sizeof(req.jurisdiction_path), "US,US-MD,US-MD-FREDERICK");
    snprintf(req.authoritative_psap_id, sizeof(req.authoritative_psap_id), "psap-frederick");
    snprintf(req.authoritative_psap_endpoint, sizeof(req.authoritative_psap_endpoint), "sip:psap-frederick@example.invalid");
    snprintf(req.resource_snapshot, sizeof(req.resource_snapshot), "celix-live-snapshot");
    snprintf(req.policy_version, sizeof(req.policy_version), "baseline-v1");
    itrs_number_result_t result;
    int rc = number->resolve(number->handle, &req, &result);
    printf("ITRS_NG_CELIX_SMOKE rc=%d psap=%s selected=%s candidates=%zu\n",
           rc, result.authoritative_psap_id, result.selected ? result.selected_id : "none", result.candidate_count);
    fflush(stdout);
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = ITRS_NUMBER_SERVICE_NAME;
    opts.callbackHandle = data;
    opts.add = use_number;
    data->tracker_id = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    return data->tracker_id < 0 ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->tracker_id >= 0) celix_bundleContext_stopTracker(ctx, data->tracker_id);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
