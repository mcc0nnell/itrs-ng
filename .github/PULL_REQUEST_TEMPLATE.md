## What changed

Describe the smallest architectural or interoperability change.

## Authority boundary

- [ ] This change preserves the NG911/authoritative-PSAP boundary.
- [ ] Accessibility resources do not implicitly acquire incident/dispatch authority.

## Determinism and evidence

- [ ] Identical request + policy + resource snapshot gives identical selection.
- [ ] Candidate rejection/selection remains explainable from retained reasons/provenance.
- [ ] New failure modes fail closed.

## Validation

Paste the local CTest result and, for candidate releases or architecture milestones, the WindAnvil evidence/receipt identifiers for the exact commit.
