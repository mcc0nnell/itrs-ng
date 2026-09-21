import { readFile, writeFile } from "node:fs/promises";
import { createHash } from "node:crypto";
import { argv, exit } from "node:process";

const [wasmPath, evidencePath] = argv.slice(2);
if (!wasmPath || !evidencePath) {
  console.error("usage: node twitch.mjs <module.wasm> <evidence.json>");
  exit(2);
}

const enc = new TextEncoder();
const dec = new TextDecoder();
const sha256 = (data) => createHash("sha256").update(data).digest("hex");
const must = (condition, message) => {
  if (!condition) throw new Error(message);
};

const wasmBytes = new Uint8Array(await readFile(wasmPath));
const module = new WebAssembly.Module(wasmBytes);
const imports = WebAssembly.Module.imports(module);
must(imports.length === 0, `expected zero imports, found ${JSON.stringify(imports)}`);

const instance = await WebAssembly.instantiate(module, {});
const abi = instance.exports;
const memory = abi.memory;

function withBuffer(bytes, fn) {
  const ptr = abi.sipx_alloc(bytes.length);
  must(ptr !== 0, "sipx_alloc returned 0");
  new Uint8Array(memory.buffer, ptr, bytes.length).set(bytes);
  try {
    return fn(ptr, bytes.length);
  } finally {
    abi.sipx_free(ptr, bytes.length);
  }
}

function unpack(packed) {
  return {
    ptr: Number(packed >> 32n),
    len: Number(packed & 0xffffffffn),
  };
}

function drain(handle) {
  const records = [];
  for (;;) {
    const packed = abi.sipx_next_output(handle);
    if (packed === 0n) break;
    const { ptr, len } = unpack(packed);
    const framed = new Uint8Array(memory.buffer, ptr, len);
    const view = new DataView(framed.buffer, framed.byteOffset, framed.byteLength);

    const type = view.getUint32(0, true);
    const payloadLen = view.getUint32(4, true);
    const payload = framed.slice(8, 8 + payloadLen);
    if (type === 1 || type === 4) {
      records.push({ type, text: dec.decode(payload) });
    } else if (type === 2) {
      const timers = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
      records.push({
        type,
        id: timers.getBigUint64(0, true).toString(),
        fireAtMs: timers.getBigUint64(8, true).toString(),
      });
    } else if (type === 3) {
      const timers = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
      records.push({ type, id: timers.getBigUint64(0, true).toString() });
    } else {
      throw new Error(`unknown output record type ${type}`);
    }
  }
  return records;
}

function snapshot(handle) {
  const packed = abi.sipx_snapshot(handle);
  const { ptr, len } = unpack(packed);
  return JSON.parse(dec.decode(new Uint8Array(memory.buffer, ptr, len)));
}

function header(message, name) {
  const prefix = `${name}: `;
  const line = message.split("\r\n").find((value) => value.startsWith(prefix));
  return line ? line.slice(prefix.length) : null;
}

function respondTo(request, status, extra = []) {
  let response = `SIP/2.0 ${status}\r\n`;
  for (const name of ["Via", "From", "To", "Call-ID", "CSeq"]) {
    const value = header(request, name);
    if (value) response += `${name}: ${value}\r\n`;
  }
  for (const line of extra) response += `${line}\r\n`;
  return response + "Content-Length: 0\r\n\r\n";
}

function inputBytes(handle, text, nowMs) {
  return withBuffer(enc.encode(text), (ptr, len) =>
    abi.sipx_input_bytes(handle, ptr, len, BigInt(nowMs)),
  );
}

const config = enc.encode(
  '{"v":1,"aor":"sip:alice@example.net","auth":{"username":"alice","password":"secret"},' +
    '"transport":{"scheme":"wss","host":"edge.example.net","resource":"/sip"},"insecure":"refuse"}',
);

const handle = withBuffer(config, (ptr, len) => abi.sipx_kernel_new(ptr, len));
must(handle > 0, `sipx_kernel_new returned ${handle}`);

const entropy = Uint8Array.from({ length: 256 }, (_, i) => (0x80 + i) & 0xff);
must(
  withBuffer(entropy, (ptr, len) => abi.sipx_input_entropy(handle, ptr, len)) === 0,
  "entropy input failed",
);
drain(handle);

const registerCommand = enc.encode('{"v":1,"cmd":"register","id":1,"expires":600}');
must(
  withBuffer(registerCommand, (ptr, len) => abi.sipx_command(handle, ptr, len, 0n)) === 0,
  "register command failed",
);

const firstRecords = drain(handle);
const first = firstRecords.find((r) => r.type === 1)?.text;
must(first?.startsWith("REGISTER sip:example.net SIP/2.0\r\n"), "first REGISTER missing");
must(header(first, "Authorization") === null, "first REGISTER unexpectedly authenticated");

const challenge = respondTo(first, "401 Unauthorized", [
  'WWW-Authenticate: Digest realm="example.net", nonce="dcd98b7102dd2f0e", qop="auth", algorithm=SHA-256',
]);

must(inputBytes(handle, challenge, 20) === 0, "401 input failed");
const challengeRecords = drain(handle);
const second = challengeRecords.find((r) => r.type === 1)?.text;
must(second?.startsWith("REGISTER sip:example.net SIP/2.0\r\n"), "retry REGISTER missing");

const authorization = header(second, "Authorization") ?? "";
must(authorization.startsWith("Digest "), "digest Authorization missing");
must(authorization.includes('realm="example.net"'), "digest realm missing");
must(authorization.includes("algorithm=SHA-256"), "SHA-256 digest not selected");
must(!authorization.includes("secret"), "credential leaked onto wire");

const ok = respondTo(second, "200 OK", ["Expires: 600"]);
must(inputBytes(handle, ok, 40) === 0, "200 input failed");
const finalRecords = drain(handle);
const finalState = snapshot(handle);

const events = [...firstRecords, ...challengeRecords, ...finalRecords]
  .filter((record) => record.type === 4)
  .map((record) => JSON.parse(record.text));

must(finalState.registration === "registered", "registration did not reach registered");
must(
  events.some((event) => event.evt === "registration" && event.state === "registered"),
  "registered event missing",
);

const evidence = {
  specimen: "itrs-ng/wasm-sip-twitch",
  sipx_commit: "cb71afd95a0fe2bf7405b45285cae17a2195b4d4",
  wasm: {
    bytes: wasmBytes.length,
    sha256: sha256(wasmBytes),
    imports,
    abi_version: abi.sipx_abi_version(),
  },
  twitch: {
    input: "REGISTER -> 401 -> authenticated REGISTER -> 200",
    first_wire_sha256: sha256(first),
    retry_wire_sha256: sha256(second),
    request_line: first.split("\r\n", 1)[0],
    authenticated_retry: true,
    digest_algorithm: "SHA-256",
    credential_on_wire: false,
  },
  events,
  final_snapshot: finalState,
};

await writeFile(evidencePath, JSON.stringify(evidence, null, 2) + "\n");
must(abi.sipx_kernel_free(handle) === 0, "kernel free failed");

console.log("iTRS-NG WASM SIP twitch: PASS");
console.log(`  wasm: ${wasmBytes.length} bytes sha256:${evidence.wasm.sha256}`);
console.log("  reflex: REGISTER -> 401 -> authenticated REGISTER -> 200");
console.log(`  final: registration=${finalState.registration}`);
console.log(`  evidence: ${evidencePath}`);
