# iTRS Edge MVP

iTRS Edge is the agent-facing execution boundary for iTRS NG. MCP is the northbound semantic contract; this kernel is the deterministic layer beneath it.

The first MVP deliberately does not put network I/O, clocks, persistence, WebRTC, Cloudflare APIs, or Celix runtime state inside the kernel. External interactions are explicit effects executed by the host.

## Shape

~~~
MCP client
   |
Cloudflare Worker
   |
canonical MessagePack
   v
iTRS Edge kernel
   |
   | service.lookup effect
   v
Celix/resource host
   |
frozen resource snapshot
   v
iTRS Edge resume
   |
deterministic Number result + evidence
~~~

The kernel is stateless across step and resume. The canonical request is carried through the effect transcript, so another Worker can resume the operation without hidden continuation state.

## Wire profile

The MVP uses a deliberately tiny canonical MessagePack profile:

- arrays, strings, booleans, and signed 32-bit integers only;
- shortest valid MessagePack representation is emitted;
- fixed field ordering is defined by this document;
- duplicate-key ambiguity is impossible because maps are not used;
- embedded NUL bytes in strings are rejected;
- trailing bytes are rejected;
- resource snapshots are bounded by ITRS_NUMBER_MAX_CANDIDATES.

### Resolve

~~~text
[1, 0, request]
~~~

request is:

~~~text
[
  request_id,
  service,
  language,
  media,
  jurisdiction_path,
  authoritative_psap_id,
  authoritative_psap_endpoint,
  resource_snapshot,
  policy_version
]
~~~

### Effect

~~~text
[
  1,
  1,
  "service.lookup",
  "org.itrsng.asl.resource",
  request
]
~~~

The host satisfies this effect from the local or remote capability fabric. iTRS Edge does not know whether resources came from an in-process Celix registry, a remote node, a Cloudflare object, or a synthetic fixture.

### Resume

~~~text
[1, 2, request, resources]
~~~

Each resource is:

~~~text
[
  id,
  endpoint,
  language,
  media,
  role,
  scope,
  jurisdictions,
  services,
  state,
  authority_source,
  authority_version,
  priority,
  dispatch_authority
]
~~~

### Done

~~~text
[1, 3, result]
~~~

The result preserves the existing Number invariant: the authoritative PSAP is copied through unchanged while the selected communication resource may change.

### Fault

~~~text
[1, 255, errno, message]
~~~

Malformed envelopes, oversized snapshots, and resolver failures fail closed.

## Wasm boundary

The exported C API is intentionally host-buffer based:

~~~c
itrs_edge_step(input, input_len, output, output_capacity, &output_len);
itrs_edge_resume(input, input_len, output, output_capacity, &output_len);
~~~

A Worker-facing Wasm shim can be extremely small: copy canonical MessagePack into linear memory, invoke one of these functions, and copy the returned bytes out. No host API needs to be imported into the deterministic kernel.

That keeps Cloudflare-specific code outside the reference semantics and allows the exact same kernel to run natively in tests, in a Wasm Worker, or under independent WindAnvil replay.

The reference Wasm build is freestanding rather than WASI-hosted. Run:

~~~sh
./scripts/build-edge-wasm.sh
node wasm/test.mjs
~~~

The resulting module exports only linear memory and the iTRS Edge buffer/step/resume ABI. It has **zero imports**: no WASI, clocks, sockets, filesystem, entropy, or Cloudflare-specific host functions. The Wasm test exercises the same effect, deterministic ordering, PSAP invariant, and local-to-regional failover as the native test.

## Next effects

The MVP starts with one effect:

~~~text
service.lookup
~~~

Future effects should remain small and explicit:

~~~text
service.invoke
state.get
state.put
transport.send
clock.read
entropy.get
evidence.emit
~~~

Time and entropy are effects rather than hidden inputs. A complete transcript can therefore contain every nondeterministic observation needed to replay the decision.
