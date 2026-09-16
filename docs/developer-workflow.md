# Developer workflow

## Fast loop

```bash
cmake -S . -B build -DITRS_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Exercise the three baseline paths:

```bash
./build/itrs-number-demo
./build/itrs-number-demo --local-down
./build/itrs-number-demo --local-down --regional-down
```

Expected selections are `asl-local`, `asl-regional`, and `vrs-bridge`. The authoritative PSAP line must be identical in all three runs.

Exercise the historical iTRS ENUM seam:

```bash
./build/itrs-enum-demo
```

Expected query name: `2.1.2.1.5.5.5.1.0.8.1.itrs.us.`. The demo is deterministic and performs no network lookup; it evaluates a frozen synthetic NAPTR set.

## Celix loop

Set `ITRS_ENABLE_CELIX=ON` and point `ITRS_CELIX_SOURCE_DIR` at a pinned Apache Celix source checkout. The build produces three bundles: Number, synthetic ASL resources, and a smoke consumer, plus an `itrs-ng-celix-demo` container.

The Number bundle tracks `org.itrsng.asl.resource` services and copies their current properties into a frozen in-process snapshot before invoking the deterministic core. Resource service churn therefore affects subsequent resolutions without making one resolution internally time-dependent.

Run the generated container from its deployment directory so the relative `bundles/` path resolves:

```bash
cd build-celix/deploy/itrs-ng-celix-demo
./itrs-ng-celix-demo
```

## Before WindAnvil

The worktree must be clean. Commit the candidate, record its full SHA, run local tests again from that exact commit, and only then invoke WindAnvil. Do not label a dirty-tree test run as evidence for the committed object.
