#include "itrs_asl_resource_service.h"

#include <celix_bundle_activator.h>
#include <celix_properties.h>

#include <stdio.h>

typedef struct activator_data {
    itrs_asl_resource_marker_service_t markers[3];
    long service_ids[3];
} activator_data_t;

static long register_resource(celix_bundle_context_t *ctx, itrs_asl_resource_marker_service_t *marker,
                              const char *id, const char *endpoint, const char *role, const char *scope,
                              const char *jurisdictions, int priority) {
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, ITRS_PROP_RESOURCE_ID, id);
    celix_properties_set(props, ITRS_PROP_ENDPOINT, endpoint);
    celix_properties_set(props, ITRS_PROP_LANGUAGE, "ASL");
    celix_properties_set(props, ITRS_PROP_MEDIA, "video,rtt");
    celix_properties_set(props, ITRS_PROP_ROLE, role);
    celix_properties_set(props, ITRS_PROP_SCOPE, scope);
    celix_properties_set(props, ITRS_PROP_JURISDICTIONS, jurisdictions);
    celix_properties_set(props, ITRS_PROP_SERVICES, "urn:service:sos");
    celix_properties_set(props, ITRS_PROP_STATE, "available");
    celix_properties_set(props, ITRS_PROP_AUTHORITY_SOURCE, "celix-fixture-registry");
    celix_properties_set(props, ITRS_PROP_AUTHORITY_VERSION, "fixture-v1");
    celix_properties_setLong(props, ITRS_PROP_PRIORITY, priority);
    celix_properties_setBool(props, ITRS_PROP_DISPATCH_AUTHORITY, false);
    return celix_bundleContext_registerService(ctx, marker, ITRS_ASL_RESOURCE_SERVICE_NAME, props);
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->service_ids[0] = register_resource(ctx, &data->markers[0], "asl-local", "sip:asl-local@example.invalid",
                                             "telecommunicator", "local", "US-MD-FREDERICK", 100);
    data->service_ids[1] = register_resource(ctx, &data->markers[1], "asl-regional", "sip:asl-regional@example.invalid",
                                             "telecommunicator", "regional", "US-MD", 90);
    data->service_ids[2] = register_resource(ctx, &data->markers[2], "vrs-bridge", "sip:vrs-bridge@example.invalid",
                                             "bridge", "national", "US", 10);
    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    for (size_t i = 0; i < 3; ++i) if (data->service_ids[i] >= 0) celix_bundleContext_unregisterService(ctx, data->service_ids[i]);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
