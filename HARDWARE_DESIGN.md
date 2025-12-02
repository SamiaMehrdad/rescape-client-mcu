# Hardware Design - XIAO ESP32-C3 Escape Room Client

## Design Philosophy

This project demonstrates **efficient hardware design** by maximizing the use of on-board components and minimizing external parts.

## Minimal External Components

### What You Need to Add

| Component                      | Quantity | Connection                 | Purpose                      |
| ------------------------------ | -------- | -------------------------- | ---------------------------- |
| **Status LED**                 | 1        | GPIO 3 → LED → 220Ω → GND  | Visual system status         |
| **Current limiting resistor**  | 1        | 220Ω-470Ω                  | LED protection               |
| **Trimmer potentiometer**      | 1        | 3.3V → Wiper(GPIO 2) → GND | Device type selection (0-31) |
| **Optional: Filter capacitor** | 1        | GPIO 2 → 0.1µF → GND       | ADC noise filtering          |

**Total BOM:** 2-4 components (LED, resistor, trimmer, optional cap)

### What's Already on the XIAO ESP32-C3

| Feature          | Hardware             | GPIO     | Advantage                          |
| ---------------- | -------------------- | -------- | ---------------------------------- |
| **User Button**  | On-board boot button | GPIO 9   | ✅ No external button needed!      |
| **USB Serial**   | Built-in USB-C       | -        | Debugging without USB-UART adapter |
| **Reset Button** | On-board reset       | -        | Easy programming/debugging         |
| **Power LED**    | On-board             | -        | Basic power indication             |
| **Pull-ups**     | Internal             | All GPIO | No external resistors needed       |

## Pin Utilization Summary

### On-Board Resources Used

```
GPIO 9  = Boot button (BTN1)    ← Perfect for user input!
GPIO 6  = I2C SDA              ← Default I2C pins
GPIO 7  = I2C SCL              ← Default I2C pins
GPIO 20 = UART RX (RS-485)     ← Room Bus communication
GPIO 21 = UART TX (RS-485)     ← Room Bus communication
```

### External Components Required

```
GPIO 2  = Config ADC            ← Trimmer pot (device type)
GPIO 3  = Status LED            ← Single LED + resistor
GPIO 4  = WS2812B Data         ← Addressable LED strip
GPIO 5  = Speaker PWM          ← Audio output (piezo/speaker)
GPIO 8  = RS-485 DE (optional) ← Usually auto-direction
GPIO 10 = I/O Expander INT     ← Optional interrupt pin
```

## The Boot Button Advantage

### Why Using GPIO 9 Boot Button is Perfect

**Multi-Purpose Design:**

1. **During Power-Up:** Hold for bootloader mode (firmware upload)
2. **During Runtime:** User interface button (color cycling, animations)

**Technical Benefits:**

-   ✅ **High-quality tactile switch** - Better than cheap external buttons
-   ✅ **Proper hardware design** - Built-in pull-up, debounced PCB traces
-   ✅ **No GPIO wasted** - Would need this pin reserved anyway for programming
-   ✅ **Cost savings** - No need to buy/solder external button
-   ✅ **Cleaner PCB** - Less components, less soldering
-   ✅ **More reliable** - Factory-installed switch vs hand-soldered

**User Experience:**

-   Single, easy-to-find button on the board
-   Natural location (edge of board)
-   Can't accidentally press wrong button (only one!)

## Device Type Configuration

### Trimmer Potentiometer Approach

**Why Trimmer vs Jumpers:**

-   32 device types (5-bit resolution) vs 2-4 types with jumpers
-   One component vs multiple jumper headers
-   Smooth voltage adjustment vs discrete steps
-   Easy to change without opening enclosure (if accessible)

**Implementation:**

```
3.3V ─────┬─────────────────┐
          │                 │
10kΩ-25kΩ │                 │
Trimmer   │                 │
Pot       ├──── Wiper ──────► GPIO 2 (ADC)
          │                 │
          └─────────────────┴──── GND
```

**Result:** 32 unique device configurations from a single firmware image!

## Status LED - Simple but Effective

### Visual Feedback Without Serial Monitor

```
GPIO 3 ───┬──── LED (+)
          └──── 220Ω ──── LED (-) ──── GND
```

**Status Indication:**

-   **Solid ON** = Everything working perfectly
-   **Fast blink (5Hz)** = I2C error (no I/O expander found)
-   **Slow blink (1Hz)** = Configuration error (reserved)

**Why It's Better Than Serial:**

-   Works in deployed/enclosed installations
-   Instant visual check without USB connection
-   Non-technical users can verify system health
-   Lower power than constantly running USB serial

## Comparison: Before vs After

### Original Design (Hypothetical)

```
Components needed:
- 2× Push buttons
- 2× 10kΩ pull-up resistors (for buttons)
- 1× Status LED
- 1× LED resistor
- 4× Jumper headers (for device type selection)
- 4× Jumpers (for configuration)
= 14 components to purchase/install
```

### Optimized Design (Current)

```
Components needed:
- 0× Push buttons (using on-board!)
- 0× Pull-up resistors (internal!)
- 1× Status LED
- 1× LED resistor
- 1× Trimmer pot (replaces 4 jumpers + headers)
= 3 components to purchase/install
```

**Savings:**

-   🎯 **11 fewer components** to buy
-   🎯 **10 fewer solder joints**
-   🎯 **Simpler assembly**
-   🎯 **Lower BOM cost**
-   🎯 **Smaller PCB possible**

## Best Practices Demonstrated

### 1. Use On-Board Resources First

✅ Boot button for user input ✅ USB-C for power and programming ✅ Internal pull-ups/pull-downs

### 2. Minimize External Components

✅ Single LED for status (not RGB, not multiple LEDs) ✅ One trimmer pot vs multiple jumpers ✅ Optional components clearly marked

### 3. Design for Manufacturability

✅ Fewer components = faster assembly ✅ Common components (220Ω resistor, standard LED) ✅ No specialized or hard-to-find parts

### 4. Dual-Purpose Pins

✅ GPIO 9: Boot button during programming, user button during runtime ✅ GPIO 2: Can be I/O expander interrupt OR ADC input (optional feature)

## Future Expansion Options

### Available GPIOs

```
GPIO 10 = Free (was BTN2)         ← Could be: Sensor input, Extra LED, etc.
GPIO 1  = Free (ADC capable)      ← Could be: Analog sensor, Battery monitor
GPIO 3  = Status LED (could share) ← Could be: PWM output when not blinking
```

### Potential Additions (Without Hardware Changes)

-   **GPIO 10:** Magnetic door sensor (on/off input)
-   **GPIO 1:** Battery voltage monitoring (ADC)
-   **Shared GPIO 3:** Buzzer output (when not using status LED)

## Conclusion

This design exemplifies **elegant engineering**:

-   Maximum functionality
-   Minimum components
-   Using what's already there (boot button!)
-   Clean, professional result

The XIAO ESP32-C3's built-in boot button is the perfect example of **good hardware selection** - choosing a development board that already has what you need, rather than fighting against hardware limitations.

**Engineering Lesson:** Sometimes the best design decision is realizing you don't need to add anything at all! 🎯
