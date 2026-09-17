# Celix ENUM provider plane

The mutable DNS edge is a Celix service. The ENUM/NAPTR resolver core remains a pure function over a frozen observation.

```text
org.itrsng.number
      |
      | tracks highest-ranked provider
      v
org.itrsng.enum.provider
      |
      +-- synthetic fixture (ranking 1000)
      +-- system DNS       (ranking 100)
      |
      v
NAPTR observation
      |
      v
itrs_enum_resolve_sip()
      |
      v
SIP URI + provenance
```

`org.itrsng.enum.provider` returns the query domain, observation time, minimum TTL, DNS authenticated-data bit, and a bounded copy of the NAPTR answer set. Number then resolves that frozen set deterministically.
## Threading boundary

`resolveE164()` is deliberately rejected with `EWOULDBLOCK` when called on the Celix framework event-loop thread. A live DNS lookup may block; it must not stall framework lifecycle or service-registration events.

The Number bundle tracks the current highest-ranked provider using Celix's `set` service-tracker callback. It holds a provider-specific mutex while a lookup is in flight so an unregister/ranking transition cannot invalidate the service pointer. ASL-resource tracking uses a separate mutex, so DNS latency cannot stall emergency-resource churn.

The smoke consumer invokes Number from a worker thread, not from a service-registration callback.

## DNS evidence boundary

The system-DNS provider uses the host resolver to issue an IN/NAPTR query for the fully qualified ENUM name. It parses the returned order, preference, flags, service, regexp, replacement, TTL, and DNS header AD bit.

`authenticated_data=true` means the resolver response carried the DNS AD bit. It is evidence from the configured resolver, **not** an independent assertion by iTRS NG that DNSSEC validation was performed correctly. DNSSEC trust policy belongs outside the deterministic NAPTR rewrite core.

## Test and production composition

The deterministic demo container includes both providers. The fixture has ranking 1000 and therefore wins over system DNS at ranking 100. This makes CI and WindAnvil replay independent of network state while compiling and packaging the real DNS bundle in the same build.

A production composition should omit the fixture provider. TTL-aware caching, negative caching, timeout policy, explicit DNSSEC validation policy, and asynchronous refresh are future provider-plane work; none of those concerns belong in `itrs_enum_resolve_sip()`.