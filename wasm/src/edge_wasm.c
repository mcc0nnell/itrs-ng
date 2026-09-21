#include "itrsng/edge.h"

#include <stddef.h>
#include <stdint.h>

#define ITRS_EDGE_WASM_OUTPUT_MAX 65536u

static uint8_t input_buffer[ITRS_EDGE_PAYLOAD_MAX];
static uint8_t output_buffer[ITRS_EDGE_WASM_OUTPUT_MAX];
static size_t last_output_len;

uint32_t itrs_edge_wasm_input_ptr(void) {
    return (uint32_t)(uintptr_t)input_buffer;
}

uint32_t itrs_edge_wasm_input_capacity(void) {
    return ITRS_EDGE_PAYLOAD_MAX;
}

uint32_t itrs_edge_wasm_output_ptr(void) {
    return (uint32_t)(uintptr_t)output_buffer;
}

uint32_t itrs_edge_wasm_output_capacity(void) {
    return ITRS_EDGE_WASM_OUTPUT_MAX;
}

uint32_t itrs_edge_wasm_output_len(void) {
    return (uint32_t)last_output_len;
}

int32_t itrs_edge_wasm_step(uint32_t input_len) {
    last_output_len = 0u;
    if (input_len > ITRS_EDGE_PAYLOAD_MAX) return -1;
    return itrs_edge_step(input_buffer,
                          input_len,
                          output_buffer,
                          ITRS_EDGE_WASM_OUTPUT_MAX,
                          &last_output_len);
}

int32_t itrs_edge_wasm_resume(uint32_t input_len) {
    last_output_len = 0u;
    if (input_len > ITRS_EDGE_PAYLOAD_MAX) return -1;
    return itrs_edge_resume(input_buffer,
                            input_len,
                            output_buffer,
                            ITRS_EDGE_WASM_OUTPUT_MAX,
                            &last_output_len);
}
