/**
 * @file display.hpp
 * @brief Abstracted Display Class Header File
 * @version 1.0
 * @date 2025-12-28
 *
 * @Hardware: M5Dial (abstracted for future displays)
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#pragma once

#include <Arduino.h>
#include <M5Unified.h>

/**
 * @enum DisplayArea
 * @brief Enumeration for different display areas
 */
enum DisplayArea
{
    TITLE = 0,
    TIME = 1,
    TEMP = 2,
    STOVE = 3,
    STATUS_AREA = 4
};

/**
 * @enum TextColor
 * @brief Text color options
 */
enum TextColor
{
    COLOR_BLACK = 0,
    COLOR_RED = 1,
    COLOR_WHITE = 2,
    COLOR_BLUE = 3
};

/**
 * @class Display
 * @brief Abstracted display class for screen management
 *
 * This class provides functionality to:
 * 1. Abstract display operations for different hardware
 * 2. Manage different screen areas (title, time, temp, stove, status)
 * 3. Handle text clearing and drawing operations
 * 4. Provide consistent interface across different display types
 */
class Display
{
private:
    int centerX;
    int centerY;
    int width;
    int height;
    uint32_t backgroundColor;

    // Area-specific Y coordinates
    int titleY;
    int timeY;
    int tempY;
    int stoveY;
    int statusY;

    /**
     * @brief Get Y coordinate for specified display area
     * @param area Display area
     * @return Y coordinate
     */
    int getAreaY(DisplayArea area);

    /**
     * @brief Get text size for specified display area
     * @param area Display area
     * @return Text size multiplier
     */
    int getAreaTextSize(DisplayArea area);

    /**
     * @brief Convert TextColor enum to actual color value
     * @param color TextColor enum value
     * @return Actual color value for the display
     */
    uint32_t getColorValue(TextColor color);

    /**
     * @brief Clear specific area of the screen
     * @param area Display area to clear
     */
    void clearArea(DisplayArea area);

public:
    /**
     * @brief Constructor
     */
    Display();

    /**
     * @brief Destructor
     */
    ~Display();

    /**
     * @brief Initialize display system
     */
    void setup();

    /**
     * @brief Clear entire screen and show splash screen
     */
    void showSplashScreen();

    /**
     * @brief Display text in specified area
     * @param area Display area (TITLE, TIME, TEMP, STOVE, STATUS)
     * @param text Text to display
     * @param color Text color (optional, defaults to black)
     * @param clearFirst Whether to clear the area first (optional, defaults to true)
     */
    void showText(DisplayArea area, const String &text, TextColor color = COLOR_BLACK, bool clearFirst = true);

    /**
     * @brief Clear entire screen
     */
    void clear();

    /**
     * @brief Draw a horizontal line
     * @param x1 Start X coordinate
     * @param x2 End X coordinate
     * @param y Y coordinate
     * @param color Line color
     */
    void drawHorizontalLine(int x1, int x2, int y, TextColor color = COLOR_BLUE);

    /**
     * @brief Get display width
     * @return Display width in pixels
     */
    int getWidth() const;

    /**
     * @brief Get display height
     * @return Display height in pixels
     */
    int getHeight() const;

    /**
     * @brief Get center X coordinate
     * @return Center X coordinate
     */
    int getCenterX() const;

    /**
     * @brief Get center Y coordinate
     * @return Center Y coordinate
     */
    int getCenterY() const;
};

// Global instance for easy access
extern Display display;