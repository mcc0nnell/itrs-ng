# Baudot assurance integration

[iTRS NG](../README.md) and [Baudot](https://github.com/mcc0nnell/baudot) are linked projects with intentionally different authority boundaries.

## Rule

**iTRS NG decides what should happen. Baudot proves what actually happened at the communications boundary.**

iTRS NG remains authoritative for identifier resolution, resource eligibility, routing policy, provenance, failover order, and session intent. Baudot remains authoritative only for its own executable test vocabulary, observations, evidence preservation, and terminal reductions inside declared claim boundaries.

Neither project inherits authority merely because the other reports success.

## Integration shape

```text
NG911 / PSAP authority
        │
        ▼
      iTRS NG
        │
        ├── Tilden Number resolution
        ├── ASL resource selection
        ├── deterministic failover policy
        └── session-join intent
        │
        ▼
 scenario contract
        │
        ▼
       Baudot
        │
        ├── SIP / SDP observations
        ├── T.140 / RFC 4103 observations
        ├── WebRTC / media observations
        ├── transfer / replacement-leg readiness
        ├── controlled-network evidence
        └── independent reducers
        │
        ▼
 evidence facts
        │
        ▼
 WindAnvil campaign verdict / retained record
```

## What crosses the boundary

The preferred interface is data, not code linkage.

An iTRS NG assurance case should provide:

- scenario identifier and version;
- selected resource and the provenance-bearing reason it was eligible;
- intended session transition;
- media/accessibility capabilities required for success;
- observable readiness conditions;
- timeout and failover conditions; and
- the exact Baudot commit or release used by the campaign.

Baudot should return observations such as:

- signaling state reached;
- replacement or joined dialog established;
- negotiated media and payload facts;
- first independently observed usable T.140 / RTT event where relevant;
- video/WebRTC readiness facts where relevant;
- old-leg preservation or teardown state;
- packet/evidence artifact digests; and
- terminal reducer output with an explicit claim boundary.

## Important separation

A successful iTRS NG resolution is not evidence that a communications path is usable.

A successful Baudot communications scenario is not evidence that the selected resource was legally, operationally, or geographically authoritative.

For emergency use, NG911 remains authoritative for geographic routing and PSAP selection. iTRS NG adds accessibility-resource resolution and communication routing. Baudot tests the observable communications behavior that results.

## First WindAnvil campaign

The first campaign should exercise the three iTRS NG failover arms independently:

1. local ASL telecommunicator;
2. regional ASL emergency resource;
3. interpreter/VRS fallback.

For each arm, preserve two independent outputs:

1. **decision record** — why iTRS NG selected or rejected the resource; and
2. **communications evidence** — whether the resulting session became observably usable under the Baudot scenario.

The campaign should then inject controlled failures between the arms and verify that iTRS NG changes selection only according to declared policy while Baudot continues to report communications facts without becoming a routing authority.

## Repository relationship

Do not vendor Baudot into iTRS NG and do not make Baudot a runtime dependency of the production design.

For reproducible assurance runs, WindAnvil may materialize both repositories side by side at pinned commits, bind the iTRS NG scenario contract into Baudot inputs, execute the controlled specimen, and retain the combined evidence record.
