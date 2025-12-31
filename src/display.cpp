/**
 * @file display.cpp
 * @brief Abstracted Display Implementation
 * @version 1.0
 * @date 2025-12-28
 */

#include "display.hpp"
#include "fontmanager.hpp"

// Global instance for easy access
Display display;

// Display layout constants
// See https://docs.m5stack.com/en/arduino/m5gfx/m5gfx_appendix for options
static const uint32_t BACKGROUND_COLOR = 0xFFB040;
static const uint32_t CLEAR_COLOR = 0xBED500;

Display::Display() : centerX(120), centerY(120), width(240), height(240), 
                     backgroundColor(BACKGROUND_COLOR),
                     titleY(40), timeY(60), tempY(90), stoveY(120), statusY(190)
{
    initializeAreaConfigs();
}

Display::~Display()
{
    // Cleanup if needed
}

void Display::initializeAreaConfigs()
{
    // Initialize default configurations for each area
    areaConfigs[TITLE] = AreaConfig(&fonts::FreeSans12pt7b, 1, COLOR_BLACK, COLOR_YELLOW);
    areaConfigs[TIME] = AreaConfig(&fonts::FreeSans12pt7b, 1, COLOR_BLACK, COLOR_GREEN);
    areaConfigs[TEMP] = AreaConfig(nullptr, 2, COLOR_RED, COLOR_YELLOW);
    areaConfigs[STOVE] = AreaConfig(nullptr, 2, COLOR_BLUE, COLOR_YELLOW);
    areaConfigs[STATUS_AREA] = AreaConfig(nullptr, 1, COLOR_BLACK, COLOR_YELLOW);
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
    
    // Use the title area configuration for splash screen
    const AreaConfig& titleConfig = getAreaConfig(TITLE);
    
    if (titleConfig.font != nullptr) {
        M5.Display.setFont(titleConfig.font);
        M5.Display.setTextSize(1);
    } else {
        M5.Display.setFont(&fonts::Font2);
        M5.Display.setTextSize(titleConfig.textSize);
    }
    M5.Display.setTextColor(getColorValue(titleConfig.textColor));
    
    M5.Display.drawCenterString("M5Dial Thermostat v 2.0.0", centerX, titleY);
    drawHorizontalLine(20, width - 20, titleY + 15, COLOR_BLUE);
    
    // Reset to default font
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK);
    
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

const AreaConfig& Display::getAreaConfig(DisplayArea area) const
{
    return areaConfigs[area];
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
    const AreaConfig& config = getAreaConfig(area);
    int clearHeight;
    
    // Different heights for different font types
    if (config.font != nullptr) {
        clearHeight = 20; // GFX fonts height
    } else {
        clearHeight = 16 * config.textSize; // Approximate height based on text size
    }
    
    // Use area-specific background color
    uint32_t clearColor = getColorValue(config.backgroundColor);
    
    M5.Display.fillRect(0, y - 2, width, clearHeight + 4, clearColor);
}

void Display::showText(DisplayArea area, const String& text, TextColor color, bool clearFirst)
{
    if (clearFirst) {
        clearArea(area);
    }
    
    const AreaConfig& config = getAreaConfig(area);
    
    // Use provided color or area default
    TextColor actualTextColor = (color == COLOR_BLACK && config.textColor != COLOR_BLACK) ? config.textColor : color;
    M5.Display.setTextColor(getColorValue(actualTextColor));
    
    // Set font for the area
    if (config.font != nullptr) {
        M5.Display.setFont(config.font);
        M5.Display.setTextSize(1); // GFX fonts use size 1
    } else {
        M5.Display.setFont(&fonts::Font2); // Default font
        M5.Display.setTextSize(config.textSize);
    }

    // if  text (for the STATUS_AREA or STOVE area) is too long, break it into multiple lines
    if (area == STATUS_AREA || area == STOVE) {
        drawMultiLineText(text, centerX, getAreaY(area), config.font != nullptr ? 1 : config.textSize);
    } else {
        M5.Display.drawCenterString(text, centerX, getAreaY(area));
    }
    
    // Reset to default font, color and size
    M5.Display.setFont(&fonts::Font2);
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

void Display::drawMultiLineText(const String& text, int centerX, int startY, int textSize)
{
    M5.Display.setTextSize(textSize);
    
    // Calculate approximate character width for line breaking
    int charWidth = 6 * textSize;  // Rough estimate for default font
    int radius = getWidth() / 2;  // Assume circular display
    int centerY = getHeight() / 2;
    
    String remainingText = text;
    int currentY = startY;
    int lineHeight = 8 * textSize + 2; // Text height plus small spacing
    
    while (remainingText.length() > 0) {
        // Calculate available width at current Y position for round display
        int distanceFromCenter = abs(currentY - centerY);
        int availableWidth = getWidth();
        
        // Apply circular constraint - reduce width as we move away from center
        if (distanceFromCenter < radius) {
            // Use Pythagorean theorem to find available width at this Y position
            float availableRadius = sqrt(radius * radius - distanceFromCenter * distanceFromCenter);
            availableWidth = (int)(2 * availableRadius * 0.9); // 90% to add margin
        }
        
        int maxCharsPerLine = availableWidth / charWidth;
        maxCharsPerLine = max(maxCharsPerLine, 8); // Minimum 8 characters per line
        
        String line;
        
        if (remainingText.length() <= maxCharsPerLine) {
            // Remaining text fits on one line
            line = remainingText;
            remainingText = "";
        } else {
            // Find the best break point (space or punctuation)
            int breakPoint = maxCharsPerLine;
            
            // Look for space, comma, or other natural break points
            for (int i = maxCharsPerLine - 1; i > maxCharsPerLine * 0.7; i--) {
                if (remainingText.charAt(i) == ' ' || 
                    remainingText.charAt(i) == ',' || 
                    remainingText.charAt(i) == ':' ||
                    remainingText.charAt(i) == '(' ||
                    remainingText.charAt(i) == ')') {
                    breakPoint = i;
                    break;
                }
            }
            
            line = remainingText.substring(0, breakPoint);
            remainingText = remainingText.substring(breakPoint);
            
            // Remove leading space from next line
            remainingText.trim();
        }
        
        // Draw the line centered
        M5.Display.drawCenterString(line, centerX, currentY);
        currentY += lineHeight;
        
        // Prevent infinite loop and screen overflow
        if (currentY > getHeight() - lineHeight) {
            break;
        }
    }
}

void Display::setAreaConfig(DisplayArea area, const lgfx::IFont* font, int textSize, TextColor textColor, TextColor backgroundColor)
{
    areaConfigs[area] = AreaConfig(font, textSize, textColor, backgroundColor);
}

void Display::setAreaFont(DisplayArea area, const lgfx::IFont* font, int textSize)
{
    areaConfigs[area].font = font;
    areaConfigs[area].textSize = textSize;
}

void Display::setAreaColors(DisplayArea area, TextColor textColor, TextColor backgroundColor)
{
    areaConfigs[area].textColor = textColor;
    areaConfigs[area].backgroundColor = backgroundColor;
}

AreaConfig Display::getAreaConfiguration(DisplayArea area) const
{
    return areaConfigs[area];
}
