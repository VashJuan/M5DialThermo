/**
 * @file display.cpp
 * @brief Abstracted Display Implementation
 * @version 1.0
 * @date 2025-12-28
 */

#include "display.hpp"

// Global instance for easy access
Display display;

// Display layout constants
// See https://docs.m5stack.com/en/arduino/m5gfx/m5gfx_appendix for options
static const uint32_t BACKGROUND_COLOR = 0xFFB040;
static const uint32_t CLEAR_COLOR = 0xBED500;

Display::Display() : centerX(120), centerY(120), width(240), height(240), 
                     backgroundColor(BACKGROUND_COLOR),
                     titleY(40), timeY(60), tempY(90), stoveY(120), statusY(160)
{
}

Display::~Display()
{
    // Cleanup if needed
}

void Display::setup()
{
    // Get actual display dimensions
    width = M5.Display.width() == 0 ? 240 : M5.Display.width();
    height = M5.Display.height() == 0 ? 240 : M5.Display.height();
    centerX = width / 2;
    centerY = height / 2;

    // Initialize display settings
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextDatum(middle_center);
}

void Display::showSplashScreen()
{
    Serial.println("\n\n------------------------------");
    M5.Display.clear();
    M5.Display.fillScreen(backgroundColor);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextSize(1);
    
    M5.Display.drawCenterString("M5Dial Thermostat v 2.0.0", centerX, titleY);
    drawHorizontalLine(20, width - 20, titleY + 15, COLOR_BLUE);
    
    delay(50);
}

int Display::getAreaY(DisplayArea area)
{
    switch (area) {
        case TITLE:  return titleY;
        case TIME:   return timeY;
        case TEMP:   return tempY;
        case STOVE:  return stoveY;
        case STATUS_AREA: return statusY;
        default:     return statusY;
    }
}

int Display::getAreaTextSize(DisplayArea area)
{
    switch (area) {
        case TITLE:  return 1;
        case TIME:   return 1;
        case TEMP:   return 2;  // Temperature should be larger
        case STOVE:  return 2;
        case STATUS_AREA: return 1;
        default:     return 1;
    }
}

// Real options are at: https://docs.m5stack.com/en/arduino/m5gfx/m5gfx_appendix#color
uint32_t Display::getColorValue(TextColor color)
{
    switch (color) {
        case COLOR_BLACK: return TFT_BLACK;
        case COLOR_RED:   return TFT_RED;
        case COLOR_WHITE: return TFT_WHITE;
        case COLOR_BLUE:  return TFT_BLUE;
        case COLOR_GREEN: return TFT_GREEN;
        case COLOR_YELLOW: return TFT_YELLOW;
        case COLOR_CYAN: return TFT_CYAN;
        case COLOR_MAGENTA: return TFT_MAGENTA;
        case COLOR_GRAY: return TFT_LIGHTGREY;
        default:          return TFT_BLACK;
    }
}

void Display::clearArea(DisplayArea area)
{
    int y = getAreaY(area);
    int textSize = getAreaTextSize(area);
    int clearHeight = 16 * textSize; // Approximate height based on text size
    
    // Use different clear color for time area for visual effect
    uint32_t clearColor = (area == TIME) ? CLEAR_COLOR : backgroundColor;
    
    M5.Display.fillRect(0, y - 2, width, clearHeight + 4, clearColor);
}

void Display::showText(DisplayArea area, const String& text, TextColor color, bool clearFirst)
{
    if (clearFirst) {
        clearArea(area);
    }
    
    M5.Display.setTextColor(getColorValue(color));
    M5.Display.setTextSize(getAreaTextSize(area));

    // if  text (for the STATUS_AREA) is too long, allow it to wrap
    if (area == STATUS_AREA) {
        M5.Display.setTextWrap(true);
        M5.Display.drawCenterString(text, centerX, getAreaY(area));
        M5.Display.setTextWrap(false);
    } else {
        M5.Display.drawCenterString(text, centerX, getAreaY(area));
    }
    
    // Reset to default color and size
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setTextSize(1);
}

void Display::clear()
{
    M5.Display.clear();
    M5.Display.fillScreen(backgroundColor);
}

void Display::drawHorizontalLine(int x1, int x2, int y, TextColor color)
{
    M5.Display.drawLine(x1, y, x2, y, getColorValue(color));
}

int Display::getWidth() const
{
    return width;
}

int Display::getHeight() const
{
    return height;
}

int Display::getCenterX() const
{
    return centerX;
}

int Display::getCenterY() const
{
    return centerY;
}
