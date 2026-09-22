# iTRS Edge Python MCP host

This is an alternate northbound host for iTRS Edge using Cloudflare Python Workers.

The important boundary stays unchanged: the accessibility/numbering decision still executes in the existing freestanding, zero-import Wasm kernel. Python does not reimplement that logic.

~~~text
MCP client
   |
Python Worker (native mcp package)
   |
Cloudflare cross-language RPC
   v
TypeScript Edge Worker
   |
freestanding iTRS Edge Wasm
   |
explicit host effects
~~~

The `EDGE` service binding points at the existing `itrs-edge-mcp` Worker. RPC stays inside Cloudflare's service-binding fabric; the Wasm service does not need another public API.

## Why this exists

Python Workers can now host the Python `mcp` package directly and can use the Workers networking/runtime surface without putting sockets or fetch inside the deterministic kernel. That makes Python a useful orchestration/effect host while keeping replayable telecom semantics in Wasm.

The host exposes the same two tools:

- `discover_communication_capabilities`
- `resolve_accessible_communication`

## Run

Install `uv`, then:

~~~sh
cd worker-python
uv run pywrangler dev
~~~

The Python Worker expects a service binding named `EDGE` targeting the deployed or locally configured `itrs-edge-mcp` Worker.

A health endpoint is available at `/health`; MCP is served at `/mcp`.
