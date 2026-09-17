#include "itrs_number_service.h"
#include "itrs_asl_resource_service.h"

#include <celix_bundle_activator.h>
#include <celix_compiler.h>
#include <celix_constants.h>
#include <celix_properties.h>
#include <celix_framework.h>
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
    pthread_mutex_t resource_mutex;
    pthread_mutex_t enum_mutex;
    tracked_resource_t resources[ITRS_CELIX_TRACK_MAX];
    size_t resource_count;
    bool hard_overflow;
    itrs_enum_provider_service_t *enum_provider;
    long resource_tracker_id;
    long enum_provider_tracker_id;
    long number_service_id;
    itrs_number_service_t number_service;
} activator_data_t;

static void copy_prop(char *dst, size_t size, const celix_properties_t *props, const char *key) {
    snprintf(dst, size, "%s", celix_properties_get(props, key, ""));
}

static void add_resource(void *handle, void *svc CELIX_UNUSED, const celix_properties_t *props) {
    activator_data_t *data = handle;
    long service_id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    pthread_mutex_lock(&data->resource_mutex);
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
    pthread_mutex_unlock(&data->resource_mutex);
}

static void remove_resource(void *handle, void *svc CELIX_UNUSED, const celix_properties_t *props) {
    activator_data_t *data = handle;
    long service_id = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    pthread_mutex_lock(&data->resource_mutex);
    for (size_t i = 0; i < data->resource_count; ++i) {
        if (data->resources[i].service_id == service_id) {
            data->resources[i] = data->resources[data->resource_count - 1];
            data->resource_count--;
            break;
        }
    }
    pthread_mutex_unlock(&data->resource_mutex);
}

static int resolve_service(void *handle, const itrs_number_request_t *request, itrs_number_result_t *result) {
    activator_data_t *data = handle;
    itrs_asl_resource_t snapshot[ITRS_NUMBER_MAX_CANDIDATES];
    size_t count;
    bool blocked;
    pthread_mutex_lock(&data->resource_mutex);
    count = data->resource_count;
    blocked = data->hard_overflow || count > ITRS_NUMBER_MAX_CANDIDATES;
    if (!blocked) {
        for (size_t i = 0; i < count; ++i) snapshot[i] = data->resources[i].resource;
    }
    pthread_mutex_unlock(&data->resource_mutex);
    if (blocked) return EOVERFLOW;
    return itrs_number_resolve(request, snapshot, count, result);
}


static void set_enum_provider(void *handle, void *svc) {
    activator_data_t *data = handle;
    pthread_mutex_lock(&data->enum_mutex);
    data->enum_provider = svc;
    pthread_mutex_unlock(&data->enum_mutex);
}

static int resolve_e164_service(void *handle, const char *e164, const char *apex,
                                itrs_number_enum_result_t *result) {
    activator_data_t *data = handle;
    if (data == NULL || e164 == NULL || result == NULL) return EINVAL;
    if (celix_framework_isCurrentThreadTheEventLoop(celix_bundleContext_getFramework(data->ctx))) {
        return EWOULDBLOCK;
    }
    const char *effective_apex = (apex == NULL || apex[0] == '\0') ? ITRS_ENUM_DEFAULT_APEX : apex;
    itrs_enum_observation_t observation = {0};

    pthread_mutex_lock(&data->enum_mutex);
    itrs_enum_provider_service_t *provider = data->enum_provider;
    if (provider == NULL || provider->lookup == NULL) {
        pthread_mutex_unlock(&data->enum_mutex);
        return ENOENT;
    }
    int rc = provider->lookup(provider->handle, e164, effective_apex, &observation);
    pthread_mutex_unlock(&data->enum_mutex);
    if (rc != 0) return rc;

    memset(result, 0, sizeof(*result));
    rc = itrs_enum_resolve_sip(e164, effective_apex, observation.records,
                               observation.record_count, &result->resolution);
    if (rc != 0) return rc;
    snprintf(result->provider_source, sizeof(result->provider_source), "%s", observation.source);
    result->observed_at_unix_ms = observation.observed_at_unix_ms;
    result->min_ttl_seconds = observation.min_ttl_seconds;
    result->authenticated_data = observation.authenticated_data;
    result->observed_record_count = observation.record_count;
    return 0;
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;
    data->resource_count = 0;
    data->hard_overflow = false;
    data->enum_provider = NULL;
    pthread_mutex_init(&data->resource_mutex, NULL);
    pthread_mutex_init(&data->enum_mutex, NULL);

    celix_service_tracking_options_t resource_track = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    resource_track.filter.serviceName = ITRS_ASL_RESOURCE_SERVICE_NAME;
    resource_track.callbackHandle = data;
    resource_track.addWithProperties = add_resource;
    resource_track.removeWithProperties = remove_resource;
    data->resource_tracker_id = celix_bundleContext_trackServicesWithOptions(ctx, &resource_track);

    celix_service_tracking_options_t enum_track = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    enum_track.filter.serviceName = ITRS_ENUM_PROVIDER_SERVICE_NAME;
    enum_track.callbackHandle = data;
    enum_track.set = set_enum_provider;
    data->enum_provider_tracker_id = celix_bundleContext_trackServicesWithOptions(ctx, &enum_track);

    data->number_service.handle = data;
    data->number_service.resolve = resolve_service;
    data->number_service.resolveE164 = resolve_e164_service;
    data->number_service_id = celix_bundleContext_registerService(ctx, &data->number_service, ITRS_NUMBER_SERVICE_NAME, NULL);
    return data->resource_tracker_id < 0 || data->enum_provider_tracker_id < 0 || data->number_service_id < 0
        ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->resource_tracker_id >= 0) celix_bundleContext_stopTracker(ctx, data->resource_tracker_id);
    if (data->enum_provider_tracker_id >= 0) celix_bundleContext_stopTracker(ctx, data->enum_provider_tracker_id);
    if (data->number_service_id >= 0) celix_bundleContext_unregisterService(ctx, data->number_service_id);
    pthread_mutex_destroy(&data->enum_mutex);
    pthread_mutex_destroy(&data->resource_mutex);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
