/**
 * @file matrixpanel.h
 * @brief Keypad + LED Matrix abstraction layer
 *
 * Provides high-level interface for 4x4 keypad matrix with integrated LEDs.
 * Handles the logical-to-physical mapping of keys and LEDs, hiding wiring complexity.
 */

#ifndef MATRIXPANEL_H
#define MATRIXPANEL_H

#include "msk.h"
#include "pixel.h"
#include "ioexpander.h"

/**
 * @class MatrixPanel
 * @brief High-level abstraction for keypad+LED matrix
 *
 * Provides logical grid-based interface (x,y coordinates and index 0-15)
 * that hides the physical wiring of keys to LEDs.
 */
class MatrixPanel
{
public:
        /**
         * @brief Construct a new Matrix Panel object
         * @param pixels Pointer to PixelStrip for LED control
         */
        explicit MatrixPanel(PixelStrip *pixels);

        /**
         * @brief Convert column (x) and row (y) to logical cell index
         * @param x Column index (0-3)
         * @param y Row index (0-3)
         * @return Logical cell index (0-15), or 0xFF if out of bounds
         */
        u8 cellIndex(u8 x, u8 y) const;

        /**
         * @brief Set LED color by logical index (hides physical wiring)
         * @param logicalIndex Logical cell index (0-15)
         * @param color 32-bit RGB color value (0x00RRGGBB)
         */
        void ledControl(u8 logicalIndex, u32 color);

        /**
         * @brief Set LED color by logical index with separate RGB values
         * @param logicalIndex Logical cell index (0-15)
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         */
        void ledControl(u8 logicalIndex, u8 r, u8 g, u8 b);

        /**
         * @brief Set LED color by grid coordinates
         * @param x Column index (0-3)
         * @param y Row index (0-3)
         * @param color 32-bit RGB color value (0x00RRGGBB)
         */
        void setCell(u8 x, u8 y, u32 color);

        /**
         * @brief Set LED color by grid coordinates with RGB values
         * @param x Column index (0-3)
         * @param y Row index (0-3)
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         */
        void setCell(u8 x, u8 y, u8 r, u8 g, u8 b);

        /**
         * @brief Turn off all LEDs in the matrix
         */
        void clear();

        /**
         * @brief Set all LEDs to the same color
         * @param color 32-bit RGB color value (0x00RRGGBB)
         */
        void fill(u32 color);

        /**
         * @brief Set all LEDs to the same RGB color
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         */
        void fill(u8 r, u8 g, u8 b);

        /**
         * @brief Get the number of rows in the matrix
         * @return Number of rows (4)
         */
        constexpr u8 getRows() const { return KEYPAD_ROWS; }

        /**
         * @brief Get the number of columns in the matrix
         * @return Number of columns (4)
         */
        constexpr u8 getCols() const { return KEYPAD_COLS; }

        /**
         * @brief Get the total number of cells in the matrix
         * @return Total cells (16)
         */
        constexpr u8 getSize() const { return KEYPAD_SIZE; }

private:
        PixelStrip *m_pixels; ///< Pointer to LED strip controller

        /**
         * Key to LED mapping (16 keys -> 16 LEDs)
         * Maps logical key index to physical LED index based on hardware wiring
         * K0→L0, K1→L7, K2→L8, K3→L15, K4→L1, K5→L6, K6→L9, K7→L14,
         * K8→L2, K9→L5, K10→L10, K11→L13, K12→L3, K13→L4, K14→L11, K15→L12
         */
        static const u8 kKeyToLedMap[KEYPAD_SIZE];
};

#endif // MATRIXPANEL_H
