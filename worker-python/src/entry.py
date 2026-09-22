from urllib.parse import urlparse

from workers import Response, WorkerEntrypoint, asgi


def _plain(value):
    """Convert a Workers FFI proxy to a Python value when needed."""
    to_py = getattr(value, "to_py", None)
    return to_py() if callable(to_py) else value


def build_app(env):
    from mcp.server import MCPServer

    server = MCPServer("iTRS Edge Python host")

    @server.tool()
    async def discover_communication_capabilities() -> dict:
        """Return capabilities from the pure-Wasm iTRS Edge service."""
        result = await env.EDGE.discover_communication_capabilities()
        return _plain(result)

    @server.tool()
    async def resolve_accessible_communication(
        service: str = "urn:service:sos",
        language: str = "ASL",
        media: str = "video",
        jurisdiction_path: str = "MD,Frederick",
        authoritative_psap_id: str = "psap-frederick",
        authoritative_psap_endpoint: str = "sip:psap-frederick@example.invalid",
        request_id: str | None = None,
        unavailable_resources: list[str] | None = None,
    ) -> dict:
        """Resolve communication while preserving authoritative PSAP state."""
        request = {
            "service": service,
            "language": language,
            "media": media,
            "jurisdictionPath": jurisdiction_path,
            "authoritativePsapId": authoritative_psap_id,
            "authoritativePsapEndpoint": authoritative_psap_endpoint,
        }
        if request_id is not None:
            request["requestId"] = request_id
        if unavailable_resources is not None:
            request["unavailableResources"] = unavailable_resources

        result = await env.EDGE.resolve_accessible_communication(request)
        return _plain(result)

    return server.streamable_http_app(
        streamable_http_path="/mcp",
        stateless_http=True,
    )


class Default(WorkerEntrypoint):
    def __init__(self, ctx, env):
        super().__init__(ctx, env)
        self.app = build_app(env)

    async def fetch(self, request):
        path = urlparse(request.url).path
        if path == "/health":
            return Response.json(
                {
                    "service": "iTRS Edge Python MCP host",
                    "mcp": "/mcp",
                    "edge_rpc": "EDGE",
                    "kernel": "remote-freestanding-wasm",
                }
            )

        if path == "/mcp" and request.method == "POST":
            return await asgi.fetch(self.app, request, self.env)

        return Response("Not found", status=404)
