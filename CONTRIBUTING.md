# Contributing to iTRS NG

iTRS NG is protocol- and proof-first. Contributions should preserve implementation neutrality, federation, portability, accessibility, and deterministic testability.

## Design principles

1. Do not require a single provider, registry operator, or runtime implementation.
2. Keep NG911/PSAP authority separate from communication-resource resolution.
3. Keep resolution semantics separate from call/session implementation.
4. Make trust, authority, expiry, fallback, and provenance explicit.
5. Prefer machine-readable schemas and executable conformance cases.
6. Preserve legacy telephone interoperability without making E.164 the only identity model.
7. Do not use live emergency traffic, credentials, or non-public PSAP data in tests.

## Before opening a PR

Run the local validation commands in `AGENTS.md`. Changes to resolver behavior should include a deterministic regression test and, where appropriate, a changelog fragment. Architectural boundary changes should include an ADR.

For milestone changes, attach WindAnvil evidence for the exact commit SHA after the worktree is clean.
