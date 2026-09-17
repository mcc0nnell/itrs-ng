# AGENTS.md

## Project boundary

iTRS NG is research and reference software for accessible real-time communications, TRS, and NG911 interoperability. It is **not production emergency-call software**. Tests and demos use synthetic PSAPs, jurisdictions, identities, and endpoints only.

## Architectural invariants

1. NG911/ECRF/ESRP routing is authoritative for the PSAP. The Number resolver must never substitute a different PSAP as a side effect of accessibility-resource selection.
2. Accessibility resources are additive session participants or communication resources, not implicit incident owners.
3. Identical request + policy + resource snapshot must produce identical ordering and selection.
4. Missing, malformed, unavailable, or over-capacity inputs fail closed; they never become an implicit successful route.
5. Every candidate keeps enough reason/provenance data to explain why it was eligible or rejected.

## Local validation

Run before commit:

```bash
cmake -S . -B build -DITRS_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/itrs-number-demo
./build/itrs-number-demo --local-down
./build/itrs-number-demo --local-down --regional-down
```

For the Celix integration, use a pinned Apache Celix checkout:

```bash
cmake -S . -B build-celix -DITRS_ENABLE_CELIX=ON -DITRS_CELIX_SOURCE_DIR=/path/to/apache-celix
cmake --build build-celix --parallel
```

## WindAnvil

WindAnvil evaluates immutable clean git objects, not dirty worktrees. After local validation, commit first, then run the available WindAnvil git assurance capabilities against that exact SHA. Store evidence outside the source tree unless a committed reference specimen is intentionally being updated.

GitHub CI can be added later as convenience automation, but a green GitHub badge is not the assurance authority for this project.
