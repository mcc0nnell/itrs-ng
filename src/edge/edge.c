#include "itrsng/edge.h"
#include "itrsng/number.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define EDGE_RESOURCE_FIELDS 13u
#define EDGE_REQUEST_FIELDS 9u
#define EDGE_RESULT_FIELDS 11u
#define EDGE_CANDIDATE_FIELDS 10u

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t offset;
} mp_reader_t;

typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t length;
    int error;
} mp_writer_t;

static int rd_byte(mp_reader_t *r, uint8_t *out) {
    if (!r || !out || r->offset >= r->size) return EINVAL;
    *out = r->data[r->offset++];
    return 0;
}

static int rd_be16(mp_reader_t *r, uint16_t *out) {
    if (!r || !out || r->size - r->offset < 2) return EINVAL;
    *out = (uint16_t)(((uint16_t)r->data[r->offset] << 8) |
                      (uint16_t)r->data[r->offset + 1]);
    r->offset += 2;
    return 0;
}

static int rd_be32(mp_reader_t *r, uint32_t *out) {
    if (!r || !out || r->size - r->offset < 4) return EINVAL;
    *out = ((uint32_t)r->data[r->offset] << 24) |
           ((uint32_t)r->data[r->offset + 1] << 16) |
           ((uint32_t)r->data[r->offset + 2] << 8) |
           (uint32_t)r->data[r->offset + 3];
    r->offset += 4;
    return 0;
}

static int rd_array(mp_reader_t *r, uint32_t *count) {
    uint8_t tag = 0;
    int rc = rd_byte(r, &tag);
    if (rc != 0) return rc;
    if ((tag & 0xf0u) == 0x90u) {
        *count = tag & 0x0fu;
        return 0;
    }
    if (tag == 0xdcu) {
        uint16_t n = 0;
        rc = rd_be16(r, &n);
        *count = n;
        return rc;
    }
    if (tag == 0xddu) return rd_be32(r, count);
    return EINVAL;
}

static int rd_string(mp_reader_t *r, char *dst, size_t dst_size) {
    uint8_t tag = 0;
    uint32_t len = 0;
    int rc = rd_byte(r, &tag);
    if (rc != 0) return rc;

    if ((tag & 0xe0u) == 0xa0u) {
        len = tag & 0x1fu;
    } else if (tag == 0xd9u) {
        uint8_t n = 0;
        rc = rd_byte(r, &n);
        if (rc != 0) return rc;
        len = n;
    } else if (tag == 0xdau) {
        uint16_t n = 0;
        rc = rd_be16(r, &n);
        if (rc != 0) return rc;
        len = n;
    } else {
        return EINVAL;
    }

    if (len + 1u > dst_size || r->size - r->offset < len) return EOVERFLOW;
    for (uint32_t i = 0; i < len; ++i) {
        if (r->data[r->offset + i] == 0) return EINVAL;
    }
    memcpy(dst, r->data + r->offset, len);
    dst[len] = '\0';
    r->offset += len;
    return 0;
}

static int rd_bool(mp_reader_t *r, bool *value) {
    uint8_t tag = 0;
    int rc = rd_byte(r, &tag);
    if (rc != 0) return rc;
    if (tag == 0xc2u) {
        *value = false;
        return 0;
    }
    if (tag == 0xc3u) {
        *value = true;
        return 0;
    }
    return EINVAL;
}

static int rd_i32(mp_reader_t *r, int32_t *value) {
    uint8_t tag = 0;
    int rc = rd_byte(r, &tag);
    if (rc != 0) return rc;

    if (tag <= 0x7fu) {
        *value = (int32_t)tag;
        return 0;
    }
    if (tag >= 0xe0u) {
        *value = (int8_t)tag;
        return 0;
    }
    if (tag == 0xccu) {
        uint8_t n = 0;
        rc = rd_byte(r, &n);
        *value = (int32_t)n;
        return rc;
    }
    if (tag == 0xcdu) {
        uint16_t n = 0;
        rc = rd_be16(r, &n);
        *value = (int32_t)n;
        return rc;
    }
    if (tag == 0xceu) {
        uint32_t n = 0;
        rc = rd_be32(r, &n);
        if (rc != 0 || n > INT32_MAX) return EOVERFLOW;
        *value = (int32_t)n;
        return 0;
    }
    if (tag == 0xd0u) {
        uint8_t n = 0;
        rc = rd_byte(r, &n);
        *value = (int8_t)n;
        return rc;
    }
    if (tag == 0xd1u) {
        uint16_t n = 0;
        rc = rd_be16(r, &n);
        *value = (int16_t)n;
        return rc;
    }
    if (tag == 0xd2u) {
        uint32_t n = 0;
        rc = rd_be32(r, &n);
        *value = (int32_t)n;
        return rc;
    }
    return EINVAL;
}

static void wr_byte(mp_writer_t *w, uint8_t v) {
    if (!w || w->error != 0) return;
    if (w->length >= w->capacity) {
        w->error = EOVERFLOW;
        return;
    }
    w->data[w->length++] = v;
}

static void wr_be16(mp_writer_t *w, uint16_t v) {
    wr_byte(w, (uint8_t)(v >> 8));
    wr_byte(w, (uint8_t)v);
}

static void wr_be32(mp_writer_t *w, uint32_t v) {
    wr_byte(w, (uint8_t)(v >> 24));
    wr_byte(w, (uint8_t)(v >> 16));
    wr_byte(w, (uint8_t)(v >> 8));
    wr_byte(w, (uint8_t)v);
}

static void wr_array(mp_writer_t *w, uint32_t count) {
    if (count <= 15u) {
        wr_byte(w, (uint8_t)(0x90u | count));
    } else if (count <= UINT16_MAX) {
        wr_byte(w, 0xdcu);
        wr_be16(w, (uint16_t)count);
    } else {
        wr_byte(w, 0xddu);
        wr_be32(w, count);
    }
}

static void wr_string(mp_writer_t *w, const char *s) {
    size_t len = s ? strlen(s) : 0u;
    if (len <= 31u) {
        wr_byte(w, (uint8_t)(0xa0u | len));
    } else if (len <= UINT8_MAX) {
        wr_byte(w, 0xd9u);
        wr_byte(w, (uint8_t)len);
    } else if (len <= UINT16_MAX) {
        wr_byte(w, 0xdau);
        wr_be16(w, (uint16_t)len);
    } else {
        w->error = EOVERFLOW;
        return;
    }
    if (w->error != 0) return;
    if (w->capacity - w->length < len) {
        w->error = EOVERFLOW;
        return;
    }
    memcpy(w->data + w->length, s, len);
    w->length += len;
}

static void wr_bool(mp_writer_t *w, bool v) {
    wr_byte(w, v ? 0xc3u : 0xc2u);
}

static void wr_i32(mp_writer_t *w, int32_t v) {
    if (v >= 0 && v <= 127) {
        wr_byte(w, (uint8_t)v);
    } else if (v >= -32 && v < 0) {
        wr_byte(w, (uint8_t)(int8_t)v);
    } else if (v >= 0 && v <= UINT8_MAX) {
        wr_byte(w, 0xccu);
        wr_byte(w, (uint8_t)v);
    } else if (v >= 0 && v <= UINT16_MAX) {
        wr_byte(w, 0xcdu);
        wr_be16(w, (uint16_t)v);
    } else if (v >= INT8_MIN && v <= INT8_MAX) {
        wr_byte(w, 0xd0u);
        wr_byte(w, (uint8_t)(int8_t)v);
    } else if (v >= INT16_MIN && v <= INT16_MAX) {
        wr_byte(w, 0xd1u);
        wr_be16(w, (uint16_t)(int16_t)v);
    } else {
        wr_byte(w, 0xd2u);
        wr_be32(w, (uint32_t)v);
    }
}

static int read_request(mp_reader_t *r, itrs_number_request_t *request) {
    uint32_t count = 0;
    int rc = rd_array(r, &count);
    if (rc != 0 || count != EDGE_REQUEST_FIELDS) return EINVAL;
    memset(request, 0, sizeof(*request));
    if ((rc = rd_string(r, request->request_id, sizeof(request->request_id))) != 0) return rc;
    if ((rc = rd_string(r, request->service, sizeof(request->service))) != 0) return rc;
    if ((rc = rd_string(r, request->language, sizeof(request->language))) != 0) return rc;
    if ((rc = rd_string(r, request->media, sizeof(request->media))) != 0) return rc;
    if ((rc = rd_string(r, request->jurisdiction_path, sizeof(request->jurisdiction_path))) != 0) return rc;
    if ((rc = rd_string(r, request->authoritative_psap_id, sizeof(request->authoritative_psap_id))) != 0) return rc;
    if ((rc = rd_string(r, request->authoritative_psap_endpoint, sizeof(request->authoritative_psap_endpoint))) != 0) return rc;
    if ((rc = rd_string(r, request->resource_snapshot, sizeof(request->resource_snapshot))) != 0) return rc;
    if ((rc = rd_string(r, request->policy_version, sizeof(request->policy_version))) != 0) return rc;
    if (!request->request_id[0] || !request->service[0] || !request->language[0] ||
        !request->media[0] || !request->jurisdiction_path[0] ||
        !request->authoritative_psap_id[0] || !request->authoritative_psap_endpoint[0] ||
        !request->resource_snapshot[0] || !request->policy_version[0]) {
        return EINVAL;
    }
    return 0;
}

static void write_request(mp_writer_t *w, const itrs_number_request_t *request) {
    wr_array(w, EDGE_REQUEST_FIELDS);
    wr_string(w, request->request_id);
    wr_string(w, request->service);
    wr_string(w, request->language);
    wr_string(w, request->media);
    wr_string(w, request->jurisdiction_path);
    wr_string(w, request->authoritative_psap_id);
    wr_string(w, request->authoritative_psap_endpoint);
    wr_string(w, request->resource_snapshot);
    wr_string(w, request->policy_version);
}

static int read_resource(mp_reader_t *r, itrs_asl_resource_t *resource) {
    uint32_t count = 0;
    int32_t priority = 0;
    int rc = rd_array(r, &count);
    if (rc != 0 || count != EDGE_RESOURCE_FIELDS) return EINVAL;
    memset(resource, 0, sizeof(*resource));
    if ((rc = rd_string(r, resource->id, sizeof(resource->id))) != 0) return rc;
    if ((rc = rd_string(r, resource->endpoint, sizeof(resource->endpoint))) != 0) return rc;
    if ((rc = rd_string(r, resource->language, sizeof(resource->language))) != 0) return rc;
    if ((rc = rd_string(r, resource->media, sizeof(resource->media))) != 0) return rc;
    if ((rc = rd_string(r, resource->role, sizeof(resource->role))) != 0) return rc;
    if ((rc = rd_string(r, resource->scope, sizeof(resource->scope))) != 0) return rc;
    if ((rc = rd_string(r, resource->jurisdictions, sizeof(resource->jurisdictions))) != 0) return rc;
    if ((rc = rd_string(r, resource->services, sizeof(resource->services))) != 0) return rc;
    if ((rc = rd_string(r, resource->state, sizeof(resource->state))) != 0) return rc;
    if ((rc = rd_string(r, resource->authority_source, sizeof(resource->authority_source))) != 0) return rc;
    if ((rc = rd_string(r, resource->authority_version, sizeof(resource->authority_version))) != 0) return rc;
    if ((rc = rd_i32(r, &priority)) != 0) return rc;
    resource->priority = priority;
    return rd_bool(r, &resource->dispatch_authority);
}

static void write_candidate(mp_writer_t *w, const itrs_number_candidate_t *candidate) {
    wr_array(w, EDGE_CANDIDATE_FIELDS);
    wr_string(w, candidate->id);
    wr_string(w, candidate->endpoint);
    wr_string(w, candidate->role);
    wr_string(w, candidate->scope);
    wr_string(w, candidate->authority_source);
    wr_string(w, candidate->authority_version);
    wr_bool(w, candidate->eligible);
    wr_i32(w, candidate->class_rank);
    wr_i32(w, candidate->priority);
    wr_i32(w, (int32_t)candidate->reason_mask);
}

static void write_result(mp_writer_t *w, const itrs_number_result_t *result) {
    wr_array(w, EDGE_RESULT_FIELDS);
    wr_string(w, result->request_id);
    wr_string(w, result->authoritative_psap_id);
    wr_string(w, result->authoritative_psap_endpoint);
    wr_string(w, result->resource_snapshot);
    wr_string(w, result->policy_version);
    wr_bool(w, result->selected);
    wr_string(w, result->selected_id);
    wr_string(w, result->selected_endpoint);
    wr_string(w, result->selected_role);
    wr_string(w, result->selected_scope);
    wr_array(w, (uint32_t)result->candidate_count);
    for (size_t i = 0; i < result->candidate_count; ++i) {
        write_candidate(w, &result->candidates[i]);
    }
}

static int write_fault(uint8_t *output,
                       size_t output_capacity,
                       size_t *output_len,
                       int code,
                       const char *message) {
    mp_writer_t w = {output, output_capacity, 0u, 0};
    wr_array(&w, 4u);
    wr_i32(&w, (int32_t)ITRS_EDGE_WIRE_VERSION);
    wr_i32(&w, ITRS_EDGE_OP_FAULT);
    wr_i32(&w, code);
    wr_string(&w, message);
    if (w.error != 0) return w.error;
    *output_len = w.length;
    return 0;
}

static int read_prefix(mp_reader_t *r, uint32_t expected_count, int32_t expected_opcode) {
    uint32_t count = 0;
    int32_t version = 0;
    int32_t opcode = 0;
    int rc = rd_array(r, &count);
    if (rc != 0 || count != expected_count) return EINVAL;
    if ((rc = rd_i32(r, &version)) != 0 || version != (int32_t)ITRS_EDGE_WIRE_VERSION) return EINVAL;
    if ((rc = rd_i32(r, &opcode)) != 0 || opcode != expected_opcode) return EINVAL;
    return 0;
}

int itrs_edge_step(const uint8_t *input,
                   size_t input_len,
                   uint8_t *output,
                   size_t output_capacity,
                   size_t *output_len) {
    if (!input || !output || !output_len || input_len == 0u || input_len > ITRS_EDGE_PAYLOAD_MAX) return EINVAL;

    mp_reader_t r = {input, input_len, 0u};
    itrs_number_request_t request;
    int rc = read_prefix(&r, 3u, ITRS_EDGE_OP_RESOLVE);
    if (rc == 0) rc = read_request(&r, &request);
    if (rc == 0 && r.offset != r.size) rc = EINVAL;
    if (rc != 0) return write_fault(output, output_capacity, output_len, rc, "invalid resolve envelope");

    mp_writer_t w = {output, output_capacity, 0u, 0};
    wr_array(&w, 5u);
    wr_i32(&w, (int32_t)ITRS_EDGE_WIRE_VERSION);
    wr_i32(&w, ITRS_EDGE_OP_EFFECT_LOOKUP);
    wr_string(&w, "service.lookup");
    wr_string(&w, "org.itrsng.asl.resource");
    write_request(&w, &request);
    if (w.error != 0) return w.error;
    *output_len = w.length;
    return 0;
}

int itrs_edge_resume(const uint8_t *input,
                     size_t input_len,
                     uint8_t *output,
                     size_t output_capacity,
                     size_t *output_len) {
    if (!input || !output || !output_len || input_len == 0u || input_len > ITRS_EDGE_PAYLOAD_MAX) return EINVAL;

    mp_reader_t r = {input, input_len, 0u};
    itrs_number_request_t request;
    itrs_asl_resource_t resources[ITRS_NUMBER_MAX_CANDIDATES];
    uint32_t resource_count = 0u;

    int rc = read_prefix(&r, 4u, ITRS_EDGE_OP_RESUME);
    if (rc == 0) rc = read_request(&r, &request);
    if (rc == 0) rc = rd_array(&r, &resource_count);
    if (rc == 0 && resource_count > ITRS_NUMBER_MAX_CANDIDATES) rc = EOVERFLOW;
    for (uint32_t i = 0; rc == 0 && i < resource_count; ++i) {
        rc = read_resource(&r, &resources[i]);
    }
    if (rc == 0 && r.offset != r.size) rc = EINVAL;
    if (rc != 0) return write_fault(output, output_capacity, output_len, rc, "invalid resume envelope");

    itrs_number_result_t result;
    rc = itrs_number_resolve(&request, resources, resource_count, &result);
    if (rc != 0) return write_fault(output, output_capacity, output_len, rc, "resolver rejected snapshot");

    mp_writer_t w = {output, output_capacity, 0u, 0};
    wr_array(&w, 3u);
    wr_i32(&w, (int32_t)ITRS_EDGE_WIRE_VERSION);
    wr_i32(&w, ITRS_EDGE_OP_DONE);
    write_result(&w, &result);
    if (w.error != 0) return w.error;
    *output_len = w.length;
    return 0;
}
