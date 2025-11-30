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
#define CMD_CORE_MIN 0x01
#define CMD_CORE_MAX 0x1F

#define CMD_SERVER_MIN 0x20 // server→device only
#define CMD_SERVER_MAX 0x7F

#define CMD_EVENT_MIN 0x80 // device→server only
#define CMD_EVENT_MAX 0xFF

// ---------- Device types ----------
typedef enum
{
    D_GLOWBTN = 0x01,
    D_NUMBOX = 0x02,
    D_GLOWDOTS = 0x03,
    D_TIMER = 0x04,
    D_QB = 0x05,
    D_TERMINAL = 0x06,
    D_RGBMIXER = 0x07,
    D_BOMB = 0x08,
    D_SCREEN = 0x09,
    D_ACTUATOR = 0x0A,
    D_GLOWBALL = 0x0B,
    D_GLOWGATE = 0x0C,
    D_WALL = 0x0D,
    D_FINALORDER = 0x0E,
    D_INCENTIVES = 0x0F,
    D_PUZZLE = 0x10,
} RoomDeviceType;

// ---------- Core ops (0x01–0x1F) ----------
typedef struct
{
    u8 addr;     // destination address
    u8 cmd_srv;  // server→device command (0 when not used)
    u8 cmd_dev;  // device→server event / core op (0 when not used)
    u8 p[20];    // fixed parameters (unused = 0)
    u8 reserved; // future flags / seq / etc.
} RoomFrame;

// ---------- Server→device commands (0x20–0x7F) ----------
typedef enum
{
    // Glow Button
    CMD_GLOW_SET_COLOR = 0x20,

    // Num Box
    CMD_NUM_SET_DIGIT_COLOR = 0x21,
    CMD_NUM_SET_DIGIT_VAL = 0x22,
    CMD_NUM_SET_ROW_NUM = 0x23,

    // Glow Dots
    CMD_DOTS_SET_COLORS = 0x24,
    CMD_DOTS_SET_MOVE = 0x25,
    CMD_DOTS_SET_DELAY = 0x26,
    CMD_DOTS_SET_LED = 0x27,

    // Timer
    CMD_TMR_SET_COLOR = 0x28,
    CMD_TMR_SET_VALUE = 0x29,
    CMD_TMR_START = 0x2A,
    CMD_TMR_PAUSE = 0x2B,

    // QB
    CMD_QB_SET_COLORS = 0x2C,
    CMD_QB_SET_MODES = 0x2D,

    // Terminal
    CMD_TERM_RESET = 0x2E,

    // Bomb
    CMD_BOMB_SET_STATE = 0x30,

    // Screen
    CMD_SCR_LOAD = 0x31,
    CMD_SCR_SHOW = 0x32,
    CMD_SCR_OFF = 0x33,

    // Actuator
    CMD_ACT_OPEN = 0x34,
    CMD_ACT_CLOSE = 0x35,

    // Glow Ball
    CMD_BALL_ACTIVATE = 0x36,

    // Final Order
    CMD_FINAL_RESET = 0x37,

    // Incentives
    CMD_INC_SET_VALUE = 0x38,
    CMD_INC_SET_EFFECT = 0x39,
    CMD_INC_SET_MODE = 0x3A,

    // Puzzle
    CMD_PUZZLE_RESET = 0x3B,

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
