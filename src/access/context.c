#include "itrsng/access.h"

#include <string.h>

static bool one_of(const char *value, const char *const values[], size_t count) {
    if (!value || !value[0]) return false;
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(value, values[i]) == 0) return true;
    }
    return false;
}

bool itrs_access_context_is_valid(const itrs_access_context_t *context) {
    static const char *const source_kinds[] = {"device-os", "carrier-adapter", "synthetic", "other"};
    static const char *const transports[] = {"cellular", "wifi", "ethernet", "other", "unknown"};
    static const char *const profile_states[] = {"active", "inactive", "absent", "unknown"};
    static const char *const attachments[] = {"home", "roaming", "detached", "unknown"};
    static const char *const ims_states[] = {"registered", "unregistered", "unknown"};
    static const char *const access_states[] = {"available", "unavailable", "unknown"};
    static const char *const capability_names[] = {"voice", "rtt", "video", "text", "data"};
    static const char *const capability_states[] = {"available", "unavailable", "unknown"};
    static const char *const capability_bases[] = {"advertised", "observed", "configured", "synthetic", "unknown"};

    if (!context || !context->observation_id[0] || !context->observed_at[0] || !context->source_authority[0]) return false;
    if (!one_of(context->source_kind, source_kinds, sizeof(source_kinds) / sizeof(source_kinds[0]))) return false;
    if (!one_of(context->transport, transports, sizeof(transports) / sizeof(transports[0]))) return false;
    if (!one_of(context->emergency_access_state, access_states, sizeof(access_states) / sizeof(access_states[0]))) return false;
    if (context->capability_count > ITRS_ACCESS_MAX_CAPABILITIES) return false;

    if (strcmp(context->transport, "cellular") == 0) {
        if (!one_of(context->profile_state, profile_states, sizeof(profile_states) / sizeof(profile_states[0]))) return false;
        if (!one_of(context->attachment, attachments, sizeof(attachments) / sizeof(attachments[0]))) return false;
        if (!one_of(context->ims_state, ims_states, sizeof(ims_states) / sizeof(ims_states[0]))) return false;
    }

    for (size_t i = 0; i < context->capability_count; ++i) {
        const itrs_access_capability_t *cap = &context->capabilities[i];
        if (!one_of(cap->name, capability_names, sizeof(capability_names) / sizeof(capability_names[0]))) return false;
        if (!one_of(cap->state, capability_states, sizeof(capability_states) / sizeof(capability_states[0]))) return false;
        if (!one_of(cap->basis, capability_bases, sizeof(capability_bases) / sizeof(capability_bases[0]))) return false;
    }
    return true;
}
