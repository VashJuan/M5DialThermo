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
 * @struct AreaConfig
 * @brief Configuration for a display area including font, colors, and text size
 */
struct AreaConfig
{
    const lgfx::IFont *font;  // Font pointer (nullptr for default font)
    int textSize;             // Text size multiplier for non-GFX fonts
    uint32_t textColor;       // Text/foreground color (TFT_* constant)
    uint32_t backgroundColor; // Background color (TFT_* constant)

    // Default constructor
    AreaConfig() : font(nullptr), textSize(1), textColor(TFT_BLACK), backgroundColor(TFT_WHITE) {}

    // Parameterized constructor
    AreaConfig(const lgfx::IFont *f, int size, uint32_t fg, uint32_t bg)
        : font(f), textSize(size), textColor(fg), backgroundColor(bg) {}
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

    // Area configurations
    AreaConfig areaConfigs[5]; // One for each DisplayArea enum value

    /**
     * @brief Initialize default area configurations
     */
    void initializeAreaConfigs();

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
     * @brief Get font pointer for specified display area
     * @param area Display area
     * @return Font pointer (nullptr for default font)
     */
    const lgfx::IFont *getAreaFont(DisplayArea area);

    /**
     * @brief Clear specific area of the screen
     * @param area Display area to clear
     */
    void clearArea(DisplayArea area);

    /**
     * @brief Draw multi-line text centered on screen
     * @param text Text to display (will be broken into lines)
     * @param centerX X coordinate for centering
     * @param startY Y coordinate to start drawing
     * @param textSize Text size multiplier
     */
    void drawMultiLineText(const String &text, int centerX, int startY, int textSize);

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
     * @param color Text color (optional, uses area config if not specified)
     * @param clearFirst Whether to clear the area first (optional, defaults to true)
     */
    void showText(DisplayArea area, const String &text, uint32_t color = TFT_BLACK, bool clearFirst = true);

    /**
     * @brief Set configuration for a specific display area
     * @param area Display area to configure
     * @param font Font pointer (nullptr for default font)
     * @param textSize Text size multiplier
     * @param textColor Text/foreground color
     * @param backgroundColor Background color
     */
    void setAreaConfig(DisplayArea area, const lgfx::IFont *font, int textSize, uint32_t textColor, uint32_t backgroundColor);

    /**
     * @brief Set font for a specific display area
     * @param area Display area
     * @param font Font pointer (nullptr for default font)
     * @param textSize Text size multiplier (for non-GFX fonts)
     */
    void setAreaFont(DisplayArea area, const lgfx::IFont *font, int textSize = 1);

    /**
     * @brief Set colors for a specific display area
     * @param area Display area
     * @param textColor Text/foreground color
     * @param backgroundColor Background color
     */
    void setAreaColors(DisplayArea area, uint32_t textColor, uint32_t backgroundColor);

    /**
     * @brief Get area configuration
     * @param area Display area
     * @return Copy of area configuration
     */
    AreaConfig getAreaConfiguration(DisplayArea area) const;

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
    void drawHorizontalLine(int x1, int x2, int y, uint32_t color = TFT_BLUE);

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