# Provider Edge Reflex

This lab moves the iTRS/VRS routing decision out of provider-specific edge stacks and into a neutral, deterministic capability edge.

The central rule is:

```text
number -> capabilities -> direct if possible -> attach service resource only when needed
```

A VRS provider is therefore a **resource behind the edge**, not the authority that decides what the number means.

## Modes

- `direct`: the destination itself satisfies the requested video/ASL capability. No VRS resource is consulted or inserted.
- `interpreted`: the destination does not satisfy the caller's communication capability, so an eligible interpreter/bridge resource is attached.
- `emergency-accessibility`: the authoritative NG911/PSAP route is preserved and an accessibility resource is attached to that session.
- `unresolved`: no eligible path exists; the edge returns the evaluated candidates rather than silently guessing.

## Determinism

The resolver has no clock, network, or provider-specific API. All bindings, resource state, and policy are explicit inputs. Candidate ordering uses policy role/scope scores and stable resource IDs as the final tie-breaker. Each result includes a SHA-256 decision digest over canonicalized inputs and outputs.

## Why this matters

A phone number no longer has to mean "send the session to Provider X's edge." It can resolve to a set of communication capabilities and reachable endpoints. The edge can then select:

```text
Deaf caller -> direct Deaf endpoint
Deaf caller -> interpreter resource -> voice endpoint
Deaf caller -> authoritative PSAP + ASL resource
```

The same control-plane decision can drive the pure-WASM SIP twitch in `../wasm-sip-twitch`: the reflex decides *what topology is required*; the SIP kernel performs the signalling transitions.

## Run

```bash
node test.mjs
```
