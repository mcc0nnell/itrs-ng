#include "itrsng/edge.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    size_t cap;
    size_t len;
} tw_t;

typedef struct {
    const uint8_t *data;
    size_t len;
    size_t off;
} tr_t;

static void wb(tw_t *w, uint8_t b) {
    assert(w->len < w->cap);
    w->data[w->len++] = b;
}

static void wa(tw_t *w, uint32_t n) {
    assert(n <= 15u);
    wb(w, (uint8_t)(0x90u | n));
}

static void ws(tw_t *w, const char *s) {
    size_t n = strlen(s);
    assert(n <= 255u);
    if (n <= 31u) wb(w, (uint8_t)(0xa0u | n));
    else {
        wb(w, 0xd9u);
        wb(w, (uint8_t)n);
    }
    assert(w->cap - w->len >= n);
    memcpy(w->data + w->len, s, n);
    w->len += n;
}

static void wi(tw_t *w, int v) {
    if (v >= 0 && v <= 127) wb(w, (uint8_t)v);
    else {
        assert(v >= 0 && v <= 255);
        wb(w, 0xccu);
        wb(w, (uint8_t)v);
    }
}

static void wbool(tw_t *w, bool v) {
    wb(w, v ? 0xc3u : 0xc2u);
}

static void write_request(tw_t *w) {
    wa(w, 9u);
    ws(w, "req-edge-1");
    ws(w, "urn:service:sos");
    ws(w, "ASL");
    ws(w, "video");
    ws(w, "MD,Frederick");
    ws(w, "psap-frederick");
    ws(w, "sip:psap-frederick@example.invalid");
    ws(w, "snapshot-edge-1");
    ws(w, "policy-edge-1");
}

static void write_resource(tw_t *w,
                           const char *id,
                           const char *role,
                           const char *scope,
                           const char *jurisdictions,
                           const char *state,
                           int priority) {
    wa(w, 13u);
    ws(w, id);
    ws(w, "sip:resource@example.invalid");
    ws(w, "ASL");
    ws(w, "video,rtt");
    ws(w, role);
    ws(w, scope);
    ws(w, jurisdictions);
    ws(w, "urn:service:sos");
    ws(w, state);
    ws(w, "synthetic-registry");
    ws(w, "2026-09-20");
    wi(w, priority);
    wbool(w, false);
}

static uint8_t rb(tr_t *r) {
    assert(r->off < r->len);
    return r->data[r->off++];
}

static uint32_t ra(tr_t *r) {
    uint8_t t = rb(r);
    assert((t & 0xf0u) == 0x90u);
    return t & 0x0fu;
}

static int ri(tr_t *r) {
    uint8_t t = rb(r);
    if (t <= 0x7fu) return t;
    assert(t == 0xccu);
    return rb(r);
}

static void rs(tr_t *r, char *dst, size_t cap) {
    uint8_t t = rb(r);
    size_t n = 0u;
    if ((t & 0xe0u) == 0xa0u) n = t & 0x1fu;
    else {
        assert(t == 0xd9u);
        n = rb(r);
    }
    assert(n + 1u <= cap);
    assert(r->len - r->off >= n);
    memcpy(dst, r->data + r->off, n);
    dst[n] = '\0';
    r->off += n;
}

static void skip_string(tr_t *r) {
    char tmp[256];
    rs(r, tmp, sizeof(tmp));
}

static bool rbool(tr_t *r) {
    uint8_t t = rb(r);
    assert(t == 0xc2u || t == 0xc3u);
    return t == 0xc3u;
}

static void assert_done_selected(const uint8_t *buf, size_t len, const char *expected_id) {
    tr_t r = {buf, len, 0u};
    assert(ra(&r) == 3u);
    assert(ri(&r) == 1);
    assert(ri(&r) == ITRS_EDGE_OP_DONE);
    assert(ra(&r) == 11u);

    skip_string(&r);
    char psap[128];
    rs(&r, psap, sizeof(psap));
    assert(strcmp(psap, "psap-frederick") == 0);
    skip_string(&r);
    skip_string(&r);
    skip_string(&r);
    assert(rbool(&r));

    char selected[128];
    rs(&r, selected, sizeof(selected));
    assert(strcmp(selected, expected_id) == 0);
}

static size_t build_resume(uint8_t *buf, size_t cap, bool local_available, bool reverse) {
    tw_t w = {buf, cap, 0u};
    wa(&w, 4u);
    wi(&w, 1);
    wi(&w, ITRS_EDGE_OP_RESUME);
    write_request(&w);
    wa(&w, 3u);

    if (!reverse) {
        write_resource(&w, "asl-local", "telecommunicator", "local", "MD,Frederick",
                       local_available ? "available" : "unavailable", 100);
        write_resource(&w, "asl-regional", "telecommunicator", "regional", "MD", "available", 80);
        write_resource(&w, "vrs-bridge", "bridge", "national", "*", "available", 60);
    } else {
        write_resource(&w, "vrs-bridge", "bridge", "national", "*", "available", 60);
        write_resource(&w, "asl-regional", "telecommunicator", "regional", "MD", "available", 80);
        write_resource(&w, "asl-local", "telecommunicator", "local", "MD,Frederick",
                       local_available ? "available" : "unavailable", 100);
    }
    return w.len;
}

static void test_step_is_effect(void) {
    uint8_t input[1024];
    uint8_t output[2048];
    tw_t w = {input, sizeof(input), 0u};
    wa(&w, 3u);
    wi(&w, 1);
    wi(&w, ITRS_EDGE_OP_RESOLVE);
    write_request(&w);

    size_t out_len = 0u;
    assert(itrs_edge_step(input, w.len, output, sizeof(output), &out_len) == 0);

    tr_t r = {output, out_len, 0u};
    assert(ra(&r) == 5u);
    assert(ri(&r) == 1);
    assert(ri(&r) == ITRS_EDGE_OP_EFFECT_LOOKUP);
    char effect[64];
    char service[64];
    rs(&r, effect, sizeof(effect));
    rs(&r, service, sizeof(service));
    assert(strcmp(effect, "service.lookup") == 0);
    assert(strcmp(service, "org.itrsng.asl.resource") == 0);
}

static void test_resume_failover_and_determinism(void) {
    uint8_t input_a[4096];
    uint8_t input_b[4096];
    uint8_t output_a[8192];
    uint8_t output_b[8192];
    size_t output_a_len = 0u;
    size_t output_b_len = 0u;

    size_t input_a_len = build_resume(input_a, sizeof(input_a), true, false);
    size_t input_b_len = build_resume(input_b, sizeof(input_b), true, true);
    assert(itrs_edge_resume(input_a, input_a_len, output_a, sizeof(output_a), &output_a_len) == 0);
    assert(itrs_edge_resume(input_b, input_b_len, output_b, sizeof(output_b), &output_b_len) == 0);
    assert_done_selected(output_a, output_a_len, "asl-local");
    assert(output_a_len == output_b_len);
    assert(memcmp(output_a, output_b, output_a_len) == 0);

    input_a_len = build_resume(input_a, sizeof(input_a), false, false);
    assert(itrs_edge_resume(input_a, input_a_len, output_a, sizeof(output_a), &output_a_len) == 0);
    assert_done_selected(output_a, output_a_len, "asl-regional");
}

static void test_fault_closed(void) {
    uint8_t bad[] = {0x93u, 0x01u, 0x00u, 0x90u};
    uint8_t output[256];
    size_t output_len = 0u;
    assert(itrs_edge_step(bad, sizeof(bad), output, sizeof(output), &output_len) == 0);

    tr_t r = {output, output_len, 0u};
    assert(ra(&r) == 4u);
    assert(ri(&r) == 1);
    assert(ri(&r) == ITRS_EDGE_OP_FAULT);
    assert(ri(&r) == EINVAL);
}

int main(void) {
    test_step_is_effect();
    test_resume_failover_and_determinism();
    test_fault_closed();
    puts("itrs-edge-test: ok");
    return 0;
}
