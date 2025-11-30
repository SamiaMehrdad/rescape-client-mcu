# RS-485 Room Bus Communication Guide

## Hardware Setup

### Pin Configuration (XIAO ESP32-C3)

-   **RX_PIN**: GPIO 20 (RS-485 Receive)
-   **TX_PIN**: GPIO 21 (RS-485 Transmit)
-   **RS485_DE_PIN**: GPIO 8 (Driver Enable - controls TX/RX mode)

### Wiring

Connect an RS-485 transceiver module (e.g., MAX485):

-   DI (Data In) → GPIO 21 (TX_PIN)
-   RO (Receiver Out) → GPIO 20 (RX_PIN)
-   DE (Driver Enable) → GPIO 8 (RS485_DE_PIN)
-   RE (Receiver Enable) → GPIO 8 (RS485_DE_PIN) - tie together with DE
-   A, B → RS-485 bus lines

## Software Usage

### 1. Include Header

```cpp
#include "roomserial.h"
```

### 2. Create Instance

```cpp
// In global scope
RoomSerial roomBus(RX_PIN, TX_PIN, RS485_DE_PIN, 9600);
```

### 3. Initialize in setup()

```cpp
void setup() {
  Serial.begin(115200);  // USB debug serial
  roomBus.begin();       // RS-485 communication
}
```

### 4. Receiving Frames

```cpp
void loop() {
  RoomFrame rxFrame;
  if (roomBus.receiveFrame(&rxFrame)) {
    // Frame received!
    Serial.print("From: 0x");
    Serial.println(rxFrame.addr, HEX);

    // Process based on command
    if (rxFrame.cmd_srv == CMD_GLOW_SET_COLOR) {
      u8 r = rxFrame.p[0];
      u8 g = rxFrame.p[1];
      u8 b = rxFrame.p[2];
      // Set your LED color...
    }
  }
}
```

### 5. Sending Frames

```cpp
// Send an event to server
RoomFrame txFrame;
room_frame_init_device(&txFrame, EV_GLOW_PRESSED);
txFrame.p[0] = 0x01;  // Example data
roomBus.sendFrame(&txFrame);
```

## Protocol Details

### Frame Format (28 bytes)

```
[0]      START (0xAA)
[1-24]   PAYLOAD (24 bytes):
         [0]     addr
         [1]     cmd_srv
         [2]     cmd_dev
         [3-22]  p[0-19]
         [23]    reserved
[25-26]  CRC16-CCITT
[27]     END (0x55)
```

### Addresses

-   `ADDR_SERVER` (0x01): Main server
-   `ADDR_BROADCAST` (0xFE): All devices
-   `ADDR_UNASSIGNED` (0x00): Unassigned/pairing mode

### Command Examples

#### Server → Device (0x20-0x7F)

```cpp
CMD_GLOW_SET_COLOR = 0x20   // Set LED color
CMD_TMR_START = 0x2A        // Start timer
CMD_ACT_OPEN = 0x34         // Open actuator
```

#### Device → Server (0x80-0xFF)

```cpp
EV_GLOW_PRESSED = 0x80      // Button pressed
EV_TMR_DONE = 0x81          // Timer complete
EV_PUZZLE_SOLVED = 0x90     // Puzzle solved
```

## Example: Complete Device Implementation

```cpp
#include "roomserial.h"

RoomSerial roomBus(RX_PIN, TX_PIN, RS485_DE_PIN, 9600);
u8 myAddress = 0x10;  // Device address

void setup() {
  Serial.begin(115200);
  roomBus.begin();
}

void loop() {
  // Check for incoming commands
  RoomFrame rxFrame;
  if (roomBus.receiveFrame(&rxFrame)) {
    // Only process if addressed to me or broadcast
    if (rxFrame.addr == myAddress || rxFrame.addr == ADDR_BROADCAST) {
      handleCommand(&rxFrame);
    }
  }

  // Send periodic status or events
  if (buttonPressed()) {
    sendButtonEvent();
  }
}

void handleCommand(RoomFrame *frame) {
  switch (frame->cmd_srv) {
    case CMD_GLOW_SET_COLOR:
      setColor(frame->p[0], frame->p[1], frame->p[2]);
      break;
    // ... handle other commands
  }
}

void sendButtonEvent() {
  RoomFrame txFrame;
  room_frame_init_device(&txFrame, EV_GLOW_PRESSED);
  txFrame.p[0] = myAddress;
  roomBus.sendFrame(&txFrame);
}
```

## Debugging

### Check Serial Monitor

The example code prints received frames to Serial Monitor:

```
RS-485 Room Bus initialized
Room Bus frame received! Addr: 0x10 Cmd_srv: 0x20 Cmd_dev: 0x00
Color set via Room Bus command
```

### Common Issues

1. **No communication**: Check wiring, especially A/B polarity
2. **Garbled data**: Verify baud rate matches (default 9600)
3. **One-way only**: Check DE/RE pin connections
4. **CRC errors**: Check for noise, add termination resistors (120Ω)

## Advanced Features

### Without DE Pin

If your transceiver doesn't need DE control (auto-direction):

```cpp
RoomSerial roomBus(RX_PIN, TX_PIN, -1, 9600);  // -1 = no DE pin
```

### Custom Baud Rate

```cpp
RoomSerial roomBus(RX_PIN, TX_PIN, RS485_DE_PIN, 115200);
```

### Direct Serial Access

```cpp
// Access underlying HardwareSerial if needed
roomBus.getSerial().available();
```
