/*
    re-escape room project
    roombus.h - Room Bus protocol definitions
    Created by MSK, June 2024
*/
#ifndef ROOM_BUS_H
#define ROOM_BUS_H

#include "msk.h"
#include <stdint.h>
#include <string.h> // for memset if you want

// ---------- Addresses ----------
#define ADDR_SERVER 0x01
#define ADDR_BROADCAST 0xFE
#define ADDR_UNASSIGNED 0x00 // factory / pairing mode
// ---------- Command ranges ----------
#define CORE_MIN 0x01
#define CORE_MAX 0x3F

#define SERVER_MIN 0x40 // server→device only
#define SERVER_MAX 0x7F

#define EVENT_MIN 0x80 // device→server only
#define EVENT_MAX 0xFF

// ---------- Device types ----------
// Device IDs are defined in deviceconfig.h (enum DeviceType).
// Use DeviceType from that header instead of a duplicate enum here to avoid drift.

// ---------- Core ops (0x01–0x1F) ----------
typedef struct
{
    u8 addr;     // destination address
    u8 cmd_srv;  // server→device command (0 when not used)
    u8 cmd_dev;  // device→server event / core op (0 when not used)
    u8 p[20];    // fixed parameters (unused = 0)
    u8 reserved; // future flags / seq / etc.
} RoomFrame;

// ---------- Server→device command ----------
// NOTE: Commands in the server→device range are interpreted by the receiving
// device type. The `cmd_srv` value alone is NOT globally unique — the
// destination `addr` (device ID) + `cmd_srv` together determine the concrete
// action. This allows multiple device types to reuse the same numeric command
// slots (e.g., 0x40, 0x41, ...) while keeping the wire protocol compact.
typedef enum
{
    // Core (common across all devices; 0x01-0x3F)
    CORE_HELLO = 0x01,       // device announces presence
    CORE_ACK = 0x02,         // ack for reliability/simple handshake
    CORE_PING = 0x03,        // liveness check
    CORE_RESET = 0x04,       // soft reset/restart
    CORE_SET_ADDRESS = 0x05, // server assigns address to device

    // Device-specific commands start at 0x40

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

    // Purger
    PURGER_SET_STATE = 0x40,

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

} RoomServerCommand;

// ---------- Device→server events (0x80–0xFF) ----------
typedef enum
{
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
    EV_DEVICE_ERROR = 0x8F,
    EV_PUZZLE_SOLVED = 0x90,
    EV_PUZZLE_FAILED = 0x91,
} RoomDeviceEvent;

// ---------- Helpers ----------

// device -> server (events, HELLO, ACK, etc.)
static inline void room_frame_init_device(RoomFrame *f, u8 cmd_dev)
{
    f->addr = ADDR_SERVER;
    f->cmd_srv = 0;
    f->cmd_dev = cmd_dev;
    memset(f->p, 0, sizeof(f->p));
    f->reserved = 0;
}

// server -> device (commands, ASSIGN_ADDR, etc.)
static inline void room_frame_init_server(RoomFrame *f, u8 deviceAddr, u8 cmd_srv)
{
    f->addr = deviceAddr;
    f->cmd_srv = cmd_srv;
    f->cmd_dev = 0;
    memset(f->p, 0, sizeof(f->p));
    f->reserved = 0;
}

#endif // ROOM_BUS_H
