# iTRS NG architecture

iTRS NG is an accessibility-native real-time communications architecture spanning TRS, direct video, RTT, SIP, and NG911 integration.

## Design boundary

The central rule is simple: **emergency location routing and communication-capability routing are related but distinct decisions.**

NG911 components remain authoritative for routing an emergency to the appropriate public-safety domain. iTRS NG does not replace that authority. Instead, it resolves the communication resources needed for the caller and PSAP to interact effectively.

```text
Emergency ingress
      │
      ├── location + service routing ──► NG911 ECRF / ESRP ──► local PSAP
      │
      └── communication capability ────► iTRS NG ────────────► ASL / RTT / video resource
```

## Access Identity

Device and carrier state enters iTRS NG through a separate **Access Identity** service. eSIM/eUICC, carrier attachment, roaming, and IMS observations belong here rather than inside Number.

```text
device / carrier edge
        │
        │ eSIM / network observations
        ▼
 Access Identity
        │ access-context@1
        │ typed + provenance-bearing
        ▼
   Tilden Number
```

Access Identity is deliberately weaker than an authority service. It reports what was observed and where the observation came from. It does not create an E.164 ownership claim, authorize a Tilden endpoint, select a PSAP, or prove live media readiness.

The current architecture preserves the earlier Tilden invariant:

```text
NUMBER != NETWORK
```

The same communication identity can remain stable while the observed access edge changes from home cellular to roaming, Wi-Fi, no active profile, or an unknown state.

For emergency use, missing or negative access context must not become an accidental gate:

```text
location / service context ─────────────► NG911 authority ─► PSAP

access observation ─► Access Identity ─► Tilden Number ───► accessibility resources
```

Access state can improve a resolution decision, but failure of Access Identity does not suppress the authoritative emergency path and does not erase already-known accessibility requirements.

See [`access-identity.md`](access-identity.md) and [`../spec/access-identity.md`](../spec/access-identity.md).

## Tilden Number

Tilden survives inside iTRS NG as the numbering and capability-resolution subsystem. **Number is a standalone Celix application**, not a helper library hidden inside signaling.

```text
Tilden Number
├── number-core
├── number-policy
├── number-resolver
├── number-store
├── number-events
└── number-api
```

Number owns the semantics of an addressable identifier and the authorized bindings around it. Consumers ask what capabilities are reachable; they do not need to know which registry, provider, or transport supplied the answer.

A resolved record can contain:

```yaml
identifier: "+12025550147"
namespace: e164
capabilities:
  - voice
  - video
  - rtt
endpoints:
  - uri: "sip:example@video.invalid"
    media: video
authority:
  source: registry
provenance:
  observed_at: "..."
policy:
  emergency_capable: true
```

The same service contract can be backed by a local deterministic registry, TRS-numbering data, synthetic test data, or another authorized source. Access Identity is one optional provenance-bearing input; it cannot expand an otherwise unauthorized binding.

## ASL emergency-resource resolution

For emergency communications, Number can also resolve a service capability rather than only a subscriber destination.

Conceptually:

```text
resolve_accessibility(
    service  = "urn:service:sos",
    psap     = <authoritative PSAP>,
    modality = video,
    language = ASL
)
```

The result is an ordered set of **eligible resources**, not a replacement PSAP:

```text
1. local ASL telecommunicator
2. regional ASL emergency pool
3. broader ASL emergency resource
4. interpreter / VRS bridge
5. RTT or text fallback where policy permits
```

Selection remains subject to the PSAP/NG911 policy domain. Number supplies typed eligibility, capability, reachability, authority, and provenance.

## Live Celix services

Access state and ASL resources can both be modeled as small Celix services with different authority levels:

```text
itrs.access.identity
  access.transport = cellular
  profile.state    = active
  attachment       = roaming


tilden.asl.resource
  jurisdiction = MD
  media        = video
  language     = ASL
  role         = telecommunicator
  state        = available
```

A resolver can combine authorized static bindings with live service state and non-authoritative access context. That makes failover deterministic while still reacting to availability.

## Session model

The preferred emergency model is additive rather than substitutive:

```text
                         ┌── local dispatcher ── CAD / dispatch
                         │
Deaf caller ── video ────┼── ASL telecommunicator
                         │
                         └── optional interpreter
```

The local PSAP remains the incident owner. iTRS NG helps attach the communication resource.

## Distributed execution

The service boundary is intentionally compatible with Celix Remote Service Admin and the project's MessagePack DFI work. A caller should not care whether an Access Identity observer, Number resolver, or ASL resource is in-process, on another host, or supplied by a federated service.

```text
consumer
   │ typed Celix service
   ▼
Celix RSA / MessagePack
   │
   ▼
resolver or resource provider
```

## Determinism and provenance

Every resolution should be reproducible from explicit inputs and policy. A decision record should preserve at least:

- normalized identifier or requested service;
- authoritative PSAP context;
- requested modality and language;
- access-context observation and freshness when one was used;
- candidate resources;
- eligibility and policy results;
- selected resource;
- fallback path;
- source and authority for each binding;
- timestamps and version identifiers.

This lets the system answer not only **where did the call go?** but **why was this communication resource selected?**

The evidence chain must keep three claims separate:

```text
Access Identity: access observed
Tilden Number:   capability authorized / resolved
Baudot:          communication behavior observed / proven
```

## Non-goals

iTRS NG is not intended to become a replacement numbering administrator, eSIM provisioning system, carrier IMS, ECRF, ESRP, CAD system, PSAP, or carrier network. Its job is to make cross-system accessibility capabilities resolvable, interoperable, policy-qualified, and auditable.

## First prototype

Build a synthetic environment containing:

1. simulated emergency ingress;
2. deterministic location-to-PSAP result;
3. Celix Access Identity service with home, roaming, inactive, absent, and unknown fixtures;
4. Celix Tilden Number service;
5. live ASL-resource registry;
6. local, regional, and fallback resources;
7. three-party video/session joining;
8. evidence showing access observations, each candidate, the selection decision, and communications readiness separately.

The acceptance test is failover from **local ASL telecommunicator → regional ASL resource → interpreter/VRS fallback** without changing the authoritative local PSAP. The same PSAP result must survive every access fixture, including absent or unknown eSIM state.