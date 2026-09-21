# A11YV 1.0 — Accessibility Impact Vector

A11YV is an experimental, machine-readable accessibility **impact and remediation-priority** vector for iTRS Edge.

It deliberately does **not** turn WCAG into a numeric conformance grade. WCAG/ACT/EARL-style requirement and test outcomes remain separate evidence. A11YV answers a narrower operational question:

> How severely does this accessibility failure impair the interaction, and how urgently should it be remediated?

## Vector form

Canonical serialization order:

~~~text
A11YV:1.0/BL:<v>/TC:<v>/ALT:<v>/FQ:<v>/PS:<v>/AU:<v>
~~~

The parser accepts metrics in any order, rejects duplicates or missing metrics, and fails closed on unknown values.

| Metric | Meaning | Values from least to most severe |
| --- | --- | --- |
| BL | Blockage | N=None, F=Friction, P=Partial, T=Total |
| TC | Task criticality | A=Ancillary, S=Supporting, C=Core, E=Essential |
| ALT | Alternative path | E=Equivalent, D=Degraded, R=Requires assistance, N=None |
| FQ | Frequency | R=Rare, C=Conditional, F=Frequent, A=Always |
| PS | Persistence | M=Momentary, I=Intermittent, S=Session, P=Persistent |
| AU | Autonomy impact | N=None, R=Reduced, D=Dependent, I=Independent use impossible |

Disability/access modality is **not** a severity multiplier. Hearing, vision, motor, cognitive, speech, and other modalities belong in descriptive metadata. A11YV scores the interaction failure, not the person affected.

## Scoring

Each metric value maps to 0..3. The MVP weights are:

~~~text
BL  = 4
TC  = 2
ALT = 3
FQ  = 1
PS  = 1
AU  = 3
~~~

The weighted sum is normalized to 0.0..10.0 and rounded to one decimal place.

Severity bands:

~~~text
0.0       none
0.1–3.9   low
4.0–6.9   medium
7.0–8.9   high
9.0–10.0  critical
~~~

Example:

~~~text
A11YV:1.0/BL:T/TC:C/ALT:N/FQ:A/PS:P/AU:I
~~~

scores **9.5 / critical**.

## Machine-readable evaluation record

The repository schema is `schemas/accessibility-evaluation.schema.json`.

## Execution boundary

The scorer is C code compiled into the same freestanding iTRS Edge Wasm module. It adds no host imports.

MCP, Godot/Beckett, browser clients, CI, or other hosts can submit a vector; the Wasm kernel validates and scores it.

## Standards relationship

A11YV complements rather than replaces standards-oriented evidence:

- WCAG criteria describe conformance requirements.
- ACT-style rules can describe repeatable tests.
- EARL-style records can carry test assertions.
- A11YV adds a compact operational impact/remediation signal.

The intended composition is:

~~~text
requirements + test evidence + A11YV impact + immutable execution evidence
~~~

That record can be carried as JSON, encoded into canonical MessagePack for runtime use, and hashed into the WindAnvil/iTRS Edge evidence chain.
