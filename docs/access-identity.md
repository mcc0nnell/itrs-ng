# Access identity and the eSIM seam

iTRS NG treats eSIM state as **access context**, not as communication identity and not as routing authority.

The boundary is intentionally narrow:

```text
DEVICE / ACCESS
      │
      │ carrier / eUICC / eSIM / IMS observations
      ▼
Access Identity
      │ privacy-minimized, typed assertions
      ▼
Tilden Number
      │ identity + authorized capability resolution
      ▼
iTRS NG orchestration
      │
      ├── NG911 / PSAP context
      └── SIP / video / RTT / voice resources
      │
      ▼
Baudot assurance
```

## Why this is a separate service

An eSIM profile answers questions about **network access**: whether a carrier profile exists, whether it is active, what access is presently observed, and what carrier-backed identity or capability assertions are available.

Tilden Number answers a different question: **what communication identity and capabilities are authorized and reachable?**

Combining those concerns would let transient carrier state become accidental numbering authority. Keeping them separate also allows iTRS NG to operate across Wi-Fi, carrier access, roaming, devices without an active profile, and synthetic lab environments without changing Number's identity semantics.

The earlier Tilden eSIM work established a useful invariant — `NUMBER != NETWORK`. The current iTRS NG architecture keeps that invariant but moves live eSIM/device state behind a dedicated access boundary.

## Celix shape

`access-identity` should be a small Celix application beside `tilden-number`, not a library inside it.

```text
access-identity
├── access-core
├── access-observer
├── access-policy
├── access-events
└── access-api

                    typed service
access-identity ─────────────────────► tilden-number
```

Possible observer implementations include a synthetic lab observer, a device/OS adapter, or an authorized carrier integration. The service contract is the stable part; observer mechanics remain replaceable.

## Assertion model

The output is an observation, not a grant of authority:

```yaml
version: 1
observation_id: "access-01"
observed_at: "2026-09-16T21:45:00Z"
source:
  kind: synthetic
  authority: "windanvil-lab"
access:
  transport: cellular
  profile_state: active
  attachment: roaming
  ims_state: registered
capabilities:
  - name: rtt
    state: available
    basis: advertised
  - name: video
    state: unknown
    basis: unknown
emergency:
  access_state: unknown
```

Important distinctions:

- `profile_state=active` does not prove an accessible session is usable.
- `ims_state=registered` does not prove RTT, video, or voice media readiness.
- `capabilities[].state=available` is an access observation, not Tilden authorization.
- `emergency.access_state=unknown` must remain a valid and ordinary input.

Baudot remains the place where actual communications behavior can be exercised and independently evidenced.

## Authority boundary

Access Identity may supply facts to Number, but it cannot:

- choose or replace an NG911 ECRF/ESRP routing result;
- choose the authoritative PSAP;
- create ownership of an E.164 identity merely because a profile contains or presents it;
- authorize a Tilden endpoint by itself;
- declare a signaling or media session usable;
- turn carrier authentication into Tilden control-plane authority.

Tilden can use an access assertion as one provenance-bearing input alongside registry, policy, service-discovery, and user/device context.

## Emergency invariant

The core emergency rule is:

> **Access context can improve an emergency session; missing access context must not become a gate on attempting emergency communication.**

An absent, inactive, unreadable, roaming, stale, or unknown eSIM observation must not cause iTRS NG to suppress the authoritative NG911 path. Likewise, failure of the Access Identity service must not erase already-known accessibility requirements.

This gives the architecture two independent flows:

```text
location / service context ─────────────► NG911 routing authority ─► PSAP

access observations ─► Access Identity ─► Tilden Number ──────────► accessibility resources
```

They can inform the same session without making either authority subordinate to the other.

## Privacy boundary

The normalized object must not expose raw subscription secrets or stable infrastructure identifiers merely because an observer can read them.

The public/inter-service access context should not contain:

- IMSI;
- ICCID;
- EID;
- eSIM activation codes;
- SM-DP+ transaction secrets;
- carrier credentials;
- private device identifiers.

When correlation to a Tilden identity is required, use an authorized, minimal binding appropriate to the deployment rather than forwarding the underlying carrier credential.

## WindAnvil / Baudot proof matrix

The first assurance campaign should replay the same Tilden/NG911 request against controlled access states:

| Arm | Access state | Required architectural result |
| --- | --- | --- |
| A | active / home / IMS registered | access assertion may inform Number |
| B | active / roaming | same identity semantics; provenance changes |
| C | inactive profile | emergency path remains attemptable |
| D | no profile | emergency path remains attemptable; no invented carrier identity |
| E | observer unavailable | deterministic `unknown` context; no emergency-routing suppression |
| F | advertised RTT but no live T.140 | Baudot must not promote RTT readiness |
| G | access says video available but Tilden does not authorize video | capability must not be expanded |

WindAnvil can pin the iTRS NG and Baudot revisions, materialize each arm, preserve the access assertion and Tilden decision record, and then ask Baudot for communications evidence. The terminal verdict should distinguish **access observed**, **capability authorized**, and **communication proven**.

## First implementation slice

The first implementation does not need carrier APIs. A deterministic synthetic observer is enough to prove the boundary:

1. publish `itrs.access.identity` as a Celix service;
2. emit objects conforming to `schemas/access-context.schema.json`;
3. let Tilden Number consume the object as non-authoritative context;
4. run home, roaming, inactive, absent, and unknown fixtures;
5. verify that NG911/PSAP authority is unchanged across all five;
6. hand resulting session behavior to Baudot for independent readiness evidence.

That gives iTRS NG a real eSIM seam without turning it into an eSIM provisioning stack.