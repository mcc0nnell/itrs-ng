# Current capabilities

The repository now contains an executable M0/M1 reference slice.

## Resolver

- deterministic local → regional → bridge ordering;
- ASL, media, service, availability, and jurisdiction eligibility checks;
- explicit candidate rejection reasons;
- source/version provenance carried per resource;
- bounded candidate set that fails closed instead of truncating;
- authoritative PSAP copied unchanged into every result.

## Celix

- `org.itrsng.number` resolver service;
- `org.itrsng.asl.resource` dynamic resource-advertisement service;
- live service tracker that freezes a snapshot before each resolve call;
- synthetic local/regional/VRS resource bundle;
- smoke consumer proving discovery and resolution inside an Apache Celix container.

## Assurance

- warnings-as-errors native build;
- CTest regression suite;
- three explicit failover demos;
- documented WindAnvil immutable-object gate.

## ENUM / NAPTR

- E.164 normalization and 15-digit limit enforcement.
- Reversed-digit ENUM keys beneath a configurable apex (`itrs.us` by default).
- Deterministic terminal `E2U+sip` NAPTR processing with order/preference semantics.
- POSIX ERE rewrite support with numeric backreferences.
- Synthetic public-shape fixtures and executable tests.
- DNS transport intentionally remains outside the deterministic core.
