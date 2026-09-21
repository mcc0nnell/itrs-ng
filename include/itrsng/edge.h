#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ITRS_EDGE_WIRE_VERSION 1u
#define ITRS_EDGE_PAYLOAD_MAX 16384u

typedef enum itrs_edge_opcode {
    ITRS_EDGE_OP_RESOLVE = 0,
    ITRS_EDGE_OP_EFFECT_LOOKUP = 1,
    ITRS_EDGE_OP_RESUME = 2,
    ITRS_EDGE_OP_DONE = 3,
    ITRS_EDGE_OP_FAULT = 255
} itrs_edge_opcode_t;

/**
 * Process a canonical MessagePack request.
 *
 * Input:  [1, 0, request]
 * Output: [1, 1, "service.lookup", "org.itrsng.asl.resource", request]
 *
 * The kernel performs no I/O and retains no continuation state.
 */
int itrs_edge_step(const uint8_t *input,
                   size_t input_len,
                   uint8_t *output,
                   size_t output_capacity,
                   size_t *output_len);

/**
 * Resume after the host has satisfied the emitted service.lookup effect.
 *
 * Input:  [1, 2, request, resources]
 * Output: [1, 3, result] or [1, 255, errno, message]
 */
int itrs_edge_resume(const uint8_t *input,
                     size_t input_len,
                     uint8_t *output,
                     size_t output_capacity,
                     size_t *output_len);

#ifdef __cplusplus
}
#endif
