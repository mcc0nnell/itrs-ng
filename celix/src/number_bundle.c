#include "itrs_number_service.h"
#include "itrs_asl_resource_service.h"

#include <celix_bundle_activator.h>
#include <celix_compiler.h>
#include <celix_constants.h>
#include <celix_properties.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define ITRS_CELIX_TRACK_MAX 128

typedef struct tracked_resource {
    long service_id;
    itrs_asl_resource_t resource;
} tracked_resource_t;

typedef struct activator_data {
    celix_bundle_context_t *ctx;
    pthread_mutex_t mutex;
    tracked_resource_t resources[ITRS_CELIX_TRACK_MAX];
    size_t resource_count;
    bool hard_overflow;
    long tracker_id;
    long number_service_id;
    itrs_number_service_t number_service;
} activator_data_t;

static void copy_prop(char *dst, size_t size, const celix_properties_t *props, const char *key) {
    snprintf(dst, size, "%s", celix_properties_get(props, key, ""));
}

static void add_resource(void *handle, void *svc CELIX_UNUSED, const celix_properties_t *props) {
    activator_data_t *data = handle;
    long service_id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    pthread_mutex_lock(&data->mutex);
    if (data->resource_count < ITRS_CELIX_TRACK_MAX) {
        tracked_resource_t *tracked = &data->resources[data->resource_count++];
        memset(tracked, 0, sizeof(*tracked));
        tracked->service_id = service_id;
        copy_prop(tracked->resource.id, sizeof(tracked->resource.id), props, ITRS_PROP_RESOURCE_ID);
        copy_prop(tracked->resource.endpoint, sizeof(tracked->resource.endpoint), props, ITRS_PROP_ENDPOINT);
        copy_prop(tracked->resource.language, sizeof(tracked->resource.language), props, ITRS_PROP_LANGUAGE);
        copy_prop(tracked->resource.media, sizeof(tracked->resource.media), props, ITRS_PROP_MEDIA);
        copy_prop(tracked->resource.role, sizeof(tracked->resource.role), props, ITRS_PROP_ROLE);
        copy_prop(tracked->resource.scope, sizeof(tracked->resource.scope), props, ITRS_PROP_SCOPE);
        copy_prop(tracked->resource.jurisdictions, sizeof(tracked->resource.jurisdictions), props, ITRS_PROP_JURISDICTIONS);
        copy_prop(tracked->resource.services, sizeof(tracked->resource.services), props, ITRS_PROP_SERVICES);
        copy_prop(tracked->resource.state, sizeof(tracked->resource.state), props, ITRS_PROP_STATE);
        copy_prop(tracked->resource.authority_source, sizeof(tracked->resource.authority_source), props, ITRS_PROP_AUTHORITY_SOURCE);
        copy_prop(tracked->resource.authority_version, sizeof(tracked->resource.authority_version), props, ITRS_PROP_AUTHORITY_VERSION);
        tracked->resource.priority = (int)celix_properties_getAsLong(props, ITRS_PROP_PRIORITY, 0L);
        tracked->resource.dispatch_authority = celix_properties_getAsBool(props, ITRS_PROP_DISPATCH_AUTHORITY, false);
    } else {
        data->hard_overflow = true;
    }
    pthread_mutex_unlock(&data->mutex);
}

static void remove_resource(void *handle, void *svc CELIX_UNUSED, const celix_properties_t *props) {
    activator_data_t *data = handle;
    long service_id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    pthread_mutex_lock(&data->mutex);
    for (size_t i = 0; i < data->resource_count; ++i) {
        if (data->resources[i].service_id == service_id) {
            data->resources[i] = data->resources[data->resource_count - 1];
            data->resource_count--;
            break;
        }
    }
    pthread_mutex_unlock(&data->mutex);
}

static int resolve_service(void *handle, const itrs_number_request_t *request, itrs_number_result_t *result) {
    activator_data_t *data = handle;
    itrs_asl_resource_t snapshot[ITRS_NUMBER_MAX_CANDIDATES];
    size_t count;
    bool blocked;
    pthread_mutex_lock(&data->mutex);
    count = data->resource_count;
    blocked = data->hard_overflow || count > ITRS_NUMBER_MAX_CANDIDATES;
    if (!blocked) {
        for (size_t i = 0; i < count; ++i) snapshot[i] = data->resources[i].resource;
    }
    pthread_mutex_unlock(&data->mutex);
    if (blocked) return EOVERFLOW;
    return itrs_number_resolve(request, snapshot, count, result);
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;
    data->resource_count = 0;
    data->hard_overflow = false;
    pthread_mutex_init(&data->mutex, NULL);

    celix_service_tracking_options_t track = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    track.filter.serviceName = ITRS_ASL_RESOURCE_SERVICE_NAME;
    track.callbackHandle = data;
    track.addWithProperties = (void*)add_resource;
    track.removeWithProperties = (void*)remove_resource;
    data->tracker_id = celix_bundleContext_trackServicesWithOptions(ctx, &track);

    data->number_service.handle = data;
    data->number_service.resolve = resolve_service;
    data->number_service_id = celix_bundleContext_registerService(ctx, &data->number_service, ITRS_NUMBER_SERVICE_NAME, NULL);
    return data->tracker_id < 0 || data->number_service_id < 0 ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->tracker_id >= 0) celix_bundleContext_stopTracker(ctx, data->tracker_id);
    if (data->number_service_id >= 0) celix_bundleContext_unregisterService(ctx, data->number_service_id);
    pthread_mutex_destroy(&data->mutex);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
