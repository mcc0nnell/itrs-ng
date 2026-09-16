# iTRS ENUM / NAPTR resolution

The historical iTRS query path is intentionally simple at its core: normalize an E.164 telephone number, reverse its digits beneath the iTRS ENUM apex, query NAPTR, and apply a terminal `E2U+sip` rewrite to obtain a SIP URI.

## Publicly documented shape

The FCC VRS Auto-Routing Proof-of-Concept cookbook identifies the Neustar query document as `iTRS_QI_Guide_5.0_v1.1` and configures the ENUM search domain as `itrs.us`. An IETF 105 RUM presentation later showed the historical iTRS ENUM wire shape directly, including a name beneath `itrs.us` and a terminal `E2U+sip` NAPTR rewrite.

Public references:

- FCC, *Auto-Routing Proof-of-Concept*, DA 15-1312A2: https://docs.fcc.gov/public/attachments/DA-15-1312A2.pdf
- IETF 105 RUM, *RUM – a bit of history and background*: https://datatracker.ietf.org/meeting/105/materials/slides-105-rum-rum-history-background-00
- RFC 6116, ENUM: https://www.rfc-editor.org/rfc/rfc6116.html
- RFC 3403, DNS NAPTR: https://www.rfc-editor.org/rfc/rfc3403.html

## Deterministic transform

For the example `+18015551212`:

```text
AUS:        +18015551212
Digits:      18015551212
Reversed:    2.1.2.1.5.5.5.1.0.8.1
Query name:  2.1.2.1.5.5.5.1.0.8.1.itrs.us.
```

A terminal record such as:

```text
IN NAPTR 10 11 "u" "E2U+sip" "!^(.*)$!sip:\1@providerGW.example.com!" .
```

produces:

```text
sip:+18015551212@providerGW.example.com
```

## Implementation boundary

`src/number/enum.c` implements the deterministic transformation and terminal NAPTR evaluation. It deliberately consumes an already-frozen NAPTR record set. DNS transport, caching, DNSSEC validation, resolver selection, and live `itrs.us` access are separate adapters.

This matters for assurance: WindAnvil can replay the same E.164 input and NAPTR snapshot without relying on mutable DNS state.

The implementation currently supports:

- E.164 normalization with the 15-digit maximum;
- configurable ENUM apex, with `itrs.us` as the iTRS default;
- reversed-digit query-name generation;
- terminal `U` rules;
- `E2U+sip` service matching;
- POSIX ERE matching and `\\0`–`\\9` replacement backreferences;
- NAPTR order/preference processing;
- deterministic tie-breaking;
- explicit failure for malformed, unsupported, over-capacity, or non-matching inputs.

## Relationship to Tilden Number and NG911

ENUM answers an address-resolution question: **what SIP URI is associated with this E.164 identifier under this authority?**

Tilden Number remains the broader authority/capability layer. An ENUM result can become one provenance-bearing endpoint observation consumed by Number; it does not by itself decide accessibility policy, emergency jurisdiction, or the authoritative PSAP.

```text
E.164 TN
   │
   ▼
ENUM key under itrs.us
   │
   ▼
NAPTR E2U+sip
   │
   ▼
SIP URI observation ─────► Tilden Number ─────► capability/policy resolution
                                                   │
NG911 ECRF/ESRP ───────── authoritative PSAP ──────┘
```
