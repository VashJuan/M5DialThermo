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
static const uint32_t CLEAR_COLOR = TFT_RED; // was 0xBED500

Display::Display() : centerX(120), centerY(120), width(240), height(240),
                     backgroundColor(BACKGROUND_COLOR),
                     titleY(40), timeY(60), tempY(80), stoveY(100), statusY(160)
{
    // Font initialization moved to setup() to avoid crashes before M5.begin()
}

Display::~Display()
{
    // Cleanup if needed
}

void Display::initializeAreaConfigs()
{
    // Nice fonts: Satisfy_24, DejaVu18, Font2 (tiny!), FreeSans9pt7b, FreeSansOblique12pt7b, FreeSerif12pt7b, Orbitron_Light_24, Roboto_Thin_24
    // Default Area configurations: font, text size, text color, background color
    areaConfigs[TITLE] = AreaConfig(&fonts::Font2, 1, TFT_BLACK, TFT_YELLOW);
    areaConfigs[TIME] = AreaConfig(&fonts::Font2, 1, TFT_BLACK, TFT_YELLOW);
    areaConfigs[TEMP] = AreaConfig(&fonts::DejaVu18, 2, TFT_BLACK, TFT_YELLOW);
    areaConfigs[STOVE] = AreaConfig(&fonts::DejaVu18, 2, TFT_BLACK, TFT_YELLOW);
    areaConfigs[STATUS_AREA] = AreaConfig(&fonts::Font2, 1, TFT_BLACK, TFT_YELLOW);
}

void Display::setup()
{
    // Initialize area configs after M5 is initialized
    initializeAreaConfigs();

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
    const AreaConfig titleConfig = getAreaConfiguration(TITLE);

    M5.Display.setFont(titleConfig.font);
    M5.Display.setTextSize(titleConfig.textSize);
    M5.Display.setTextColor(TFT_BLACK);

    M5.Display.drawCenterString("M5Dial Thermostat v 2.0.0", centerX, titleY);
    M5.Display.drawLine(30, titleY + 15, width - 30, titleY + 15, TFT_BLUE);

    // NEEDED? Set default font
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK);

    delay(50);
}

int Display::getAreaY(DisplayArea area)
{
    switch (area)
    {
    case TITLE:
        return titleY;
    case TIME:
        return timeY;
    case TEMP:
        return tempY;
    case STOVE:
        return stoveY;
    case STATUS_AREA:
        return statusY;
    default:
        return statusY;
    }
}

AreaConfig Display::getAreaConfiguration(DisplayArea area) const
{
    return areaConfigs[area];
}

void Display::clearArea(DisplayArea area)
{
    int y = getAreaY(area);
    const AreaConfig config = getAreaConfiguration(area);
    int clearHeight;

    // Different heights for different font types
    if (config.font != nullptr)
    {
        clearHeight = 20; // GFX fonts height
    }
    else
    {
        clearHeight = 16 * config.textSize; // Approximate height based on text size
    }

    // Use area-specific background color from config (already a TFT_* constant)
    M5.Display.fillRect(0, y - 2, width, clearHeight + 4, config.backgroundColor);
}

void Display::showText(DisplayArea area, const String &text, uint32_t color, bool clearFirst)
{
    if (clearFirst)
    {
        clearArea(area);
    }

    const AreaConfig config = getAreaConfiguration(area);

    // Use provided color or area default (both are already TFT_* constants)
    uint32_t actualColor = (color == TFT_BLACK && config.textColor != TFT_BLACK) ? config.textColor : color;
    M5.Display.setTextColor(actualColor);

    // Set font for the area
    if (config.font != nullptr)
    {
        M5.Display.setFont(config.font);
        M5.Display.setTextSize(1); // GFX fonts use size 1
    }
    else
    {
        M5.Display.setFont(&fonts::Font2); // Default font
        M5.Display.setTextSize(config.textSize);
    }

    // if  text (for the STATUS_AREA or STOVE area) is too long, break it into multiple lines
    if (area == STATUS_AREA || area == STOVE)
    {
        drawMultiLineText(text, centerX, getAreaY(area), config.font != nullptr ? 1 : config.textSize);
    }
    else
    {
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

void Display::drawHorizontalLine(int x1, int x2, int y, uint32_t color)
{
    M5.Display.drawLine(x1, y, x2, y, color);
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

void Display::drawMultiLineText(const String &text, int centerX, int startY, int textSize)
{
    M5.Display.setTextSize(textSize);

    // Calculate approximate character width for line breaking
    int charWidth = 6 * textSize; // Rough estimate for default font
    int radius = getWidth() / 2;  // Assume circular display
    int centerY = getHeight() / 2;

    String remainingText = text;
    int currentY = startY;
    int lineHeight = 8 * textSize + 2; // Text height plus small spacing

    while (remainingText.length() > 0)
    {
        // Calculate available width at current Y position for round display
        int distanceFromCenter = abs(currentY - centerY);
        int availableWidth = getWidth();

        // Apply circular constraint - reduce width as we move away from center
        if (distanceFromCenter < radius)
        {
            // Use Pythagorean theorem to find available width at this Y position
            float availableRadius = sqrt(radius * radius - distanceFromCenter * distanceFromCenter);
            availableWidth = (int)(2 * availableRadius * 0.9); // 90% to add margin
        }

        int maxCharsPerLine = availableWidth / charWidth;
        maxCharsPerLine = max(maxCharsPerLine, 8); // Minimum 8 characters per line

        String line;

        if (remainingText.length() <= maxCharsPerLine)
        {
            // Remaining text fits on one line
            line = remainingText;
            remainingText = "";
        }
        else
        {
            // Find the best break point (space or punctuation)
            int breakPoint = maxCharsPerLine;

            // Look for space, comma, or other natural break points
            for (int i = maxCharsPerLine - 1; i > maxCharsPerLine * 0.7; i--)
            {
                if (remainingText.charAt(i) == ' ' ||
                    remainingText.charAt(i) == ',' ||
                    remainingText.charAt(i) == ':' ||
                    remainingText.charAt(i) == '(' ||
                    remainingText.charAt(i) == ')')
                {
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
        if (currentY > getHeight() - lineHeight)
        {
            break;
        }
    }
}

void Display::setAreaConfig(DisplayArea area, const lgfx::IFont *font, int textSize, uint32_t textColor, uint32_t backgroundColor)
{
    areaConfigs[area] = AreaConfig(font, textSize, textColor, backgroundColor);
}

void Display::setAreaFont(DisplayArea area, const lgfx::IFont *font, int textSize)
{
    areaConfigs[area].font = font;
    areaConfigs[area].textSize = textSize;
}

void Display::setAreaColors(DisplayArea area, uint32_t textColor, uint32_t backgroundColor)
{
    areaConfigs[area].textColor = textColor;
    areaConfigs[area].backgroundColor = backgroundColor;
}
