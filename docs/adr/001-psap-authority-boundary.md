# ADR 001 — Preserve NG911 PSAP authority across accessibility-resource resolution

Status: accepted

## Context

An accessible emergency session may require an ASL-fluent telecommunicator, interpreter, video bridge, RTT path, or another communication resource that is not physically located at the PSAP selected by NG911 routing.

If accessibility resolution silently becomes a second geographic call-routing system, the architecture loses a clear authority boundary and creates failure modes that are difficult to explain or audit.

## Decision

iTRS NG treats the NG911/PSAP result as immutable input to Number resolution. The resolver may select, fail over, or decline accessibility resources, but it must copy the authoritative PSAP identity and endpoint into its result unchanged.

The first executable invariant is:

```text
authoritative_psap_before == authoritative_psap_after
```

Resource selection is a separate decision over a frozen resource snapshot.

## Consequences

- Local/regional ASL-resource failover can occur without transferring incident ownership.
- Interpreter and bridge roles do not inherit dispatch authority by being selected.
- Tests can independently prove geographic-routing stability and communication-resource failover.
- Future i3/ESRP integrations must make the boundary explicit rather than hiding it in adapter behavior.
