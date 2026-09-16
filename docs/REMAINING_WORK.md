# Remaining work

The current code proves the resolution seam. It does not yet establish a full NG911/TRS session.

Highest-leverage next work:

1. Move baseline ordering into a versioned policy document loaded by Number.
2. Add a canonical resolution-record serialization and digest for replay identity.
3. Add an `itrs-ng.reference-suite` WindAnvil capability that compiles and runs the resolver in a pinned network-disabled cell.
4. Define MessagePack DFI for the Number request/result and resource advertisement contracts.
5. Add a synthetic i3-shaped ingress adapter that supplies, but cannot mutate, the authoritative PSAP context.
6. Add a session graph proving caller + local PSAP + remote ASL resource + optional interpreter.
7. Add controlled resource churn tests to prove one resolution observes one stable snapshot.

Real PSAP endpoints, credentials, and live 911 traffic remain intentionally out of scope.
