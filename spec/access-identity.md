# Access Identity service

Status: initial design sketch.

## Purpose

This specification defines the minimum transport-neutral contract by which iTRS NG can consume observations about the device/network access edge, including eSIM-backed cellular access, without making carrier state authoritative for numbering, accessibility routing, or NG911 routing.

The service is intentionally observation-oriented:

```text
carrier / device / synthetic observer
              │
              ▼
       Access Identity
              │
       access-context@1
              ▼
        Tilden Number
```

The access context can inform resolution. It cannot mint identity authority or prove session usability.

## Service name

The initial Celix service name is:

```text
itrs.access.identity
```

The first context object version is `1` and is described by [`../schemas/access-context.schema.json`](../schemas/access-context.schema.json).

## Observation

A context object represents what one observer knew at one point in time.

Example:

```yaml
version: 1
observation_id: "access-roaming-001"
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
  - name: voice
    state: available
    basis: advertised
  - name: rtt
    state: available
    basis: advertised
  - name: video
    state: unknown
    basis: unknown
emergency:
  access_state: unknown
```

`unknown` is a first-class value. Implementations must prefer an explicit unknown over fabricating a positive or negative capability claim.

## Source semantics

`source.kind` identifies the class of observer, not an implicit trust level.

Initial values are:

- `device-os` — observation supplied by a local device/platform adapter;
- `carrier-adapter` — observation supplied through an authorized carrier-facing integration;
- `synthetic` — deterministic laboratory fixture;
- `other` — explicitly named source outside the initial set.

`source.authority` names the producer or evidence domain. Consumers decide what weight that source is allowed to carry under local policy.

## Access state

`access.transport` describes the observed access family. Initial values are:

```text
cellular
wifi
ethernet
other
unknown
```

For cellular access, these additional observations may be supplied:

```text
profile_state = active | inactive | absent | unknown
attachment    = home | roaming | detached | unknown
ims_state     = registered | unregistered | unknown
```

These are observations about access. They do not redefine the Tilden subject.

## Capability observations

A capability observation consists of:

```yaml
name: rtt
state: available
basis: advertised
```

Initial capability names are `voice`, `rtt`, `video`, `text`, and `data`.

Initial state values are `available`, `unavailable`, and `unknown`.

Initial basis values are:

- `advertised` — the access edge reports support;
- `observed` — the observer saw behavior sufficient for its own bounded claim;
- `configured` — local configuration declares the capability;
- `synthetic` — the value comes from a deterministic test fixture;
- `unknown` — no stronger basis is available.

A capability observation is not permission to add that capability to a Tilden resolution. Tilden may only expose endpoints and capabilities that are independently authorized by its own authority/policy model.

## Emergency semantics

The service may report:

```text
emergency.access_state = available | unavailable | unknown
```

This field is contextual only. iTRS NG must not treat `unknown` or `unavailable` as authority to suppress an emergency attempt that would otherwise be made through the applicable emergency communications path.

The following invariants apply:

1. Access Identity does not select the PSAP.
2. Access Identity does not replace ECRF/ESRP or other NG911 authority.
3. Failure to obtain an access context does not erase known accessibility requirements.
4. Absence or inactivity of an eSIM profile does not create a Tilden identity failure.
5. A carrier-backed assertion does not, by itself, prove Tilden ownership or authorization.
6. An access capability observation does not prove live media readiness.

## Tilden consumption

Tilden Number may consume the object as provenance-bearing context.

Conceptually:

```c
typedef struct {
    celix_status_t (*snapshot)(
        void *handle,
        itrs_access_context_t *context);

    celix_status_t (*watch)(
        void *handle,
        itrs_access_identity_listener_t *listener);
} itrs_access_identity_service_t;
```

The Number resolver can then evaluate:

```text
authorized bindings
    + policy
    + requested modality
    + authoritative emergency context
    + access observation
    + live resource state
    -> resolution decision
```

The access observation may narrow operational choices or contribute provenance. It must not expand an otherwise unauthorized capability.

## Freshness

Every context requires `observed_at`. Deployments should define a maximum age appropriate to the observer and decision being made.

A stale context must become `unknown` for decisions that require current access state rather than being silently reused as fresh truth.

## Privacy

A conforming `access-context@1` object must not contain raw subscription or provisioning secrets. In particular, implementations must not place IMSI, ICCID, EID, activation codes, SM-DP+ transaction secrets, carrier credentials, or private device identifiers in the normalized object.

Observer-specific implementations may need such data internally. That does not make those identifiers part of the iTRS NG service contract.

## Baudot boundary

Baudot consumes resulting communications behavior as evidence, not raw carrier credentials.

The important distinction is:

```text
Access Identity: access observed
Tilden Number:   capability authorized/resolved
Baudot:          communication behavior observed/proven
```

For example, `rtt=available` in an access context cannot satisfy a Baudot readiness condition that requires live T.140 observation.

## Deterministic test vectors

The first conformance fixtures should cover:

- active profile, home attachment, IMS registered;
- active profile, roaming attachment;
- inactive profile;
- absent profile;
- observer failure represented as unknown;
- access advertises RTT while no live T.140 appears;
- access advertises video while Number policy does not authorize video.

For the first five vectors, the authoritative NG911/PSAP input must remain unchanged. The test is specifically intended to prove that access state is not emergency-routing authority.