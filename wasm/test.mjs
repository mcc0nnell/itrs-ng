import fs from "node:fs";
import assert from "node:assert/strict";

function concat(parts) {
  const length = parts.reduce((n, p) => n + p.length, 0);
  const out = new Uint8Array(length);
  let offset = 0;
  for (const part of parts) {
    out.set(part, offset);
    offset += part.length;
  }
  return out;
}

function encode(value) {
  if (Array.isArray(value)) {
    const body = value.map(encode);
    const n = value.length;
    if (n <= 15) return concat([Uint8Array.of(0x90 | n), ...body]);
    if (n <= 0xffff) return concat([Uint8Array.of(0xdc, n >> 8, n & 0xff), ...body]);
    throw new Error("array too large");
  }
  if (typeof value === "string") {
    const bytes = new TextEncoder().encode(value);
    const n = bytes.length;
    if (n <= 31) return concat([Uint8Array.of(0xa0 | n), bytes]);
    if (n <= 0xff) return concat([Uint8Array.of(0xd9, n), bytes]);
    if (n <= 0xffff) return concat([Uint8Array.of(0xda, n >> 8, n & 0xff), bytes]);
    throw new Error("string too large");
  }
  if (typeof value === "boolean") return Uint8Array.of(value ? 0xc3 : 0xc2);
  if (Number.isInteger(value)) {
    if (value >= 0 && value <= 0x7f) return Uint8Array.of(value);
    if (value >= 0 && value <= 0xff) return Uint8Array.of(0xcc, value);
    if (value >= 0 && value <= 0xffff) return Uint8Array.of(0xcd, value >> 8, value & 0xff);
    if (value >= 0 && value <= 0xffffffff) {
      return Uint8Array.of(0xce, value >>> 24, value >>> 16, value >>> 8, value);
    }
    if (value >= -32 && value < 0) return Uint8Array.of(0x100 + value);
    if (value >= -128) return Uint8Array.of(0xd0, value & 0xff);
    if (value >= -32768) return Uint8Array.of(0xd1, value >> 8, value);
    return Uint8Array.of(0xd2, value >> 24, value >> 16, value >> 8, value);
  }
  throw new TypeError(`unsupported value: ${value}`);
}

function decode(bytes) {
  let offset = 0;
  const text = new TextDecoder();
  const u8 = () => {
    assert.ok(offset < bytes.length);
    return bytes[offset++];
  };
  const u16 = () => (u8() << 8) | u8();
  const u32 = () => ((u8() * 0x1000000) + (u8() << 16) + (u8() << 8) + u8()) >>> 0;

  function one() {
    const tag = u8();
    if (tag <= 0x7f) return tag;
    if (tag >= 0xe0) return tag - 0x100;
    if ((tag & 0xe0) === 0xa0) {
      const n = tag & 0x1f;
      const v = text.decode(bytes.subarray(offset, offset + n));
      offset += n;
      return v;
    }
    if ((tag & 0xf0) === 0x90) {
      const n = tag & 0x0f;
      return Array.from({length: n}, one);
    }
    if (tag === 0xc2) return false;
    if (tag === 0xc3) return true;
    if (tag === 0xcc) return u8();
    if (tag === 0xcd) return u16();
    if (tag === 0xce) return u32();
    if (tag === 0xd0) {
      const v = u8();
      return v & 0x80 ? v - 0x100 : v;
    }
    if (tag === 0xd1) {
      const v = u16();
      return v & 0x8000 ? v - 0x10000 : v;
    }
    if (tag === 0xd2) {
      const v = u32();
      return v > 0x7fffffff ? v - 0x100000000 : v;
    }
    if (tag === 0xd9) {
      const n = u8();
      const v = text.decode(bytes.subarray(offset, offset + n));
      offset += n;
      return v;
    }
    if (tag === 0xda) {
      const n = u16();
      const v = text.decode(bytes.subarray(offset, offset + n));
      offset += n;
      return v;
    }
    if (tag === 0xdc) {
      const n = u16();
      return Array.from({length: n}, one);
    }
    throw new Error(`unsupported MessagePack tag 0x${tag.toString(16)}`);
  }

  const value = one();
  assert.equal(offset, bytes.length, "trailing bytes");
  return value;
}

const request = [
  "req-edge-wasm",
  "urn:service:sos",
  "ASL",
  "video",
  "MD,Frederick",
  "psap-frederick",
  "sip:psap-frederick@example.invalid",
  "snapshot-wasm-1",
  "policy-edge-1",
];

const local = [
  "asl-local",
  "sip:local@example.invalid",
  "ASL",
  "video,rtt",
  "telecommunicator",
  "local",
  "MD,Frederick",
  "urn:service:sos",
  "available",
  "synthetic-registry",
  "2026-09-20",
  100,
  false,
];

const regional = [
  "asl-regional",
  "sip:regional@example.invalid",
  "ASL",
  "video,rtt",
  "telecommunicator",
  "regional",
  "MD",
  "urn:service:sos",
  "available",
  "synthetic-registry",
  "2026-09-20",
  80,
  false,
];

const bridge = [
  "vrs-bridge",
  "sip:vrs@example.invalid",
  "ASL",
  "video,rtt",
  "bridge",
  "national",
  "*",
  "urn:service:sos",
  "available",
  "synthetic-registry",
  "2026-09-20",
  60,
  false,
];

const wasm = fs.readFileSync(new URL("../build-wasm/itrs_edge.wasm", import.meta.url));
const module = await WebAssembly.compile(wasm);
assert.deepEqual(WebAssembly.Module.imports(module), [], "kernel must have zero imports");
const {exports: e} = await WebAssembly.instantiate(module, {});

function run(name, payload) {
  const input = encode(payload);
  assert.ok(input.length <= e.itrs_edge_wasm_input_capacity());
  const inputPtr = e.itrs_edge_wasm_input_ptr();
  new Uint8Array(e.memory.buffer, inputPtr, input.length).set(input);
  const rc = e[name](input.length);
  assert.equal(rc, 0);
  const outputPtr = e.itrs_edge_wasm_output_ptr();
  const outputLen = e.itrs_edge_wasm_output_len();
  return new Uint8Array(e.memory.buffer.slice(outputPtr, outputPtr + outputLen));
}

const effectBytes = run("itrs_edge_wasm_step", [1, 0, request]);
const effect = decode(effectBytes);
assert.equal(effect[1], 1);
assert.equal(effect[2], "service.lookup");
assert.equal(effect[3], "org.itrsng.asl.resource");
assert.deepEqual(effect[4], request);

const localBytes = run("itrs_edge_wasm_resume", [1, 2, request, [local, regional, bridge]]);
const reversedBytes = run("itrs_edge_wasm_resume", [1, 2, request, [bridge, regional, local]]);
assert.deepEqual(localBytes, reversedBytes, "resource order must not affect canonical result");

const localDone = decode(localBytes);
assert.equal(localDone[1], 3);
assert.equal(localDone[2][1], "psap-frederick");
assert.equal(localDone[2][5], true);
assert.equal(localDone[2][6], "asl-local");

const localDown = [...local];
localDown[8] = "unavailable";
const failover = decode(run("itrs_edge_wasm_resume", [1, 2, request, [localDown, regional, bridge]]));
assert.equal(failover[2][1], "psap-frederick");
assert.equal(failover[2][6], "asl-regional");

console.log("itrs-edge-wasm-test: ok");
console.log(`module_bytes=${wasm.length} imports=${WebAssembly.Module.imports(module).length}`);
