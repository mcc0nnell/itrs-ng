# Provider-edge capability resolution

Status: experimental contract.

## Purpose

This specification defines the iTRS-NG edge decision that occurs after an addressable identifier has been resolved to communication capabilities and reachable endpoints.

The edge resolves **what communication topology is required**. A VRS provider, interpreter pool, bridge, or ASL telecommunicator is a service resource behind that decision; it is not the authority that defines the number.

## Core invariant

```text
identifier -> capabilities -> topology -> service resources -> signalling
```

Provider selection MUST NOT occur before capability resolution.

If the destination itself satisfies the requested communication capability, the edge MUST prefer the direct path and MUST NOT insert an interpreter/VRS resource merely because the caller is an iTRS user.

## Resolution modes

### Direct

The destination exposes an eligible endpoint for the requested media/language capability.

```text
caller -> destination
```

No provider resource is part of the selected topology.

### Interpreted

The destination is reachable but cannot directly satisfy the caller's requested communication capability.

```text
caller -> interpreter resource -> destination
```

The selected interpreter/bridge is an attached resource. The destination identity remains the destination identity.

### Emergency accessibility

NG911/PSAP routing authority remains external to this resolver. The edge preserves the authoritative PSAP and attaches the communication resource needed by the session.

```text
caller -> authoritative PSAP
              +
        accessibility resource
```

Changing accessibility resources MUST NOT silently change the authoritative PSAP.

### Unresolved

If no eligible direct or service-resource topology exists, the resolver returns an explicit unresolved result with evaluated candidates. It MUST NOT invent a provider, endpoint, jurisdiction, or fallback.

## Deterministic selection

For a fixed request, number binding, resource-state snapshot, and policy document, the result MUST be deterministic.

The prototype ordering is:

- direct eligible endpoint before any provider resource;
- for ordinary interpreted sessions: interpreter before generic bridge;
- for emergency accessibility: eligible local telecommunicator before interpreter or bridge;
- matching jurisdiction before unscoped or other-scope resources where policy permits;
- stable resource identifier as the final tie-breaker.

Production policy can change this ordering, but the active policy version must be explicit evidence input.

## Evidence

Each decision record SHOULD preserve digests of:

- the request;
- the resolved number/capability binding;
- the complete resource-state snapshot used for selection;
- the active policy;
- the resulting topology.

The record MUST distinguish numbering/capability authority from selected service resources.

## Signalling boundary

This resolver does not own SIP transaction mechanics. Its output is a topology plan consisting of explicit session legs and selected resources.

The pure-WASM SIP kernel can execute those signalling reflexes independently:

```text
provider-edge resolver -> topology plan -> WASM SIP reflexes
```

This separation keeps provider policy, number semantics, SIP state, and host transport independently testable and replaceable.
