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
    │ device / network access
    ▼
Access Identity ── eSIM / carrier / IMS observations
    │
    │ typed, privacy-minimized context
    ▼
Tilden Number
    │
    ├──────── iTRS NG accessibility resolution
    │
    ▼
Local PSAP ──────────────── incident + dispatch authority
    │
    ├── local ASL resource
    ├── regional ASL resource
    └── interpreter / VRS fallback
```

The local PSAP does not need to surrender the emergency call merely because the caller communicates in ASL. The accessibility resource can be discovered and joined as another participant in the session.

Access Identity is deliberately non-authoritative: eSIM or carrier observations can inform Tilden resolution, but they do not select the PSAP, create number authority, or prove that a communications modality is usable.

## Architecture

- **Access Identity** — privacy-minimized observation of device/network access, including eSIM-backed cellular state; never numbering or NG911 authority.
- **Tilden Number** — identifier, authority, capability, reachability, policy, and provenance resolution.
- **ASL resource registry** — live discovery of local, regional, or fallback ASL-capable resources.
- **NG911 integration boundary** — complements ECRF/ESRP policy; does not replace geographic emergency routing.
- **Media/session plane** — SIP, video, RTT, voice, and multi-party session establishment.
- **Policy + provenance** — deterministic selection with an auditable explanation of why a resource was eligible and selected.
- **Celix runtime** — small replaceable services with dynamic discovery and a path to distributed execution.

The access seam preserves a simple three-stage claim boundary:

```text
Access Identity: access observed
Tilden Number:   capability authorized / resolved
Baudot:          communication behavior observed / proven
```

## First proof

The first concrete prototype should demonstrate deterministic failover across:

1. local ASL telecommunicator;
2. regional ASL emergency resource;
3. interpreter/VRS fallback;

while preserving the geographically authoritative PSAP throughout the call.

The access-identity slice should replay that resolution under active/home, roaming, inactive, absent, and unknown eSIM/access states without allowing those states to suppress or replace the authoritative emergency-routing result.

See [`docs/architecture.md`](docs/architecture.md), [`docs/access-identity.md`](docs/access-identity.md), [`spec/asl-resource-resolution.md`](spec/asl-resource-resolution.md), and [`spec/access-identity.md`](spec/access-identity.md).

## Status

Research and reference architecture. **Not production emergency-call software.** The repository is intended to make interfaces, policy boundaries, resolution semantics, and interoperability assumptions concrete enough to test.