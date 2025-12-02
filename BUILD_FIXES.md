# Build Fixes - Status LED & Single Button Update

## Issues Fixed

### 1. Missing Build Flag (CRITICAL)

**Problem:** Compiler couldn't find pin definitions (`STATUS_LED_PIN`, `CONFIG_ADC_PIN`) **Error:** `#error No board defined! Define SEEED_XIAO_ESP32C3 or S2_MINI.`

**Solution:** Added build flag to `platformio.ini`

```ini
build_flags =
    -D SEEED_XIAO_ESP32C3
```

### 2. Duplicate Board Definition (WARNING)

**Problem:** `SEEED_XIAO_ESP32C3` defined twice - in both `main.cpp` and build flags **Warning:** `warning: "SEEED_XIAO_ESP32C3" redefined`

**Solution:** Removed `#define SEEED_XIAO_ESP32C3` from `src/main.cpp` line 1

-   Board selection is now **exclusively** configured in `platformio.ini`
-   This allows easier multi-board support without modifying source code

### 3. Pin Conflicts in XIAO ESP32-C3

**Problem:** GPIO 2 was assigned to both `CONFIG_ADC_PIN` and `IO_EXPANDER_INT_PIN`

**Solution:** Reassigned `IO_EXPANDER_INT_PIN` to GPIO 10 (freed by removing BTN2)

```cpp
constexpr u8 CONFIG_ADC_PIN = 2;        // D0/A0 - Device type (ADC)
constexpr u8 IO_EXPANDER_INT_PIN = 10;  // D10 - I/O Expander interrupt (optional)
```

### 3. S2_MINI Board Support

**Problem:** S2_MINI section still had BTN2 and missing new pins

**Solution:** Updated S2_MINI section to match XIAO configuration:

-   Removed `BTN_2_PIN`
-   Added `STATUS_LED_PIN = 3`
-   Added `CONFIG_ADC_PIN = 1` (GPIO 1 for S2 ADC)
-   Updated `IO_EXPANDER_INT_PIN = 12`

## Pin Assignments Summary

### XIAO ESP32-C3 (Current Board)

| Function    | GPIO | Pin Label | Notes                             |
| ----------- | ---- | --------- | --------------------------------- |
| BTN1 (Boot) | 9    | D9        | Only button (BTN2 removed)        |
| Status LED  | 3    | D1/A1     | Status indicator (new)            |
| Config ADC  | 2    | D0/A0     | Device type trimmer pot (5-bit)   |
| I2C SDA     | 6    | D4        | I/O Expander communication        |
| I2C SCL     | 7    | D5        | I/O Expander communication        |
| UART TX     | 21   | D6        | Room Bus RS-485                   |
| UART RX     | 20   | D7        | Room Bus RS-485                   |
| RS485 DE    | 8    | D8        | Driver Enable (unused with auto)  |
| Speaker     | 5    | D3        | PWM audio output                  |
| WS2812B     | 4    | D2        | Addressable LED strip             |
| I/O Int     | 10   | D10       | I/O Expander interrupt (optional) |

### S2-MINI (Alternative Board)

| Function   | GPIO | Notes                              |
| ---------- | ---- | ---------------------------------- |
| BTN1       | 14   | Only button                        |
| Status LED | 3    | Status indicator                   |
| Config ADC | 1    | Device type trimmer pot (ADC1_CH0) |
| I/O Int    | 12   | I/O Expander interrupt             |

## Changes to Build Process

### Before

```bash
# Had to manually edit main.cpp to select board
# src/main.cpp line 1:
#define SEEED_XIAO_ESP32C3  # Edit this to change board

platformio run  # Would fail with "No board defined" error
```

### After

```bash
# Board is configured in platformio.ini automatically
# No source code changes needed to switch boards

platformio run  # Compiles successfully with -D SEEED_XIAO_ESP32C3 flag
```

**Advantages:**

-   ✅ Board selection in one place (`platformio.ini`)
-   ✅ No need to modify source files when switching boards
-   ✅ Can have multiple environments for different boards
-   ✅ No duplicate definition warnings

## Verification

To verify the fix:

1. Clean build directory: `platformio run --target clean`
2. Build project: `platformio run`
3. Upload to board: `platformio run --target upload`

The build should complete without errors and the status LED should turn ON at startup.

## Future Considerations

If adding support for more boards, update:

1. `platformio.ini` - Add new `[env:board_name]` section with appropriate build flag
2. `include/mcupins.h` - Add new `#elif defined(BOARD_NAME)` section with pin mappings

Example for ESP32-S3:

```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
build_flags =
    -D ESP32_S3_DEVKIT
```

Then in `mcupins.h`:

```cpp
#elif defined(ESP32_S3_DEVKIT)
// ESP32-S3 pin definitions here...
#endif
```
