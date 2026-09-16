# iTRS NG schemas

Machine-readable schemas for iTRS NG resolution objects and protocol structures live here.

- [`asl-resource.schema.json`](asl-resource.schema.json) — ASL-capable communication-resource advertisement used by the first emergency-resource resolver prototype.
- [`access-context.schema.json`](access-context.schema.json) — privacy-minimized device/network access observation consumed as non-authoritative context by Tilden Number.

Schemas should encode transport-neutral semantics where possible. Registry, federation, Celix service discovery, device/carrier adapters, and wire transport are separate implementation concerns.
