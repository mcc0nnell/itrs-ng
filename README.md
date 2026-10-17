# iTRS NG

**Next-generation interoperable Telecommunications Relay Services and NG911 architecture.**

`iTRS NG` explores a capability-aware, accessibility-native architecture for real-time communications across TRS, direct video, RTT, SIP, and NG911. The project treats accessibility as part of routing and service discovery rather than as a bolt-on after a call has already been established.

The original **Tilden** work remains the architectural lineage for the project's numbering and resolution layer. In iTRS NG, **Tilden Number** becomes a standalone Celix application responsible for typed, provenance-bearing resolution of identifiers into authorized communication capabilities.

## Core idea

```text
CALL ROUTING                    COMMUNICATION ROUTING
────────────                    ─────────────────────
Where is the emergency?         How does the caller communicate?
        │                                   │
        ▼                                   ▼
   NG911 ECRF/ESRP                      iTRS NG
        │                                   │
        ▼                                   ▼
   Local PSAP                    ASL / RTT / video / voice
```

NG911 remains authoritative for geographic emergency routing and PSAP selection. iTRS NG resolves and joins the communication resources needed to make that interaction accessible.

For an ASL video emergency call, the target model is:

```text
Deaf caller
    │
    │ video / SIP / NG911
    ▼
Local PSAP ──────────────── incident + dispatch authority
    │
    └── iTRS NG accessibility resolution
              │
              ▼
         Tilden Number
              │
       ┌──────┼─────────┐
       ▼      ▼         ▼
     local  regional  interpreter/
     ASL      ASL       VRS fallback
     rep      pool
```

The local PSAP does not need to surrender the emergency call merely because the caller communicates in ASL. The accessibility resource can be discovered and joined as another participant in the session.

## Architecture

- **Tilden Number** — identifier, authority, capability, reachability, policy, and provenance resolution.
- **ASL resource registry** — live discovery of local, regional, or fallback ASL-capable resources.
- **NG911 integration boundary** — complements ECRF/ESRP policy; does not replace geographic emergency routing.
- **Media/session plane** — SIP, video, RTT, voice, and multi-party session establishment.
- **Policy + provenance** — deterministic selection with an auditable explanation of why a resource was eligible and selected.
- **Celix runtime** — small replaceable services with dynamic discovery and a path to distributed execution.
- **[Baudot](https://github.com/mcc0nnell/baudot) assurance plane** — external, evidence-first execution of accessible-communications behavior across SIP, RTT, WebRTC, handoff, and controlled-network seams.

## Baudot assurance boundary

Baudot remains an independent project rather than being vendored into iTRS NG. The split is deliberate:

- **iTRS NG owns intent and authority**: numbering, capability resolution, accessibility-resource selection, routing policy, provenance, and session-join decisions.
- **Baudot owns executable behavior and evidence**: portable scenarios, T.140/RTT semantics, SIP/RFC 4103 behavior, handoff/readiness probes, implementation oracles, packet evidence, and independent reducers.

```text
iTRS NG decision / scenario
          │
          ▼
   Baudot test vocabulary
          │
          ▼
SIP / RTT / video / WebRTC / network specimens
          │
          ▼
   preserved observations
          │
          ▼
 independent Baudot reducers
          │
          ▼
 facts returned to iTRS NG / WindAnvil
```

This keeps production architecture and assurance architecture separate. iTRS NG can pin a Baudot commit for a WindAnvil campaign and consume its scenarios and evidence contracts without importing Baudot internals into the runtime.

See [`docs/baudot-assurance.md`](docs/baudot-assurance.md) for the integration contract.

## First proof

The first concrete prototype should demonstrate deterministic failover across:

1. local ASL telecommunicator;
2. regional ASL emergency resource;
3. interpreter/VRS fallback;

while preserving the geographically authoritative PSAP throughout the call.

Each failover arm should also be expressible as a Baudot scenario with preserved signaling/media observations and a terminal evidence reduction, so a routing decision and a usable communications path are tested as separate facts.

See [`docs/architecture.md`](docs/architecture.md), [`docs/baudot-assurance.md`](docs/baudot-assurance.md), and [`spec/asl-resource-resolution.md`](spec/asl-resource-resolution.md).

## Status

Research and reference architecture. **Not production emergency-call software.** The repository is intended to make interfaces, policy boundaries, resolution semantics, and interoperability assumptions concrete enough to test.