# iTRS NG roadmap

## M0 — deterministic Number core

- [x] Separate authoritative PSAP routing from accessibility-resource selection.
- [x] Deterministic local → regional → bridge ordering.
- [x] Candidate eligibility reasons and provenance.
- [x] Fail-closed candidate capacity.
- [x] Synthetic failover tests preserving the PSAP invariant.

## M1 — Celix resource plane

- [x] `org.itrsng.number` service contract.
- [x] `org.itrsng.asl.resource` dynamic service vocabulary.
- [x] Number bundle tracks live ASL-resource services and snapshots them before resolution.
- [x] Synthetic Celix fixture providers and smoke consumer.
- [ ] Resource update/event semantics without transient ambiguity.
- [ ] MessagePack DFI/RSA transport for the Number service contract.

## M2 — synthetic NG911 harness

- [ ] Typed immutable input representing an already-resolved NG911/PSAP decision.
- [ ] Simulated ECRF/ESRP adapter; no live emergency network dependencies.
- [ ] Multi-party session graph: caller + PSAP + ASL resource + optional interpreter.
- [ ] RTT fallback policy fixture.
- [ ] Failure injection for local/regional resource loss.

## M3 — evidence and policy

- [ ] Versioned resolver policy documents rather than hard-coded baseline ordering.
- [ ] Canonical resolution record schema and digest.
- [ ] WindAnvil capability that executes the reference failover suite on an immutable source object.
- [ ] Replay identity proving the same snapshot yields the same resolution record.

## M4 — interoperability laboratory

- [ ] SIP/SDP fixture sessions.
- [ ] NG911/i3-shaped test messages where licensing and standards access permit.
- [ ] VRS/interpreter bridge adapter contract.
- [ ] Reference web console for visualizing the call-routing and communication-routing planes separately.
