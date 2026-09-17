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

The same service contract can be backed by a local deterministic registry, TRS-numbering data, synthetic test data, or another authorized source.

### Historical iTRS ENUM seam

Public FCC and IETF material shows the historical query seam as E.164/ENUM/NAPTR: normalize the telephone number, reverse its digits beneath `itrs.us`, query NAPTR, then apply a terminal `E2U+sip` rule to obtain a SIP URI. iTRS NG implements that as a deterministic input adapter to Number rather than hiding it inside signaling.

```text
+18015551212
      │
      ▼
2.1.2.1.5.5.5.1.0.8.1.itrs.us.
      │ NAPTR
      ▼
   E2U+sip
      │
      ▼
sip:+18015551212@providerGW.example.com
      │
      ▼
Tilden Number endpoint observation
```

DNS transport and mutable live-zone state stay outside the deterministic core. The core accepts a frozen NAPTR snapshot so resolution can be replayed and assured independently. See [`enum-naptr.md`](enum-naptr.md).

## Celix ENUM provider plane

The historical iTRS ENUM seam is exposed as a dynamic Celix dependency. `org.itrsng.number` tracks the highest-ranked `org.itrsng.enum.provider`, freezes the returned NAPTR observation, and only then invokes the deterministic RFC 6116 rewrite core. The observation carries provider source, query name, timestamp, minimum TTL, and DNS AD-bit evidence. DNS and ASL-resource state use separate locks so DNS latency cannot delay accessibility-resource updates. See `docs/celix-enum-provider.md`.

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

ASL resources can advertise themselves as dynamic services:

```text
tilden.asl.resource
  jurisdiction = MD
  media        = video
  language     = ASL
  role         = telecommunicator
  state        = available
```

A resolver can combine static authority data with live service state. That makes failover deterministic while still reacting to availability.

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

The service boundary is intentionally compatible with Celix Remote Service Admin and the project's MessagePack DFI work. A caller should not care whether a Number resolver or ASL resource is in-process, on another host, or supplied by a federated service.

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
- candidate resources;
- eligibility and policy results;
- selected resource;
- fallback path;
- source and authority for each binding;
- timestamps and version identifiers.

This lets the system answer not only **where did the call go?** but **why was this communication resource selected?**

## Non-goals

iTRS NG is not intended to become a replacement numbering administrator, ECRF, ESRP, CAD system, PSAP, or carrier network. Its job is to make cross-system accessibility capabilities resolvable, interoperable, policy-qualified, and auditable.

## First prototype

Build a synthetic environment containing:

1. simulated emergency ingress;
2. deterministic location-to-PSAP result;
3. Celix Tilden Number service;
4. live ASL-resource registry;
5. local, regional, and fallback resources;
6. three-party video/session joining;
7. evidence showing each candidate and selection decision.

The acceptance test is failover from **local ASL telecommunicator → regional ASL resource → interpreter/VRS fallback** without changing the authoritative local PSAP.