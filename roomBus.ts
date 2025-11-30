// Server side helper roomBus.ts
//----------------------------------------------------
// Example device registry map
//const devicesByAddr = new Map<number, RoomDeviceInfo>();
//----------------------------------------------------

// ---------- Addresses ----------
export const ADDR_SERVER = 0x01;
export const ADDR_BROADCAST = 0xfe;
// all others 0x02 - 0xFD are device addresses

// ---------- Command ranges ----------
export const CMD_CORE_MIN = 0x01;
export const CMD_CORE_MAX = 0x1f;
export const CMD_SERVER_MIN = 0x20;
export const CMD_SERVER_MAX = 0x7f;
export const CMD_EVENT_MIN = 0x80;
export const CMD_EVENT_MAX = 0xff;

//---------- Room Frame ----------
export interface RoomFrame {
    addr: number; // 0x01 = server, 0x02–0xFD = device, 0xFE = broadcast, 0x00 and 0xFF invalid
    cmd: number;
    p: Uint8Array; // length 16
}
// ---------- Device Types ----------
export enum DeviceType {
    GlowBtn = 0x01,
    NumBox = 0x02,
    GlowDots = 0x03,
    Timer = 0x04,
    QB = 0x05,
    Terminal = 0x06,
    RGBMixer = 0x07,
    Bomb = 0x08,
    Screen = 0x09,
    Actuator = 0x0a,
    GlowBall = 0x0b,
    GlowGate = 0x0c,
    Wall = 0x0d,
    FinalOrder = 0x0e,
    Incentives = 0x0f,
}
// A simple registry entry type to be store per connected device
export interface RoomDeviceInfo {
    addr: number; // 0x02–0xFD
    type: DeviceType;
    usn: string; // from HELLO
    logicalId?: string; // "main-timer", "door-1", etc
    lastSeenTs?: number; // for health
}

export function createFrame(addr: number, cmd: number, params?: ArrayLike<number>): RoomFrame {
    const p = new Uint8Array(16);
    if (params) {
        const len = Math.min(16, params.length);
        for (let i = 0; i < len; i++) p[i] = params[i] & 0xff;
    }
    return { addr: addr & 0xff, cmd: cmd & 0xff, p };
}

// Encode to a Buffer for TCP/WebSocket
export function encodeFrame(frame: RoomFrame): Buffer {
    const buf = Buffer.alloc(18);
    buf[0] = frame.addr & 0xff;
    buf[1] = frame.cmd & 0xff;
    for (let i = 0; i < 16; i++) {
        buf[2 + i] = frame.p[i] & 0xff;
    }
    return buf;
}

// Decode from a Buffer (expects exactly 18 bytes)
export function decodeFrame(buf: Buffer): RoomFrame | null {
    if (buf.length < 18) return null;
    const addr = buf[0];
    const cmd = buf[1];
    const p = new Uint8Array(16);
    for (let i = 0; i < 16; i++) {
        p[i] = buf[2 + i];
    }
    return { addr, cmd, p };
}

export function isServerCommand(cmd: number): boolean {
    return cmd >= CMD_SERVER_MIN && cmd <= CMD_SERVER_MAX;
}

export function isDeviceEvent(cmd: number): boolean {
    return cmd >= CMD_EVENT_MIN && cmd <= CMD_EVENT_MAX;
}

export function isCoreOp(cmd: number): boolean {
    return cmd >= CMD_CORE_MIN && cmd <= CMD_CORE_MAX;
}

// Example helper: build a Timer SET_VALUE frame
export const CMD_TMR_SET_VALUE = 0x29; // keep in sync with C header

export function makeTimerSetValue(deviceAddr: number, seconds: number): RoomFrame {
    const s = Math.max(0, Math.min(35999, seconds | 0));
    const low = s & 0xff;
    const high = (s >> 8) & 0xff;
    return createFrame(deviceAddr, CMD_TMR_SET_VALUE, [low, high]);
}
