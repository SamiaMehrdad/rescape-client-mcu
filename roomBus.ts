// Server side helper roomBus.ts
//----------------------------------------------------
// Example device registry map
//const devicesByAddr = new Map<number, RoomDeviceInfo>();
//----------------------------------------------------

// ---------- Addresses ----------
export const ADDR_SERVER = 0x01;
export const ADDR_BROADCAST = 0xfe;
export const ADDR_UNASSIGNED = 0x00;
// all others 0x02 - 0xFD are device addresses

// ---------- Command ranges ----------
export const CMD_CORE_MIN = 0x01;
export const CMD_CORE_MAX = 0x3f;
export const CMD_SERVER_MIN = 0x40;
export const CMD_SERVER_MAX = 0x7f;
export const CMD_EVENT_MIN = 0x80;
export const CMD_EVENT_MAX = 0xff;

//---------- Room Frame ----------
// Matches C struct:
// typedef struct {
//    u8 addr;     // destination address
//    u8 cmd_srv;  // server→device command (0 when not used)
//    u8 cmd_dev;  // device→server event / core op (0 when not used)
//    u8 p[20];    // fixed parameters (unused = 0)
//    u8 reserved; // future flags / seq / etc.
// } RoomFrame;
export interface RoomFrame {
    addr: number; // 1 byte
    cmd_srv: number; // 1 byte
    cmd_dev: number; // 1 byte
    p: Uint8Array; // 20 bytes
    reserved: number; // 1 byte
}

// ---------- Device Types ----------
// Should match DEVICE_TYPE_LIST in deviceconfig.h
export enum DeviceType {
    Terminal = 0,
    GlowButton = 1,
    NumBox = 2,
    Timer = 3,
    GlowDots = 4,
    QB = 5,
    RGBMixer = 6,
    Bomb = 7,
    FinalOrder = 8,
    BallGate = 9,
    Actuator = 10,
    TheWall = 11,
    Scores = 12,
    BallBase = 13,
    // ... others
}

// ---------- Server Commands (0x01-0x7F) ----------
export enum RoomServerCommand {
    // Core
    CORE_HELLO = 0x01,
    CORE_ACK = 0x02,
    CORE_PING = 0x03,
    CORE_RESET = 0x04,

    // Device Specific (0x40+)
    // Glow Button
    GLOW_SET_COLOR = 0x40,

    // Num Box
    NUM_SET_DIGIT_COLOR = 0x40,
    NUM_SET_DIGIT_VAL = 0x41,
    NUM_SET_ROW_NUM = 0x42,

    // Glow Dots
    DOTS_SET_COLORS = 0x40,
    DOTS_SET_MOVE = 0x41,
    DOTS_SET_DELAY = 0x42,
    DOTS_SET_LED = 0x43,

    // Timer
    TMR_SET_COLOR = 0x40,
    TMR_SET_VALUE = 0x41,
    TMR_START = 0x42,
    TMR_PAUSE = 0x43,

    // QB
    QB_SET_COLORS = 0x40,
    QB_SET_MODES = 0x41,

    // Terminal
    TERM_RESET = 0x40,

    // Bomb
    BOMB_SET_STATE = 0x40,

    // Screen
    SCR_LOAD = 0x40,
    SCR_SHOW = 0x41,
    SCR_OFF = 0x42,

    // Actuator
    ACT_OPEN = 0x40,
    ACT_CLOSE = 0x41,

    // Glow Ball
    BALL_ACTIVATE = 0x40,

    // Final Order
    FINAL_RESET = 0x40,

    // Incentives
    INC_SET_VALUE = 0x40,
    INC_SET_EFFECT = 0x41,
    INC_SET_MODE = 0x42,

    // Puzzle
    PUZZLE_RESET = 0x40,
}

// ---------- Device Events (0x80-0xFF) ----------
export enum RoomDeviceEvent {
    EV_GLOW_PRESSED = 0x80,
    EV_TMR_DONE = 0x81,
    EV_QB_PRESSED = 0x82,
    EV_TERM_CODE = 0x83,
    EV_MIXER_RGB = 0x84,
    EV_BALL_ACTIVE = 0x85,
    EV_GATE_DETECT = 0x86,
    EV_GATE_BYPASS = 0x87,
    EV_WALL_HIT = 0x88,
    EV_FINAL_ORDER = 0x89,
    EV_DEVICE_ERROR = 0x8f,
    EV_PUZZLE_SOLVED = 0x90,
    EV_PUZZLE_FAILED = 0x91,
}

// A simple registry entry type to be store per connected device
export interface RoomDeviceInfo {
    addr: number; // 0x02–0xFD
    type: DeviceType;
    usn: string; // from HELLO
    logicalId?: string; // "main-timer", "door-1", etc
    lastSeenTs?: number; // for health
}

// Helper to create a server->device frame
export function createServerFrame(addr: number, cmd_srv: number, params?: ArrayLike<number>): RoomFrame {
    const p = new Uint8Array(20);
    if (params) {
        const len = Math.min(20, params.length);
        for (let i = 0; i < len; i++) p[i] = params[i] & 0xff;
    }
    return { addr: addr & 0xff, cmd_srv: cmd_srv & 0xff, cmd_dev: 0, p, reserved: 0 };
}

// Helper to create a device->server frame (mostly for simulation/testing in JS)
export function createDeviceFrame(cmd_dev: number, params?: ArrayLike<number>): RoomFrame {
    const p = new Uint8Array(20);
    if (params) {
        const len = Math.min(20, params.length);
        for (let i = 0; i < len; i++) p[i] = params[i] & 0xff;
    }
    return { addr: ADDR_SERVER, cmd_srv: 0, cmd_dev: cmd_dev & 0xff, p, reserved: 0 };
}

// Encode to a Buffer for TCP/WebSocket (24 bytes)
export function encodeFrame(frame: RoomFrame): Buffer {
    const buf = Buffer.alloc(24);
    buf[0] = frame.addr & 0xff;
    buf[1] = frame.cmd_srv & 0xff;
    buf[2] = frame.cmd_dev & 0xff;
    for (let i = 0; i < 20; i++) {
        buf[3 + i] = frame.p[i] & 0xff;
    }
    buf[23] = frame.reserved & 0xff;
    return buf;
}

// Decode from a Buffer (expects exactly 24 bytes)
export function decodeFrame(buf: Buffer): RoomFrame | null {
    if (buf.length < 24) return null;
    const addr = buf[0];
    const cmd_srv = buf[1];
    const cmd_dev = buf[2];
    const p = new Uint8Array(20);
    for (let i = 0; i < 20; i++) {
        p[i] = buf[3 + i];
    }
    const reserved = buf[23];
    return { addr, cmd_srv, cmd_dev, p, reserved };
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
export function makeTimerSetValue(deviceAddr: number, seconds: number): RoomFrame {
    // TMR_SET_VALUE = 0x41
    // param[0..3] = seconds (little endian or big endian? C code usually does manual shift)
    // Let's assume Little Endian for ESP32
    const p = [seconds & 0xff, (seconds >> 8) & 0xff, (seconds >> 16) & 0xff, (seconds >> 24) & 0xff];
    return createServerFrame(deviceAddr, RoomServerCommand.TMR_SET_VALUE, p);
}
