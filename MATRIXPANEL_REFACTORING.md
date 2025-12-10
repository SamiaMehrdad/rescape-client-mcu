# MatrixPanel Class - Refactoring Documentation

## Overview

Created a dedicated `MatrixPanel` class to encapsulate all keypad+LED matrix functionality, separating concerns and improving code organization.

## Changes Made

### New Files Created

#### `include/matrixpanel.h`

-   **MatrixPanel class interface**
-   Public methods:
    -   `cellIndex(x, y)` - Convert grid coordinates to logical index
    -   `ledControl(index, color)` - Set LED by logical index (two overloads)
    -   `setCell(x, y, color)` - Set LED by grid coordinates (two overloads)
    -   `clear()` - Turn off all LEDs
    -   `fill(color)` - Set all LEDs to same color (two overloads)
    -   `getRows()`, `getCols()`, `getSize()` - Query matrix dimensions
-   Private data:
    -   `kKeyToLedMap[16]` - Physical wiring mapping array
    -   `m_pixels` - Pointer to PixelStrip

#### `src/matrixpanel.cpp`

-   Complete implementation of all MatrixPanel methods
-   Moved `kKeyToLedMap` array from Core class
-   Implements logical-to-physical LED index translation
-   Provides high-level operations (clear, fill, setCell)

### Modified Files

#### `include/core.h`

-   Added `#include "matrixpanel.h"`
-   Added `MatrixPanel *m_matrixPanel` member
-   Added `getMatrixPanel()` accessor method
-   Kept `cellIndex()` and `ledControl()` as convenience delegates
-   Removed `kKeyToLedMap` static member (moved to MatrixPanel)
-   Updated comments to indicate delegation to MatrixPanel

#### `src/core.cpp`

-   Removed `kKeyToLedMap[16]` array definition (moved to MatrixPanel)
-   Updated constructor to initialize `m_matrixPanel`
-   Simplified `cellIndex()` to delegate to `m_matrixPanel->cellIndex()`
-   Simplified `ledControl()` methods to delegate to `m_matrixPanel->ledControl()`
-   Removed all physical LED mapping logic (now in MatrixPanel)

## Benefits

### Separation of Concerns

-   **Core class**: High-level firmware logic, modes, device types
-   **MatrixPanel class**: Low-level matrix hardware abstraction

### Improved Maintainability

-   All matrix-related code in one place
-   Easy to modify LED wiring without touching Core
-   Clear API for matrix operations

### Enhanced Functionality

-   New high-level methods (`clear()`, `fill()`, `setCell()`)
-   Direct access via `getMatrixPanel()` for advanced use
-   Convenience delegates in Core for backward compatibility

### Code Reusability

-   MatrixPanel can be used independently
-   Easy to create multiple matrix instances if needed
-   Clear abstraction suitable for other projects

## API Examples

### Using Core delegates (backward compatible)

```cpp
// Using cellIndex and ledControl (still works as before)
u8 index = core->cellIndex(2, 1);  // Column 2, Row 1 -> index 6
core->ledControl(index, COLOR_RED);
```

### Using MatrixPanel directly

```cpp
// Get matrix panel
MatrixPanel *matrix = core->getMatrixPanel();

// Set cell by coordinates
matrix->setCell(2, 1, COLOR_RED);

// Fill all LEDs
matrix->fill(COLOR_BLUE);

// Clear matrix
matrix->clear();

// Query dimensions
u8 rows = matrix->getRows();    // 4
u8 cols = matrix->getCols();    // 4
u8 size = matrix->getSize();    // 16
```

## Testing

All existing functionality preserved:

-   ✅ Keypad test mode works (toggles LEDs)
-   ✅ cellIndex() calculations correct
-   ✅ ledControl() mapping correct
-   ✅ Physical wiring hidden from user code
-   ✅ No compilation errors
-   ✅ Backward compatibility maintained

## Future Enhancements

Possible additions to MatrixPanel:

-   Animation helpers (sweep, blink, fade)
-   Pattern rendering (shapes, text)
-   State management (snapshot/restore)
-   Event callbacks (on cell change)
