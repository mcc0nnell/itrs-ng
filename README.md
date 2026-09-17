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
- **iTRS ENUM/NAPTR** — E.164 → `itrs.us` NAPTR → terminal `E2U+sip` resolution as a deterministic Number input.
- **Celix ENUM provider plane** — ranked `org.itrsng.enum.provider` services for deterministic fixtures or live system DNS, frozen before Number policy is applied.
- **ASL resource registry** — live discovery of local, regional, or fallback ASL-capable resources.
- **NG911 integration boundary** — complements ECRF/ESRP policy; does not replace geographic emergency routing.
- **Media/session plane** — SIP, video, RTT, voice, and multi-party session establishment.
- **Policy + provenance** — deterministic selection with an auditable explanation of why a resource was eligible and selected.
- **Celix runtime** — small replaceable services with dynamic discovery and a path to distributed execution.

## Quick start

```bash
./scripts/validate.sh
```

The validation builds with warnings as errors, runs the deterministic resolver tests, exercises the local/regional/bridge failover chain, and checks that the authoritative PSAP is unchanged in all three runs.

To compile the Celix application against a pinned Apache Celix checkout:

```bash
cmake -S . -B build-celix -DITRS_ENABLE_CELIX=ON -DITRS_CELIX_SOURCE_DIR=/path/to/apache-celix
cmake --build build-celix --parallel
```

Or include the full Celix smoke in the normal validator:

```bash
ITRS_CELIX_SOURCE_DIR=/path/to/apache-celix ./scripts/validate.sh
```

See [`docs/CURRENT_CAPABILITIES.md`](docs/CURRENT_CAPABILITIES.md), [`ROADMAP.md`](ROADMAP.md), and [`docs/windanvil.md`](docs/windanvil.md).

## First proof

The first executable prototype demonstrates deterministic failover across:

1. local ASL telecommunicator;
2. regional ASL emergency resource;
3. interpreter/VRS fallback;

while preserving the geographically authoritative PSAP throughout the call.

See [`docs/architecture.md`](docs/architecture.md), [`docs/enum-naptr.md`](docs/enum-naptr.md), and [`spec/asl-resource-resolution.md`](spec/asl-resource-resolution.md).

## Status

Research and reference architecture. **Not production emergency-call software.** The repository is intended to make interfaces, policy boundaries, resolution semantics, and interoperability assumptions concrete enough to test.