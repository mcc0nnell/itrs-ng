# WASM SIP twitch

This lab proves the smallest useful iTRS Edge signalling reflex:

```
command or SIP bytes
        |
        v
  zero-import WASM
        |
        +--> SIP wire bytes
        +--> timer requests
        +--> typed events
        +--> state snapshot
```

The specimen pins sipx at commit
`cb71afd95a0fe2bf7405b45285cae17a2195b4d4` (v1.0.1) and builds its
sans-I/O browser kernel for `wasm32-unknown-unknown`.

No socket, clock, async runtime, filesystem, or entropy source exists inside
the module. The host supplies bytes, monotonic time, timer firings, and entropy.
That is the JAIN-SLEE-like "twitch" boundary: one event advances one bounded
telecom state machine and emits explicit effects.

## Run

```bash
./build.sh
```

The run first executes sipx's raw-WASM artifact harness, then drives an iTRS
registration reflex:

```
REGISTER
  -> 401 Digest challenge
  -> authenticated REGISTER (SHA-256)
  -> 200 OK
  -> registered
```

`evidence/twitch.json` records the pinned source commit, WASM hash and size,
zero-import assertion, wire-message hashes, emitted events, and final state.
It intentionally does not retain credentials or full SIP messages.

## Reproducibility boundary

The signalling behavior is deterministic under the same explicit inputs, but this lab does **not**
yet claim bit-for-bit identical WASM across arbitrary checkout roots. Source and Cargo paths are
remapped during compilation, yet a second checkout still produced a different module hash. Treat
the recorded WASM hash as evidence for the exact built artifact, not as a source-only derivation.

## Next seam

The host is deliberately replaceable. A Cloudflare Worker can provide WSS,
timers, monotonic time, and entropy while the identical WASM artifact remains
the signalling brain. iTRS-NG resolution and accessibility policy can be added
as adjacent deterministic reflex modules rather than folded into transport.
