# Threat model

The first iTRS NG slice is a synthetic interoperability laboratory, not a live emergency network component.

## Protected invariants

- Accessibility routing cannot replace or rewrite the authoritative PSAP supplied by the NG911 side of the boundary.
- Unavailable or mismatched resources cannot rank as eligible.
- Candidate ordering cannot depend on Celix discovery order or input array order.
- Too many candidate resources fail closed instead of truncating the snapshot silently.
- Resource authority source/version survive into candidate evidence.

## Untrusted or failure-prone inputs

- live resource availability;
- resource metadata and service properties;
- jurisdiction declarations;
- endpoint URIs;
- future federation data;
- transport and remote-service availability.

These inputs may affect eligibility but must not alter the PSAP authority fields in a resolution request.

## Out of scope for M0/M1

- caller authentication and identity proofing;
- production i3 security profiles;
- real PSAP, CAD, selective-router, or VRS credentials;
- live 911 traffic;
- operational staffing/load-balancing policy.
