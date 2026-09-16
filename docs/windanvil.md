# WindAnvil assurance profile

iTRS NG uses WindAnvil to distinguish ordinary test success from evidence bound to an immutable source object.

## Current candidate gate

Before assurance:

1. configure and compile the resolver with warnings as errors;
2. run CTest;
3. exercise all three failover demo paths;
4. optionally compile the Celix integration against a pinned Celix checkout;
5. commit the candidate and confirm the worktree is clean.

WindAnvil then evaluates repository-object integrity capabilities against the exact candidate SHA. The first profile uses `git.head-commit`, `git.object-integrity`, `git.repository-fsck`, and `git.symlink-boundary`.

## Next capability

M3 adds an `itrs-ng.reference-suite` WindAnvil capability. It will build and execute the deterministic resolver tests in a pinned, network-disabled execution cell and emit evidence for:

- source SHA match;
- compile success;
- core test execution;
- three-path failover execution;
- PSAP invariant;
- deterministic replay identity.

A skipped build or unavailable Celix prerequisite will be `BLOCKED`, never `PASS`.
