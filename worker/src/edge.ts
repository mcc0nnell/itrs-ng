import edgeModule from "./generated/itrs_edge.wasm";
import {decode, encode, type WireValue} from "./msgpack";

interface EdgeWasmExports {
  memory: WebAssembly.Memory;
  itrs_edge_wasm_input_ptr(): number;
  itrs_edge_wasm_input_capacity(): number;
  itrs_edge_wasm_output_ptr(): number;
  itrs_edge_wasm_output_capacity(): number;
  itrs_edge_wasm_output_len(): number;
  itrs_edge_wasm_step(inputLength: number): number;
  itrs_edge_wasm_resume(inputLength: number): number;
  itrs_accessibility_wasm_score(inputLength: number): number;
  itrs_accessibility_wasm_severity(): number;
}

export interface ResolveAccessibilityInput {
  requestId?: string;
  service: string;
  language: string;
  media: string;
  jurisdictionPath: string;
  authoritativePsapId: string;
  authoritativePsapEndpoint: string;
  unavailableResources?: string[];
}

export interface AccessibilityEvaluation {
  scheme: "A11YV";
  version: "1.0";
  vector: string;
  metrics: Record<string, string>;
  score: number;
  severity: "none" | "low" | "medium" | "high" | "critical";
  modalities: string[];
  requirements: {
    wcagCriteria: string[];
  };
  evidence: {
    kernel: "itrs-edge-wasm-v1";
    vectorSha256: string;
  };
  note: "remediation-priority-not-wcag-conformance";
}

export interface CapabilityCandidate {
  id: string;
  endpoint: string;
  role: string;
  scope: string;
  authoritySource: string;
  authorityVersion: string;
  eligible: boolean;
  classRank: number;
  priority: number;
  reasonMask: number;
}

export interface EdgeResolution {
  requestId: string;
  authoritativePsapId: string;
  authoritativePsapEndpoint: string;
  resourceSnapshot: string;
  policyVersion: string;
  selected: boolean;
  selectedId: string;
  selectedEndpoint: string;
  selectedRole: string;
  selectedScope: string;
  candidates: CapabilityCandidate[];
  invariant: {
    authoritativePsapPreserved: boolean;
  };
  evidence: {
    kernel: "itrs-edge-wasm-v1";
    requestSha256: string;
    effectSha256: string;
    resumeSha256: string;
    resultSha256: string;
  };
}

type EdgeCall = "itrs_edge_wasm_step" | "itrs_edge_wasm_resume";

function asArray(value: WireValue, label: string): WireValue[] {
  if (!Array.isArray(value)) throw new Error(`${label} is not an array`);
  return value;
}

function asString(value: WireValue, label: string): string {
  if (typeof value !== "string") throw new Error(`${label} is not a string`);
  return value;
}

function asNumber(value: WireValue, label: string): number {
  if (typeof value !== "number") throw new Error(`${label} is not a number`);
  return value;
}

function asBoolean(value: WireValue, label: string): boolean {
  if (typeof value !== "boolean") throw new Error(`${label} is not a boolean`);
  return value;
}

function parseA11yvMetrics(vector: string): Record<string, string> {
  const prefix = "A11YV:1.0/";
  if (!vector.startsWith(prefix)) return {};
  return Object.fromEntries(
    vector.slice(prefix.length).split("/").map((metric) => {
      const [name, value] = metric.split(":");
      return [name, value];
    }),
  );
}

export async function evaluateAccessibility(
  vector: string,
  modalities: string[] = [],
  wcagCriteria: string[] = [],
): Promise<AccessibilityEvaluation> {
  const kernel = await instantiateKernel();
  const bytes = new TextEncoder().encode(vector);
  if (bytes.length === 0 || bytes.length >= kernel.itrs_edge_wasm_input_capacity()) {
    throw new RangeError("A11YV vector exceeds Wasm input capacity");
  }

  const inputPtr = kernel.itrs_edge_wasm_input_ptr();
  new Uint8Array(kernel.memory.buffer, inputPtr, bytes.length).set(bytes);
  const scoreTenths = kernel.itrs_accessibility_wasm_score(bytes.length);
  if (scoreTenths < 0) throw new Error(`invalid A11YV vector (rc=${scoreTenths})`);

  const severityCode = kernel.itrs_accessibility_wasm_severity();
  const severities = ["none", "low", "medium", "high", "critical"] as const;
  const severity = severities[severityCode];
  if (!severity) throw new Error(`invalid A11YV severity code: ${severityCode}`);

  return {
    scheme: "A11YV",
    version: "1.0",
    vector,
    metrics: parseA11yvMetrics(vector),
    score: scoreTenths / 10,
    severity,
    modalities: [...new Set(modalities)].sort(),
    requirements: {
      wcagCriteria: [...new Set(wcagCriteria)].sort(),
    },
    evidence: {
      kernel: "itrs-edge-wasm-v1",
      vectorSha256: await sha256(bytes),
    },
    note: "remediation-priority-not-wcag-conformance",
  };
}

function syntheticResources(unavailable: Set<string>): WireValue[][] {
  const state = (id: string): string => unavailable.has(id) ? "unavailable" : "available";

  return [
    [
      "asl-local",
      "webrtc:room/asl-local",
      "ASL",
      "video,rtt",
      "telecommunicator",
      "local",
      "MD,Frederick",
      "urn:service:sos",
      state("asl-local"),
      "itrs-edge-synthetic",
      "2026-09-20",
      100,
      false,
    ],
    [
      "asl-regional",
      "webrtc:room/asl-regional",
      "ASL",
      "video,rtt",
      "telecommunicator",
      "regional",
      "MD",
      "urn:service:sos",
      state("asl-regional"),
      "itrs-edge-synthetic",
      "2026-09-20",
      80,
      false,
    ],
    [
      "vrs-bridge",
      "webrtc:room/vrs-bridge",
      "ASL",
      "video,rtt",
      "bridge",
      "national",
      "*",
      "urn:service:sos",
      state("vrs-bridge"),
      "itrs-edge-synthetic",
      "2026-09-20",
      60,
      false,
    ],
  ];
}

export function listSyntheticCapabilities(): Array<Record<string, WireValue>> {
  return syntheticResources(new Set()).map((resource) => ({
    id: resource[0],
    endpoint: resource[1],
    language: resource[2],
    media: resource[3],
    role: resource[4],
    scope: resource[5],
    jurisdictions: resource[6],
    services: resource[7],
    state: resource[8],
    authoritySource: resource[9],
    authorityVersion: resource[10],
    priority: resource[11],
  }));
}

async function instantiateKernel(): Promise<EdgeWasmExports> {
  const instance = await WebAssembly.instantiate(edgeModule, {});
  return instance.exports as unknown as EdgeWasmExports;
}

function runKernel(exports: EdgeWasmExports, call: EdgeCall, payload: WireValue): Uint8Array {
  const input = encode(payload);
  if (input.length > exports.itrs_edge_wasm_input_capacity()) {
    throw new RangeError("iTRS Edge request exceeds Wasm input capacity");
  }

  const inputPtr = exports.itrs_edge_wasm_input_ptr();
  new Uint8Array(exports.memory.buffer, inputPtr, input.length).set(input);

  const rc = exports[call](input.length);
  if (rc !== 0) throw new Error(`iTRS Edge Wasm call failed with rc=${rc}`);

  const outputPtr = exports.itrs_edge_wasm_output_ptr();
  const outputLength = exports.itrs_edge_wasm_output_len();
  if (outputLength > exports.itrs_edge_wasm_output_capacity()) {
    throw new Error("iTRS Edge Wasm reported invalid output length");
  }

  return new Uint8Array(exports.memory.buffer.slice(outputPtr, outputPtr + outputLength));
}

async function sha256(bytes: Uint8Array): Promise<string> {
  const digest = await crypto.subtle.digest("SHA-256", Uint8Array.from(bytes).buffer);
  return Array.from(new Uint8Array(digest), (byte) => byte.toString(16).padStart(2, "0")).join("");
}

function candidateFromWire(value: WireValue, index: number): CapabilityCandidate {
  const fields = asArray(value, `candidate[${index}]`);
  if (fields.length !== 10) throw new Error(`candidate[${index}] has invalid field count`);
  return {
    id: asString(fields[0], "candidate.id"),
    endpoint: asString(fields[1], "candidate.endpoint"),
    role: asString(fields[2], "candidate.role"),
    scope: asString(fields[3], "candidate.scope"),
    authoritySource: asString(fields[4], "candidate.authoritySource"),
    authorityVersion: asString(fields[5], "candidate.authorityVersion"),
    eligible: asBoolean(fields[6], "candidate.eligible"),
    classRank: asNumber(fields[7], "candidate.classRank"),
    priority: asNumber(fields[8], "candidate.priority"),
    reasonMask: asNumber(fields[9], "candidate.reasonMask"),
  };
}

export async function resolveAccessibility(input: ResolveAccessibilityInput): Promise<EdgeResolution> {
  const unavailable = new Set(input.unavailableResources ?? []);
  const known = new Set(["asl-local", "asl-regional", "vrs-bridge"]);
  for (const id of unavailable) {
    if (!known.has(id)) throw new Error(`unknown synthetic resource: ${id}`);
  }

  const requestId = input.requestId ?? crypto.randomUUID();
  const snapshotIds = [...unavailable].sort();
  const resourceSnapshot = `synthetic-v1:down=${snapshotIds.join(",") || "none"}`;
  const policyVersion = "itrs-edge-policy-v1";

  const request: WireValue[] = [
    requestId,
    input.service,
    input.language,
    input.media,
    input.jurisdictionPath,
    input.authoritativePsapId,
    input.authoritativePsapEndpoint,
    resourceSnapshot,
    policyVersion,
  ];
  const requestEnvelope: WireValue[] = [1, 0, request];

  const kernel = await instantiateKernel();
  const effectBytes = runKernel(kernel, "itrs_edge_wasm_step", requestEnvelope);
  const effect = asArray(decode(effectBytes), "effect");
  if (
    effect.length !== 5 ||
    effect[1] !== 1 ||
    effect[2] !== "service.lookup" ||
    effect[3] !== "org.itrsng.asl.resource"
  ) {
    throw new Error("unexpected effect emitted by iTRS Edge kernel");
  }

  const effectRequest = asArray(effect[4], "effect.request");
  const resources = syntheticResources(unavailable);
  const resumeEnvelope: WireValue[] = [1, 2, effectRequest, resources];
  const resumeBytes = encode(resumeEnvelope);
  const resultBytes = runKernel(kernel, "itrs_edge_wasm_resume", resumeEnvelope);
  const done = asArray(decode(resultBytes), "done");
  if (done.length !== 3 || done[1] !== 3) {
    throw new Error("iTRS Edge kernel did not return a done envelope");
  }

  const result = asArray(done[2], "result");
  if (result.length !== 11) throw new Error("iTRS Edge result has invalid field count");
  const candidates = asArray(result[10], "result.candidates").map(candidateFromWire);
  const resultPsap = asString(result[1], "result.authoritativePsapId");

  return {
    requestId: asString(result[0], "result.requestId"),
    authoritativePsapId: resultPsap,
    authoritativePsapEndpoint: asString(result[2], "result.authoritativePsapEndpoint"),
    resourceSnapshot: asString(result[3], "result.resourceSnapshot"),
    policyVersion: asString(result[4], "result.policyVersion"),
    selected: asBoolean(result[5], "result.selected"),
    selectedId: asString(result[6], "result.selectedId"),
    selectedEndpoint: asString(result[7], "result.selectedEndpoint"),
    selectedRole: asString(result[8], "result.selectedRole"),
    selectedScope: asString(result[9], "result.selectedScope"),
    candidates,
    invariant: {
      authoritativePsapPreserved: resultPsap === input.authoritativePsapId,
    },
    evidence: {
      kernel: "itrs-edge-wasm-v1",
      requestSha256: await sha256(encode(requestEnvelope)),
      effectSha256: await sha256(effectBytes),
      resumeSha256: await sha256(resumeBytes),
      resultSha256: await sha256(resultBytes),
    },
  };
}
