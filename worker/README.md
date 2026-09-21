# iTRS Edge MCP Worker

This directory is the thin northbound MCP surface for iTRS Edge.

It intentionally contains almost no telecommunications policy. The Worker accepts agent-facing MCP tool calls, translates the request into the canonical iTRS Edge MessagePack envelope, executes the freestanding Wasm kernel, satisfies explicit host effects, and returns the deterministic result with transcript digests.

## Boundary

~~~text
MCP client
   |
Streamable HTTP
   v
Cloudflare Worker
   |
canonical MessagePack
   v
iTRS Edge Wasm
   |
service.lookup effect
   v
synthetic capability registry (MVP)
   |
frozen snapshot
   v
iTRS Edge Wasm resume
   |
result + SHA-256 evidence
~~~

The Wasm module has zero imports. It cannot fetch, read a clock, obtain entropy, use a filesystem, open a socket, or call a Cloudflare API. The Worker owns those effects.

## Current tools

### discover_communication_capabilities

Returns the synthetic communication resources visible to the MVP capability plane.

### resolve_accessible_communication

Resolves an accessibility resource from a frozen snapshot while preserving the authoritative PSAP supplied to the kernel.

For failover demonstrations, mark any of these synthetic resources unavailable:

- `asl-local`
- `asl-regional`
- `vrs-bridge`

The normal path selects `asl-local`. With `asl-local` unavailable, the same policy selects `asl-regional`.

## Local validation

~~~sh
cd worker
npm install
npm run check
npm run build
~~~

`npm run build`:

1. rebuilds the freestanding Wasm kernel;
2. type-checks the Worker against the pinned MCP/Cloudflare dependencies; and
3. runs a Wrangler dry-run bundle.

To exercise the MCP endpoint:

~~~sh
npm run dev
~~~

Then connect an MCP client to:

~~~text
http://localhost:8787/mcp
~~~

A simple health endpoint is available at `/health`.

## Not yet in the MVP

The synthetic registry is deliberately temporary. The next host effects are expected to connect the same kernel contract to:

- Celix-backed local or remote service discovery;
- Cloudflare Realtime rooms / WebRTC media;
- Beckett filthy-edge adapters;
- durable room state;
- independent WindAnvil transcript replay.

Those integrations must remain outside the deterministic kernel unless their observations are explicitly represented in the effect transcript.
