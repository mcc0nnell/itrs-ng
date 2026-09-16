#pragma once

#define ITRS_ASL_RESOURCE_SERVICE_NAME "org.itrsng.asl.resource"
#define ITRS_ASL_RESOURCE_SERVICE_VERSION "1.0.0"

#define ITRS_PROP_RESOURCE_ID "itrs.resource.id"
#define ITRS_PROP_ENDPOINT "itrs.resource.endpoint"
#define ITRS_PROP_LANGUAGE "itrs.resource.language"
#define ITRS_PROP_MEDIA "itrs.resource.media"
#define ITRS_PROP_ROLE "itrs.resource.role"
#define ITRS_PROP_SCOPE "itrs.resource.scope"
#define ITRS_PROP_JURISDICTIONS "itrs.resource.jurisdictions"
#define ITRS_PROP_SERVICES "itrs.resource.services"
#define ITRS_PROP_STATE "itrs.resource.state"
#define ITRS_PROP_AUTHORITY_SOURCE "itrs.resource.authority.source"
#define ITRS_PROP_AUTHORITY_VERSION "itrs.resource.authority.version"
#define ITRS_PROP_PRIORITY "itrs.resource.priority"
#define ITRS_PROP_DISPATCH_AUTHORITY "itrs.resource.dispatch-authority"

typedef struct itrs_asl_resource_marker_service {
    void *handle;
} itrs_asl_resource_marker_service_t;
