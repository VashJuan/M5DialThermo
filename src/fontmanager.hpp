/**
 * @file fontmanager.hpp
 * @brief Font Display Manager Header
 * @version 1.0
 * @date 2025-12-16
 *
 * @Hardwares: M5Dial
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#pragma once

#include <Arduino.h>
#include "M5GFX.h" // For lgfx font types

/**
 * @interface DeviceInterface
 * @brief Abstract interface for device-specific display operations
 *
 * This interface allows the FontDisplayManager to work with different devices
 * without being tightly coupled to any specific hardware implementation.
 */
class DeviceInterface
{
public:
    virtual ~DeviceInterface() = default;

    /**
     * @brief Clear the display
     */
    virtual void clearDisplay() = 0;

    /**
     * @brief Get display width
     * @return Display width in pixels
     */
    virtual int getDisplayWidth() const = 0;

    /**
     * @brief Get display height
     * @return Display height in pixels
     */
    virtual int getDisplayHeight() const = 0;

    /**
     * @brief Display font information and sample text
     * @param familyName Font family name
     * @param fontName Font name
     * @param fontSize Font size
     * @param fontPtr Pointer to the font object
     * @param sampleText Sample text to display
     */
    virtual void displayFont(const String &familyName, const String &fontName,
                             int fontSize, const lgfx::IFont *fontPtr, const char *sampleText) = 0;
};

// Arduino-compatible font definitions with font pointer
struct FontInfo
{
    const char *family;
    const char *name;
    int size;
    const lgfx::IFont *fontPtr; // Pointer to actual font object
};

// Define font families - simplified for Arduino compatibility
extern const FontInfo fontFamilies[][20];
extern const int NUM_FONT_FAMILIES;

/**
 * @class FontDisplayManager
 * @brief Manages font family display based on encoder position
 *
 * This class allows cycling through different font families and displaying
 * sample text using fonts from the selected family based on encoder position.
 */
class FontDisplayManager
{
private:
    int currentFamilyIndex;  // Currently selected font family
    int currentFontIndex;    // Currently selected font within family
    int lastEncoderPosition; // Last recorded encoder position
    const char *sampleText;  // Sample text to display
    bool displayChanged;     // Flag to track if display needs update
    DeviceInterface *device; // Pointer to device-specific implementation

    int getFontsInFamily(int familyIndex) const;
    String getFamilyName(int familyIndex) const;
    String getFontName(int familyIndex, int fontIndex) const;
    void mapEncoderToFont(long encoderPosition);

public:
    /**
     * @brief Constructor
     * @param deviceInterface Pointer to device-specific implementation
     */
    FontDisplayManager(DeviceInterface *deviceInterface = nullptr);

    /**
     * @brief Set device interface
     * @param deviceInterface Pointer to device-specific implementation
     */
    void setDevice(DeviceInterface *deviceInterface);

    /**
     * @brief Set sample text to display
     * @param text Text to display with fonts
     */
    void setSampleText(const char *text);

    /**
     * @brief Update display based on encoder position
     * @param encoderPosition Current encoder position
     */
    void update(long encoderPosition);

    /**
     * @brief Display current font with sample text
     */
    void displayCurrentFont();

    /**
     * @brief Get current font family name
     * @return Current font family name
     */
    String getCurrentFamilyName() const;

    /**
     * @brief Get current font name
     * @return Current font name
     */
    String getCurrentFontName() const;

    /**
     * @brief Get total number of font families
     * @return Number of font families
     */
    int getTotalFamilies() const;

    /**
     * @brief Get font size of current font
     * @return Font size
     */
    int getCurrentFontSize() const;

    /**
     * @brief Force display update
     */
    void forceUpdate();

    /**
     * @brief Get font pointer for current font
     * @return Pointer to current font object
     */
    const lgfx::IFont *getCurrentFontPtr() const;
};

// Global instance declaration
extern FontDisplayManager fontManager;