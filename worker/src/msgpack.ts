export type WireValue = string | boolean | number | WireValue[];

function concat(parts: Uint8Array[]): Uint8Array {
  const length = parts.reduce((total, part) => total + part.length, 0);
  const out = new Uint8Array(length);
  let offset = 0;
  for (const part of parts) {
    out.set(part, offset);
    offset += part.length;
  }
  return out;
}

export function encode(value: WireValue): Uint8Array {
  if (Array.isArray(value)) {
    const body = value.map(encode);
    const count = value.length;
    if (count <= 15) return concat([Uint8Array.of(0x90 | count), ...body]);
    if (count <= 0xffff) {
      return concat([Uint8Array.of(0xdc, count >> 8, count & 0xff), ...body]);
    }
    throw new RangeError("iTRS Edge wire array exceeds uint16");
  }

  if (typeof value === "string") {
    const bytes = new TextEncoder().encode(value);
    const length = bytes.length;
    if (length <= 31) return concat([Uint8Array.of(0xa0 | length), bytes]);
    if (length <= 0xff) return concat([Uint8Array.of(0xd9, length), bytes]);
    if (length <= 0xffff) {
      return concat([Uint8Array.of(0xda, length >> 8, length & 0xff), bytes]);
    }
    throw new RangeError("iTRS Edge wire string exceeds uint16");
  }

  if (typeof value === "boolean") return Uint8Array.of(value ? 0xc3 : 0xc2);

  if (!Number.isInteger(value) || value < -0x80000000 || value > 0xffffffff) {
    throw new RangeError("iTRS Edge wire number must fit signed/unsigned 32-bit integer");
  }

  if (value >= 0 && value <= 0x7f) return Uint8Array.of(value);
  if (value >= 0 && value <= 0xff) return Uint8Array.of(0xcc, value);
  if (value >= 0 && value <= 0xffff) {
    return Uint8Array.of(0xcd, value >> 8, value & 0xff);
  }
  if (value >= 0) {
    return Uint8Array.of(0xce, value >>> 24, value >>> 16, value >>> 8, value);
  }
  if (value >= -32) return Uint8Array.of(0x100 + value);
  if (value >= -128) return Uint8Array.of(0xd0, value & 0xff);
  if (value >= -32768) return Uint8Array.of(0xd1, value >> 8, value);
  return Uint8Array.of(0xd2, value >> 24, value >> 16, value >> 8, value);
}

export function decode(bytes: Uint8Array): WireValue {
  let offset = 0;
  const decoder = new TextDecoder();

  const u8 = (): number => {
    if (offset >= bytes.length) throw new Error("truncated iTRS Edge wire value");
    return bytes[offset++];
  };
  const u16 = (): number => (u8() << 8) | u8();
  const u32 = (): number =>
    ((u8() * 0x1000000) + (u8() << 16) + (u8() << 8) + u8()) >>> 0;

  const readString = (length: number): string => {
    if (bytes.length - offset < length) throw new Error("truncated iTRS Edge string");
    const value = decoder.decode(bytes.subarray(offset, offset + length));
    offset += length;
    return value;
  };

  const one = (): WireValue => {
    const tag = u8();
    if (tag <= 0x7f) return tag;
    if (tag >= 0xe0) return tag - 0x100;
    if ((tag & 0xe0) === 0xa0) return readString(tag & 0x1f);
    if ((tag & 0xf0) === 0x90) {
      return Array.from({length: tag & 0x0f}, one);
    }

    switch (tag) {
      case 0xc2: return false;
      case 0xc3: return true;
      case 0xcc: return u8();
      case 0xcd: return u16();
      case 0xce: return u32();
      case 0xd0: {
        const value = u8();
        return value & 0x80 ? value - 0x100 : value;
      }
      case 0xd1: {
        const value = u16();
        return value & 0x8000 ? value - 0x10000 : value;
      }
      case 0xd2: {
        const value = u32();
        return value > 0x7fffffff ? value - 0x100000000 : value;
      }
      case 0xd9: return readString(u8());
      case 0xda: return readString(u16());
      case 0xdc: return Array.from({length: u16()}, one);
      default:
        throw new Error(`unsupported iTRS Edge MessagePack tag 0x${tag.toString(16)}`);
    }
  };

  const value = one();
  if (offset !== bytes.length) throw new Error("trailing bytes in iTRS Edge wire value");
  return value;
}
