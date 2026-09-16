# Schemas

Machine-readable contracts for the first iTRS NG reference slice:

- `asl-resource.schema.json` — an ASL-capable resource advertisement;
- `resolution-request.schema.json` — immutable request context including the authoritative PSAP;
- `resolution-result.schema.json` — selected resource plus all evaluated candidates and reasons.

The C MVP uses fixed-size structures with equivalent semantics so a resolver call can be allocation-free across the Celix service boundary.
