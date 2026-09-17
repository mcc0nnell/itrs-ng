#include "itrs_number_service.h"

#include <celix_bundle_activator.h>
#include <celix_compiler.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct activator_data {
    celix_bundle_context_t *ctx;
    pthread_t thread;
    bool thread_started;
    long guard_tracker_id;
} activator_data_t;

static void use_number(void *handle CELIX_UNUSED, void *svc) {
    itrs_number_service_t *number = svc;
    itrs_number_request_t req = {0};
    snprintf(req.request_id, sizeof(req.request_id), "celix-smoke-001");
    snprintf(req.service, sizeof(req.service), "urn:service:sos");
    snprintf(req.language, sizeof(req.language), "ASL");
    snprintf(req.media, sizeof(req.media), "video");
    snprintf(req.jurisdiction_path, sizeof(req.jurisdiction_path), "US,US-MD,US-MD-FREDERICK");
    snprintf(req.authoritative_psap_id, sizeof(req.authoritative_psap_id), "psap-frederick");    snprintf(req.authoritative_psap_endpoint, sizeof(req.authoritative_psap_endpoint), "sip:psap-frederick@example.invalid");
    snprintf(req.resource_snapshot, sizeof(req.resource_snapshot), "celix-live-snapshot");
    snprintf(req.policy_version, sizeof(req.policy_version), "baseline-v1");
    itrs_number_result_t result = {0};
    int rc = number->resolve(number->handle, &req, &result);
    printf("ITRS_NG_CELIX_SMOKE rc=%d psap=%s selected=%s candidates=%zu\n",
           rc, result.authoritative_psap_id, result.selected ? result.selected_id : "none", result.candidate_count);

    itrs_number_enum_result_t enum_result = {0};
    int enum_rc = number->resolveE164 != NULL
        ? number->resolveE164(number->handle, "+18015551212", ITRS_ENUM_DEFAULT_APEX, &enum_result)
        : ENOSYS;
    printf("ITRS_NG_ENUM_SMOKE rc=%d query=%s uri=%s provider=%s ttl=%u ad=%s records=%zu\n",
           enum_rc, enum_rc == 0 ? enum_result.resolution.query_domain : "none",
           enum_rc == 0 ? enum_result.resolution.uri : "none",
           enum_rc == 0 ? enum_result.provider_source : "none",
           enum_rc == 0 ? enum_result.min_ttl_seconds : 0u,
           enum_rc == 0 && enum_result.authenticated_data ? "true" : "false",
           enum_rc == 0 ? enum_result.observed_record_count : 0u);
    fflush(stdout);
}


static void probe_event_loop_guard(void *handle CELIX_UNUSED, void *svc) {
    itrs_number_service_t *number = svc;
    itrs_number_enum_result_t result = {0};
    int rc = number->resolveE164(number->handle, "+18015551212", ITRS_ENUM_DEFAULT_APEX, &result);
    printf("ITRS_NG_ENUM_EVENT_LOOP_GUARD rc=%d pass=%s\n",
           rc, rc == EWOULDBLOCK ? "true" : "false");
    fflush(stdout);
}

static void *smoke_thread(void *arg) {
    activator_data_t *data = arg;
    usleep(100000);
    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    opts.filter.serviceName = ITRS_NUMBER_SERVICE_NAME;    opts.callbackHandle = data;
    opts.waitTimeoutInSeconds = 2.0;
    opts.use = use_number;
    bool called = celix_bundleContext_useServiceWithOptions(data->ctx, &opts);
    if (!called) {
        printf("ITRS_NG_CELIX_SMOKE rc=%d service=unavailable\n", ENOENT);
        fflush(stdout);
    }
    return NULL;
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;
    celix_service_tracking_options_t guard = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    guard.filter.serviceName = ITRS_NUMBER_SERVICE_NAME;
    guard.callbackHandle = data;
    guard.add = probe_event_loop_guard;
    data->guard_tracker_id = celix_bundleContext_trackServicesWithOptions(ctx, &guard);
    data->thread_started = pthread_create(&data->thread, NULL, smoke_thread, data) == 0;
    return data->guard_tracker_id >= 0 && data->thread_started ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->thread_started) pthread_join(data->thread, NULL);
    if (data->guard_tracker_id >= 0) celix_bundleContext_stopTracker(ctx, data->guard_tracker_id);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
