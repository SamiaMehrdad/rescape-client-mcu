# Code Review - Improvement Opportunities

**Date:** December 9, 2025  
**Reviewer:** AI Code Analysis  
**Project:** ESP32 Escape Room Client MCU Firmware

---

## Summary

Overall, the codebase is **well-structured** with good separation of concerns, clear naming conventions, and proper use of namespaces for constants. However, there are several opportunities for improvement in terms of code duplication, maintainability, and architectural refinement.

---

## 🟢 Strengths

1. **Excellent use of namespaces** for configuration constants (ADCConfig, TypeLimits, DetectionTiming)
2. **Good abstraction layers** - PixelStrip, IOExpander, InputManager well isolated
3. **Proper hardware timer management** with IRAM_ATTR for ISRs
4. **Comprehensive documentation** in CHANGELOG.md and README.md
5. **Clean Core/App separation** architecture ready for expansion
6. **Logical keypad/LED abstraction** successfully hides physical wiring

---

## 🟡 Moderate Issues - Code Duplication

### 1. Duplicate `ledControl()` Implementation

**Location:** `src/core.cpp` lines 903-957

**Issue:** Two overloaded functions with significant code duplication

```cpp
void Core::ledControl(u8 logicalIndex, u32 color)
{
    if (logicalIndex >= 16) return;
    u8 physicalLedIndex = kKeyToLedMap[logicalIndex];
    if (physicalLedIndex >= m_pixels->getCount()) return;

    u8 r = (color >> 16) & 0xFF;
    u8 g = (color >> 8) & 0xFF;
    u8 b = color & 0xFF;
    m_pixels->setColor(physicalLedIndex, r, g, b);
}

void Core::ledControl(u8 logicalIndex, u8 r, u8 g, u8 b)
{
    if (logicalIndex >= 16) return;
    u8 physicalLedIndex = kKeyToLedMap[logicalIndex];
    if (physicalLedIndex >= m_pixels->getCount()) return;
    m_pixels->setColor(physicalLedIndex, r, g, b);
}
```

**Recommendation:** Implement delegation pattern

```cpp
void Core::ledControl(u8 logicalIndex, u32 color)
{
    u8 r = (color >> 16) & 0xFF;
    u8 g = (color >> 8) & 0xFF;
    u8 b = color & 0xFF;
    ledControl(logicalIndex, r, g, b); // Delegate to RGB version
}

void Core::ledControl(u8 logicalIndex, u8 r, u8 g, u8 b)
{
    if (logicalIndex >= 16) return;
    u8 physicalLedIndex = kKeyToLedMap[logicalIndex];
    if (physicalLedIndex >= m_pixels->getCount()) return;
    m_pixels->setColor(physicalLedIndex, r, g, b);
}
```

**Impact:** Reduces duplication, easier maintenance

---

### 2. Mode Enter/Exit Pattern Repetition

**Location:** `src/core.cpp` - Type Detection Mode and Keypad Test Mode

**Issue:** Similar pattern repeated for different modes

```cpp
void Core::enterTypeDetectionMode() {
    m_previousMode = m_mode;
    m_mode = MODE_TYPE_DETECTION;
    // ... mode-specific setup
}

void Core::exitTypeDetectionMode() {
    m_mode = m_previousMode;
}

void Core::enterKeypadTestMode() {
    m_previousMode = m_mode;
    m_mode = MODE_KEYPAD_TEST;
    // ... mode-specific setup
}

void Core::exitKeypadTestMode() {
    m_mode = m_previousMode;
}
```

**Recommendation:** Consider a mode manager pattern or template

```cpp
// Helper method
void Core::enterMode(CoreMode newMode) {
    m_previousMode = m_mode;
    m_mode = newMode;
}

void Core::exitMode() {
    m_mode = m_previousMode;
}

void Core::enterTypeDetectionMode() {
    enterMode(MODE_TYPE_DETECTION);
    // ... mode-specific setup only
}

void Core::exitTypeDetectionMode() {
    // ... mode-specific cleanup
    exitMode();
}
```

**Impact:** Less boilerplate, clearer mode transitions

---

## 🟡 Maintainability Issues

### 3. Magic Number in Keypad LED States Array

**Location:** `include/core.h` line 100

```cpp
bool m_keypadLedStates[16]; // Track LED state for each keypad button
```

**Issue:** Hardcoded array size should use constant

**Recommendation:**

```cpp
// In ioexpander.h (where KEYPAD_ROWS and KEYPAD_COLS are defined)
constexpr u8 KEYPAD_SIZE = KEYPAD_ROWS * KEYPAD_COLS; // 16

// In core.h
bool m_keypadLedStates[KEYPAD_SIZE];
```

**Impact:** Better maintainability if keypad size changes

---

### 4. Key-to-LED Mapping Documentation Mismatch

**Location:** `src/core.cpp` lines 77-99

**Issue:** Comment says one mapping, code shows different mapping

```cpp
// Comment says: K1→L11, K4→L14, K7→L13, etc.
// But code has:
const u8 Core::kKeyToLedMap[16] = {
    0,  // K0  -> L0
    7,  // K1  -> L7  ❌ Comment says L11
    8,  // K2  -> L8
    15, // K3  -> L15
    1,  // K4  -> L1  ❌ Comment says L14
    // ...
};
```

**Recommendation:** Update comments to match code or vice versa

**Impact:** Prevents confusion during debugging

---

### 5. Inconsistent Const Correctness

**Location:** Various files

**Issue:** Some getter methods not marked `const`

```cpp
// In core.h
u8 getDeviceType() const { return m_deviceType; }  ✅ Good
CoreMode getMode() const { return m_mode; }        ✅ Good

// But in pixel.h
u8 getCount() const { return logicalCount; }       ✅ Good
u32 *getBuffer() { return colorBuffer; }           ❌ Should be const version too
```

**Recommendation:** Add const overloads where appropriate

```cpp
u32 *getBuffer() { return colorBuffer; }
const u32 *getBuffer() const { return colorBuffer; }
```

**Impact:** Better const-correctness, prevents unintended modifications

---

## 🔴 Architecture Issues

### 6. Tight Coupling in systemInit()

**Location:** `src/core.cpp` Core::systemInit()

**Issue:** systemInit takes 7 parameters, all pointers to global objects

```cpp
static void systemInit(PixelStrip *pixels, Synth *synth, Animation *animation,
                      InputManager *inputManager, RoomSerial *roomBus,
                      Core *core, IOExpander *ioExpander, hw_timer_t **timer);
```

**Recommendation:** Consider dependency injection container or builder pattern

```cpp
struct SystemComponents {
    PixelStrip *pixels;
    Synth *synth;
    Animation *animation;
    InputManager *inputManager;
    RoomSerial *roomBus;
    IOExpander *ioExpander;
    hw_timer_t **timer;
};

static void systemInit(Core *core, const SystemComponents &components);
```

**Impact:** Cleaner interface, easier to extend

---

### 7. Global External Variables

**Location:** `src/core.cpp` lines 18-23

**Issue:** Extern variables create hidden dependencies

```cpp
extern const u8 ISR_INTERVAL_MS;
extern const u8 ANIM_REFRESH_MS;
extern volatile bool pixelUpdateFlag;
extern void IRAM_ATTR refreshTimer();
```

**Recommendation:** Pass as parameters or use singleton/config object

```cpp
struct TimingConfig {
    u8 isrIntervalMs;
    u8 animRefreshMs;
    volatile bool *pixelUpdateFlag;
};

// Pass to Core constructor or init
```

**Impact:** Explicit dependencies, better testability

---

## 🟢 Minor Improvements

### 8. Add constexpr Where Possible

**Location:** Various

**Recommendation:** Use `constexpr` for compile-time constants

```cpp
// In core.cpp
constexpr size_t KEYPAD_SIZE = 16;
constexpr size_t NOTE_MAP_SIZE = 16;
constexpr size_t KEY_TO_LED_MAP_SIZE = 16;

// Enables compile-time array size validation
static_assert(sizeof(kKeyToLedMap) / sizeof(kKeyToLedMap[0]) == KEYPAD_SIZE);
```

---

### 9. Error Return Type Instead of Side Effects

**Location:** `src/ioexpander.cpp` scanKeypad()

**Issue:** Returns 255 for "no event" which is a valid u8 value

**Recommendation:** Use Optional<u8> or std::optional if C++17 available

```cpp
// Alternative: Use struct for clearer semantics
struct KeyEvent {
    bool valid;
    u8 keyIndex;
};

KeyEvent scanKeypad(); // Returns {true, index} or {false, 0}
```

**Impact:** Clearer API, less magic numbers

---

### 10. Extract Constants to Header

**Location:** `src/ioexpander.cpp` lines 157, 222

**Issue:** Magic numbers in keypad scanning

```cpp
if (currentTime - _lastScanTime < 10) // Magic number
if (_debounceCount >= 3) // Magic number
if (currentTime - _lastKeyPressTime >= 200) // Magic number
```

**Recommendation:** Move to namespace in header

```cpp
namespace KeypadConfig {
    constexpr unsigned long SCAN_RATE_MS = 10;
    constexpr u8 DEBOUNCE_COUNT = 3;
    constexpr unsigned long MIN_PRESS_INTERVAL_MS = 200;
}
```

---

## 🔧 Refactoring Priorities

### High Priority

1. ✅ **Fix ledControl duplication** (Quick win, reduces code)
2. ✅ **Update Key-to-LED mapping comments** (Prevents bugs)
3. ✅ **Extract keypad constants** (Improves maintainability)

### Medium Priority

4. **Add mode management helpers** (Reduces boilerplate)
5. **Fix const-correctness** (Better API design)
6. **Use constexpr for array sizes** (Compile-time safety)

### Low Priority (Consider for v2.0)

7. **Refactor systemInit** (Better architecture, but major change)
8. **Replace extern variables** (Testability, but requires main.cpp changes)
9. **Consider Optional<T>** (Cleaner API, requires C++17)

---

## Testing Recommendations

1. **Add unit tests for:**

    - `cellIndex(x, y)` boundary conditions
    - `ledControl()` with invalid indices
    - Key-to-LED mapping verification

2. **Add integration tests for:**

    - Mode transitions
    - Keypad debouncing behavior
    - LED abstraction layer

3. **Add hardware-in-loop tests for:**
    - Keypad scan timing (200ms minimum)
    - LED physical mapping validation

---

## Documentation Gaps

1. **Missing API documentation for:**

    - `ledControl()` thread safety
    - `cellIndex()` return value meaning (0xFF)
    - Mode transition state machine diagram

2. **Add examples for:**
    - Using ledControl() in application code
    - Custom mode implementation guide

---

## Conclusion

The codebase is in **good shape** overall with solid architecture and clean code. The main improvements are:

1. **Eliminate code duplication** in ledControl()
2. **Fix documentation** (Key-to-LED mapping comments)
3. **Extract magic numbers** to named constants
4. **Add const-correctness** where missing

These are **low-risk, high-value** improvements that can be made incrementally without disrupting existing functionality.

**Estimated effort:** 2-4 hours for high-priority items
