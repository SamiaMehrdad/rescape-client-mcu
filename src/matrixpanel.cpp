/**
 * @file matrixpanel.cpp
 * @brief Implementation of MatrixPanel class for keypad+LED matrix abstraction
 *
 * This file provides the implementation of a 4x4 keypad+LED matrix controller
 * that abstracts the physical wiring between logical key positions and physical
 * LED indices. This allows application code to work with logical grid coordinates
 * (x, y) or indices (0-15) without needing to know the hardware wiring details.
 *
 * The MatrixPanel class handles:
 * - Logical to physical LED index mapping
 * - Grid coordinate to index conversion
 * - High-level matrix operations (fill, clear, setCell)
 * - Bounds checking and validation
 *
 * Hardware Configuration:
 * - Matrix size: 4x4 (16 cells)
 * - LEDs: WS2812B RGB addressable LEDs
 * - Logical indexing: Row-major order (0-15)
 * - Physical wiring: Custom mapping defined in kKeyToLedMap
 */

#include "matrixpanel.h"

//============================================================================
// STATIC DATA - KEY TO LED MAPPING
//============================================================================

/**
 * Key to LED mapping table (16 keys -> 16 LEDs)
 * Maps logical key index (0-15) to physical LED index based on hardware wiring
 *
 * This table encapsulates the physical wiring between the keypad positions
 * and the LED strip indices, allowing the rest of the code to use logical
 * indices without worrying about the actual hardware connections.
 *
 * Wiring Map:
 * K0→L0,  K1→L7,  K2→L8,   K3→L15
 * K4→L1,  K5→L6,  K6→L9,   K7→L14
 * K8→L2,  K9→L5,  K10→L10, K11→L13
 * K12→L3, K13→L4, K14→L11, K15→L12
 */
const u8 MatrixPanel::kKeyToLedMap[KEYPAD_SIZE] = {
    0,  // K0  -> L0
    7,  // K1  -> L7
    8,  // K2  -> L8
    15, // K3  -> L15
    1,  // K4  -> L1
    6,  // K5  -> L6
    9,  // K6  -> L9
    14, // K7  -> L14
    2,  // K8  -> L2
    5,  // K9  -> L5
    10, // K10 -> L10
    13, // K11 -> L13
    3,  // K12 -> L3
    4,  // K13 -> L4
    11, // K14 -> L11
    12  // K15 -> L12
};

//============================================================================
// CONSTRUCTOR
//============================================================================

/**
 * @brief Construct a new MatrixPanel object
 * @param pixels Pointer to PixelStrip controller for LED operations
 *
 * Initializes the matrix panel with a reference to the LED strip controller.
 * The PixelStrip must be already initialized and remain valid for the lifetime
 * of this MatrixPanel instance.
 */
MatrixPanel::MatrixPanel(PixelStrip *pixels) : m_pixels(pixels)
{
}

//============================================================================
// COORDINATE CONVERSION
//============================================================================

/**
 * @brief Convert grid coordinates to logical cell index
 * @param x Column index (0-3, left to right)
 * @param y Row index (0-3, top to bottom)
 * @return Logical cell index (0-15), or 0xFF if coordinates are out of bounds
 *
 * Converts 2D grid coordinates to a linear index using row-major order:
 * Index = (row * columns) + column
 *
 * Examples:
 * - cellIndex(0, 0) = 0  (top-left)
 * - cellIndex(3, 0) = 3  (top-right)
 * - cellIndex(0, 3) = 12 (bottom-left)
 * - cellIndex(3, 3) = 15 (bottom-right)
 * - cellIndex(2, 1) = 6  (column 2, row 1)
 * - cellIndex(4, 0) = 0xFF (out of bounds)
 */
u8 MatrixPanel::cellIndex(u8 x, u8 y) const
{
        // Validate input bounds
        if (x >= KEYPAD_COLS || y >= KEYPAD_ROWS)
        {
                return 0xFF; // Invalid index
        }

        // Calculate logical index: row * columns + column
        return y * KEYPAD_COLS + x;
}

//============================================================================
// LED CONTROL BY LOGICAL INDEX
//============================================================================

/**
 * @brief Set LED color by logical index using 32-bit color value
 * @param logicalIndex Logical cell index (0-15)
 * @param color 32-bit RGB color in format 0x00RRGGBB
 *
 * Sets the LED at the specified logical index to the given color.
 * This function extracts RGB components from the 32-bit value and
 * delegates to the RGB overload.
 *
 * Examples:
 * - ledControl(0, 0xFF0000) - Set cell 0 to red
 * - ledControl(5, 0x00FF00) - Set cell 5 to green
 * - ledControl(15, 0x0000FF) - Set cell 15 to blue
 */
void MatrixPanel::ledControl(u8 logicalIndex, u32 color)
{
        // Extract RGB components and delegate to RGB version
        u8 r = (color >> 16) & 0xFF;
        u8 g = (color >> 8) & 0xFF;
        u8 b = color & 0xFF;
        ledControl(logicalIndex, r, g, b);
}

/**
 * @brief Set LED color by logical index using separate RGB values
 * @param logicalIndex Logical cell index (0-15)
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 *
 * This is the core LED control function that:
 * 1. Validates the logical index is in range
 * 2. Translates logical index to physical LED index using kKeyToLedMap
 * 3. Validates the physical LED exists in the strip
 * 4. Sets the physical LED to the specified color
 *
 * The physical wiring is completely hidden from the caller, who only
 * needs to know the logical cell index (0-15).
 *
 * Examples:
 * - ledControl(0, 255, 0, 0) - Set cell 0 to full red
 * - ledControl(5, 0, 255, 0) - Set cell 5 to full green
 * - ledControl(15, 128, 128, 255) - Set cell 15 to light blue
 */
void MatrixPanel::ledControl(u8 logicalIndex, u8 r, u8 g, u8 b)
{
        // Validate logical index
        if (logicalIndex >= KEYPAD_SIZE)
        {
                return;
        }

        // Map logical index to physical LED index using the wiring table
        u8 physicalLedIndex = kKeyToLedMap[logicalIndex];

        // Check if physical LED exists
        if (physicalLedIndex >= m_pixels->getCount())
        {
                return;
        }

        // Set the physical LED color
        m_pixels->setColor(physicalLedIndex, r, g, b);
}

//============================================================================
// LED CONTROL BY GRID COORDINATES
//============================================================================

/**
 * @brief Set LED color by grid coordinates using 32-bit color value
 * @param x Column index (0-3)
 * @param y Row index (0-3)
 * @param color 32-bit RGB color in format 0x00RRGGBB
 *
 * Convenience function that converts grid coordinates to a cell index
 * and sets the LED color. Silently ignores out-of-bounds coordinates.
 *
 * Examples:
 * - setCell(0, 0, 0xFF0000) - Set top-left to red
 * - setCell(3, 3, 0x00FF00) - Set bottom-right to green
 * - setCell(2, 1, 0x0000FF) - Set column 2, row 1 to blue
 */
void MatrixPanel::setCell(u8 x, u8 y, u32 color)
{
        u8 index = cellIndex(x, y);
        if (index != 0xFF)
        {
                ledControl(index, color);
        }
}

/**
 * @brief Set LED color by grid coordinates using separate RGB values
 * @param x Column index (0-3)
 * @param y Row index (0-3)
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 *
 * Convenience function that converts grid coordinates to a cell index
 * and sets the LED color. Silently ignores out-of-bounds coordinates.
 *
 * Examples:
 * - setCell(0, 0, 255, 0, 0) - Set top-left to red
 * - setCell(3, 3, 0, 255, 0) - Set bottom-right to green
 * - setCell(2, 1, 0, 0, 255) - Set column 2, row 1 to blue
 */
void MatrixPanel::setCell(u8 x, u8 y, u8 r, u8 g, u8 b)
{
        u8 index = cellIndex(x, y);
        if (index != 0xFF)
        {
                ledControl(index, r, g, b);
        }
}

//============================================================================
// MATRIX-WIDE OPERATIONS
//============================================================================

/**
 * @brief Turn off all LEDs in the matrix
 *
 * Sets all 16 LEDs to black (0, 0, 0), effectively turning them off.
 * This is useful for resetting the display or clearing between modes.
 *
 * Implementation iterates through all logical indices (0-15) and sets
 * each to black. The physical LED mapping is handled automatically.
 */
void MatrixPanel::clear()
{
        for (u8 i = 0; i < KEYPAD_SIZE; i++)
        {
                ledControl(i, 0, 0, 0);
        }
}

/**
 * @brief Set all LEDs to the same color using 32-bit color value
 * @param color 32-bit RGB color in format 0x00RRGGBB
 *
 * Fills the entire matrix with a single color. Useful for creating
 * solid color backgrounds or indicating system states.
 *
 * Examples:
 * - fill(0xFF0000) - Fill with red
 * - fill(0x00FF00) - Fill with green
 * - fill(0x0000FF) - Fill with blue
 * - fill(0xFFFF00) - Fill with yellow
 * - fill(0x800080) - Fill with purple
 */
void MatrixPanel::fill(u32 color)
{
        for (u8 i = 0; i < KEYPAD_SIZE; i++)
        {
                ledControl(i, color);
        }
}

/**
 * @brief Set all LEDs to the same color using separate RGB values
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 *
 * Fills the entire matrix with a single RGB color. Useful for creating
 * solid color backgrounds or indicating system states.
 *
 * Examples:
 * - fill(255, 0, 0) - Fill with red
 * - fill(0, 255, 0) - Fill with green
 * - fill(0, 0, 255) - Fill with blue
 * - fill(255, 255, 0) - Fill with yellow
 * - fill(128, 0, 128) - Fill with purple
 * - fill(255, 255, 255) - Fill with white (full brightness)
 */
void MatrixPanel::fill(u8 r, u8 g, u8 b)
{
        for (u8 i = 0; i < KEYPAD_SIZE; i++)
        {
                ledControl(i, r, g, b);
        }
}
