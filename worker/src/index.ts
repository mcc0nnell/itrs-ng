import {McpServer} from "@modelcontextprotocol/server";
import {createMcpHandler} from "agents/mcp/server";
import {z} from "zod";

import {evaluateAccessibility, listSyntheticCapabilities, resolveAccessibility} from "./edge";

function createServer(): McpServer {
  const server = new McpServer({
    name: "iTRS Edge",
    version: "0.1.0",
  });

  server.registerTool(
    "discover_communication_capabilities",
    {
      description:
        "Discover the communication capabilities currently visible to the iTRS Edge resource plane. " +
        "The MVP uses synthetic ASL/video resources and preserves their authority/provenance metadata.",
      inputSchema: {},
    },
    async () => {
      const capabilities = listSyntheticCapabilities();
      return {
        content: [
          {
            type: "text" as const,
            text: `iTRS Edge sees ${capabilities.length} synthetic communication capabilities.`,
          },
        ],
        structuredContent: {capabilities},
      };
    },
  );

  server.registerTool(
    "evaluate_accessibility",
    {
      description:
        "Evaluate an A11YV 1.0 machine-readable accessibility impact vector in the freestanding Wasm kernel. " +
        "The 0.0-10.0 result is remediation priority, not a WCAG conformance score.",
      inputSchema: {
        vector: z.string().min(1).max(255),
        modalities: z.array(z.string().min(1).max(63)).max(16).optional(),
        wcagCriteria: z.array(z.string().min(1).max(31)).max(32).optional(),
      },
    },
    async (input) => {
      const evaluation = await evaluateAccessibility(
        input.vector,
        input.modalities,
        input.wcagCriteria,
      );
      return {
        content: [
          {
            type: "text" as const,
            text: `A11YV ${evaluation.score.toFixed(1)} ${evaluation.severity.toUpperCase()}; this is remediation priority, not WCAG conformance.`,
          },
        ],
        structuredContent: evaluation,
      };
    },
  );

  server.registerTool(
    "resolve_accessible_communication",
    {
      description:
        "Resolve an accessible communication resource from a frozen capability snapshot without changing " +
        "the authoritative PSAP. The result includes ordered candidates and replay-oriented evidence digests.",
      inputSchema: {
        requestId: z.string().min(1).max(63).optional(),
        service: z.string().min(1).max(63).default("urn:service:sos"),
        language: z.string().min(1).max(63).default("ASL"),
        media: z.string().min(1).max(63).default("video"),
        jurisdictionPath: z.string().min(1).max(255).default("MD,Frederick"),
        authoritativePsapId: z.string().min(1).max(63).default("psap-frederick"),
        authoritativePsapEndpoint: z
          .string()
          .min(1)
          .max(191)
          .default("sip:psap-frederick@example.invalid"),
        unavailableResources: z
          .array(z.enum(["asl-local", "asl-regional", "vrs-bridge"]))
          .max(3)
          .optional()
          .describe("Synthetic resources to mark unavailable for deterministic failover testing."),
      },
    },
    async (input) => {
      const resolution = await resolveAccessibility(input);
      const summary = resolution.selected
        ? `Selected ${resolution.selectedId} (${resolution.selectedRole}/${resolution.selectedScope}); authoritative PSAP preserved=${resolution.invariant.authoritativePsapPreserved}.`
        : `No eligible communication resource; authoritative PSAP preserved=${resolution.invariant.authoritativePsapPreserved}.`;

      return {
        content: [{type: "text" as const, text: summary}],
        structuredContent: resolution,
      };
    },
  );

  return server;
}

const mcp = createMcpHandler(createServer);

export default {
  async fetch(request: Request, env: unknown, ctx: ExecutionContext): Promise<Response> {
    const url = new URL(request.url);
    if (url.pathname === "/health") {
      return Response.json({
        service: "iTRS Edge",
        mcp: "/mcp",
        kernel: "freestanding-wasm",
        wire: "canonical-messagepack-v1",
      });
    }
    return mcp(request, env, ctx);
  },
};
