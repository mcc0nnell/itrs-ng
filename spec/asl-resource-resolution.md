# ASL emergency resource resolution

Status: initial design sketch.

## Purpose

This specification defines the minimum semantics for resolving an ASL-capable communications resource for an emergency session while preserving the authoritative NG911/PSAP routing result.

The resolver does **not** choose the geographic PSAP. It operates after, alongside, or under policy derived from that routing decision.

## Request

A request should be explicit and typed:

```yaml
service: "urn:service:sos"
psap: "sip:psap@example.invalid"
requested:
  language: ASL
  media: video
  role: telecommunicator
caller:
  capabilities:
    - video
    - rtt
policy_context:
  jurisdiction: "MD"
```

Required semantics:

- `service` identifies the requested emergency service context.
- `psap` identifies the already-selected or authoritative public-safety endpoint/domain.
- `language` and `media` describe the communication capability needed.
- `role` distinguishes an ASL-fluent telecommunicator from an interpreter or other assistance role.
- `policy_context` provides information used to determine eligibility; it is not an invitation for the resolver to invent jurisdiction.

## Resource advertisement

An ASL-capable resource advertises at least:

```yaml
id: "asl-resource-001"
endpoint: "sip:asl-resource-001@example.invalid"
language: ASL
media:
  - video
role: telecommunicator
scope:
  jurisdiction:
    - MD
state: available
authority:
  source: "synthetic-test-registry"
```

Optional fields can describe dispatch authority, queue membership, operating organization, load, supported codecs, RTT capability, and policy metadata.

## Role semantics

`telecommunicator` means the resource can directly perform the communication role represented by the resource record.

`interpreter` means the resource provides communication assistance and does not silently inherit dispatch or PSAP authority.

`bridge` means the resource can join or mediate a multi-party session but should not be treated as the incident owner.

## Resolution result

The resolver returns candidates and an explanation, not merely a URI:

```yaml
request_id: "..."
selected: "asl-resource-001"
candidates:
  - id: "asl-resource-001"
    eligible: true
    rank: 1
    reasons:
      - "language-match"
      - "media-match"
      - "jurisdiction-match"
      - "available"
  - id: "regional-asl-002"
    eligible: true
    rank: 2
    reasons:
      - "regional-fallback"
provenance:
  resolver_version: "..."
  policy_version: "..."
```

The ranking must be deterministic for the same resource state, policy, and request inputs.

## Baseline ordering

The initial prototype should support this policy-shaped ordering:

1. local ASL telecommunicator;
2. regional ASL emergency resource;
3. broader qualified ASL resource;
4. interpreter or VRS bridge;
5. alternate accessible modality where permitted.

The ordering is a prototype policy, not a universal rule. Production deployments must bind ordering and eligibility to applicable operational and public-safety policy.

## Failover

A resource becoming unavailable must not implicitly alter the authoritative PSAP. The resolver reevaluates communication-resource candidates while preserving the emergency-routing context.

```text
local PSAP
   │
   ├── local ASL resource       unavailable
   ├── regional ASL resource    selected
   └── interpreter/VRS bridge   standby
```

## Evidence

Every resolution event should be capable of producing an evidence record containing:

- request inputs;
- authoritative PSAP context;
- resource-state snapshot or version;
- all evaluated candidates;
- policy result per candidate;
- selected resource;
- failover reason when applicable;
- resolver and policy versions;
- timestamp.

## Celix service shape

A first C service contract can remain intentionally small:

```c
typedef struct {
    celix_status_t (*resolve)(
        void *handle,
        const itrs_asl_resolution_request_t *request,
        itrs_asl_resolution_result_t *result);

    celix_status_t (*watch)(
        void *handle,
        const itrs_asl_resource_filter_t *filter,
        itrs_asl_resource_listener_t *listener);
} itrs_asl_resolution_service_t;
```

The contract should not expose registry-specific mechanics. Registry, federation, policy, and transport implementations remain replaceable Celix services.