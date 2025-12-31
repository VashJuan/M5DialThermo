/**
 * @file fontmanager.cpp
 * @brief Font Display Manager Implementation
 * @version 1.0
 * @date 2025-12-16
 *
 * @Hardwares: M5Dial
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#include "fontmanager.hpp"
#include <M5Unified.h>  // For font definitions

// Define font families with font pointers for easy iteration
const FontInfo fontFamilies[][20] = {
    // Built-in LGFX fonts
    {
        {"lgfx_fonts", "Font0", 0, &fonts::Font0},
        {"lgfx_fonts", "Font2", 2, &fonts::Font2},
        {"lgfx_fonts", "Font4", 4, &fonts::Font4},
        {"lgfx_fonts", "Font6", 6, &fonts::Font6},
        {"lgfx_fonts", "Font7", 7, &fonts::Font7},
        {"lgfx_fonts", "Font8", 8, &fonts::Font8},
        {"lgfx_fonts", "TomThumb", 0, &fonts::TomThumb},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // Free Mono family
    {
        {"Free Mono", "FreeMono9pt7b", 9, &fonts::FreeMono9pt7b},
        {"Free Mono", "FreeMono12pt7b", 12, &fonts::FreeMono12pt7b},
        {"Free Mono", "FreeMono18pt7b", 18, &fonts::FreeMono18pt7b},
        {"Free Mono", "FreeMono24pt7b", 24, &fonts::FreeMono24pt7b},
        {"Free Mono", "FreeMonoBold9pt7b", 9, &fonts::FreeMonoBold9pt7b},
        {"Free Mono", "FreeMonoBold12pt7b", 12, &fonts::FreeMonoBold12pt7b},
        {"Free Mono", "FreeMonoBold18pt7b", 18, &fonts::FreeMonoBold18pt7b},
        {"Free Mono", "FreeMonoBold24pt7b", 24, &fonts::FreeMonoBold24pt7b},
        {"Free Mono", "FreeMonoOblique9pt7b", 9, &fonts::FreeMonoOblique9pt7b},
        {"Free Mono", "FreeMonoOblique12pt7b", 12, &fonts::FreeMonoOblique12pt7b},
        {"Free Mono", "FreeMonoOblique18pt7b", 18, &fonts::FreeMonoOblique18pt7b},
        {"Free Mono", "FreeMonoOblique24pt7b", 24, &fonts::FreeMonoOblique24pt7b},
        {"Free Mono", "FreeMonoBoldOblique9pt7b", 9, &fonts::FreeMonoBoldOblique9pt7b},
        {"Free Mono", "FreeMonoBoldOblique12pt7b", 12, &fonts::FreeMonoBoldOblique12pt7b},
        {"Free Mono", "FreeMonoBoldOblique18pt7b", 18, &fonts::FreeMonoBoldOblique18pt7b},
        {"Free Mono", "FreeMonoBoldOblique24pt7b", 24, &fonts::FreeMonoBoldOblique24pt7b},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // Free Sans family
    {
        {"Free Sans", "FreeSans9pt7b", 9, &fonts::FreeSans9pt7b},
        {"Free Sans", "FreeSans12pt7b", 12, &fonts::FreeSans12pt7b},
        {"Free Sans", "FreeSans18pt7b", 18, &fonts::FreeSans18pt7b},
        {"Free Sans", "FreeSans24pt7b", 24, &fonts::FreeSans24pt7b},
        {"Free Sans", "FreeSansBold9pt7b", 9, &fonts::FreeSansBold9pt7b},
        {"Free Sans", "FreeSansBold12pt7b", 12, &fonts::FreeSansBold12pt7b},
        {"Free Sans", "FreeSansBold18pt7b", 18, &fonts::FreeSansBold18pt7b},
        {"Free Sans", "FreeSansBold24pt7b", 24, &fonts::FreeSansBold24pt7b},
        {"Free Sans", "FreeSansOblique9pt7b", 9, &fonts::FreeSansOblique9pt7b},
        {"Free Sans", "FreeSansOblique12pt7b", 12, &fonts::FreeSansOblique12pt7b},
        {"Free Sans", "FreeSansOblique18pt7b", 18, &fonts::FreeSansOblique18pt7b},
        {"Free Sans", "FreeSansOblique24pt7b", 24, &fonts::FreeSansOblique24pt7b},
        {"Free Sans", "FreeSansBoldOblique9pt7b", 9, &fonts::FreeSansBoldOblique9pt7b},
        {"Free Sans", "FreeSansBoldOblique12pt7b", 12, &fonts::FreeSansBoldOblique12pt7b},
        {"Free Sans", "FreeSansBoldOblique18pt7b", 18, &fonts::FreeSansBoldOblique18pt7b},
        {"Free Sans", "FreeSansBoldOblique24pt7b", 24, &fonts::FreeSansBoldOblique24pt7b},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // Free Serif family
    {
        {"Free Serif", "FreeSerif9pt7b", 9, &fonts::FreeSerif9pt7b},
        {"Free Serif", "FreeSerif12pt7b", 12, &fonts::FreeSerif12pt7b},
        {"Free Serif", "FreeSerif18pt7b", 18, &fonts::FreeSerif18pt7b},
        {"Free Serif", "FreeSerif24pt7b", 24, &fonts::FreeSerif24pt7b},
        {"Free Serif", "FreeSerifItalic9pt7b", 9, &fonts::FreeSerifItalic9pt7b},
        {"Free Serif", "FreeSerifItalic12pt7b", 12, &fonts::FreeSerifItalic12pt7b},
        {"Free Serif", "FreeSerifItalic18pt7b", 18, &fonts::FreeSerifItalic18pt7b},
        {"Free Serif", "FreeSerifItalic24pt7b", 24, &fonts::FreeSerifItalic24pt7b},
        {"Free Serif", "FreeSerifBold9pt7b", 9, &fonts::FreeSerifBold9pt7b},
        {"Free Serif", "FreeSerifBold12pt7b", 12, &fonts::FreeSerifBold12pt7b},
        {"Free Serif", "FreeSerifBold18pt7b", 18, &fonts::FreeSerifBold18pt7b},
        {"Free Serif", "FreeSerifBold24pt7b", 24, &fonts::FreeSerifBold24pt7b},
        {"Free Serif", "FreeSerifBoldItalic9pt7b", 9, &fonts::FreeSerifBoldItalic9pt7b},
        {"Free Serif", "FreeSerifBoldItalic12pt7b", 12, &fonts::FreeSerifBoldItalic12pt7b},
        {"Free Serif", "FreeSerifBoldItalic18pt7b", 18, &fonts::FreeSerifBoldItalic18pt7b},
        {"Free Serif", "FreeSerifBoldItalic24pt7b", 24, &fonts::FreeSerifBoldItalic24pt7b},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // Orbitron family
    {
        {"Orbitron", "Orbitron_Light_24", 24, &fonts::Orbitron_Light_24},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // Roboto and other decorative fonts
    {
        {"Roboto", "Roboto_Thin_24", 24, &fonts::Roboto_Thin_24},
        {"Satisfy", "Satisfy_24", 24, &fonts::Satisfy_24},
        {"Yellowtail", "Yellowtail_32", 32, &fonts::Yellowtail_32},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // DejaVu family
    {
        {"DejaVu", "DejaVu9", 9, &fonts::DejaVu9},
        {"DejaVu", "DejaVu12", 12, &fonts::DejaVu12},
        {"DejaVu", "DejaVu18", 18, &fonts::DejaVu18},
        {"DejaVu", "DejaVu24", 24, &fonts::DejaVu24},
        {"DejaVu", "DejaVu40", 40, &fonts::DejaVu40},
        {"DejaVu", "DejaVu56", 56, &fonts::DejaVu56},
        {"DejaVu", "DejaVu72", 72, &fonts::DejaVu72},
        {nullptr, nullptr, 0, nullptr} // End marker
    }
    /** remove these to save memory space!
    ,
    // Japanese Mincho family
    {
        {"JapanMincho", "lgfxJapanMincho_8", 8, &fonts::lgfxJapanMincho_8},
        {"JapanMincho", "lgfxJapanMincho_12", 12, &fonts::lgfxJapanMincho_12},
        {"JapanMincho", "lgfxJapanMincho_16", 16, &fonts::lgfxJapanMincho_16},
        {"JapanMincho", "lgfxJapanMincho_20", 20, &fonts::lgfxJapanMincho_20},
        {"JapanMincho", "lgfxJapanMincho_24", 24, &fonts::lgfxJapanMincho_24},
        {"JapanMincho", "lgfxJapanMinchoP_8", 8, &fonts::lgfxJapanMinchoP_8},
        {"JapanMincho", "lgfxJapanMinchoP_12", 12, &fonts::lgfxJapanMinchoP_12},
        {"JapanMincho", "lgfxJapanMinchoP_16", 16, &fonts::lgfxJapanMinchoP_16},
        {"JapanMincho", "lgfxJapanMinchoP_20", 20, &fonts::lgfxJapanMinchoP_20},
        {"JapanMincho", "lgfxJapanMinchoP_24", 24, &fonts::lgfxJapanMinchoP_24},
        {nullptr, nullptr, 0, nullptr} // End marker
    }
    // Japanese Gothic family
    {
        {"JapanGothic", "lgfxJapanGothic_8", 8, &fonts::lgfxJapanGothic_8},
        {"JapanGothic", "lgfxJapanGothic_12", 12, &fonts::lgfxJapanGothic_12},
        {"JapanGothic", "lgfxJapanGothic_16", 16, &fonts::lgfxJapanGothic_16},
        {"JapanGothic", "lgfxJapanGothic_20", 20, &fonts::lgfxJapanGothic_20},
        {"JapanGothic", "lgfxJapanGothic_24", 24, &fonts::lgfxJapanGothic_24},
        {"JapanGothic", "lgfxJapanGothicP_8", 8, &fonts::lgfxJapanGothicP_8},
        {"JapanGothic", "lgfxJapanGothicP_12", 12, &fonts::lgfxJapanGothicP_12},
        {"JapanGothic", "lgfxJapanGothicP_16", 16, &fonts::lgfxJapanGothicP_16},
        {"JapanGothic", "lgfxJapanGothicP_20", 20, &fonts::lgfxJapanGothicP_20},
        {"JapanGothic", "lgfxJapanGothicP_24", 24, &fonts::lgfxJapanGothicP_24},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // eFontCN family (Chinese)
    {
        {"eFontCN", "efontCN_10", 10, &fonts::efontCN_10},
        {"eFontCN", "efontCN_12", 12, &fonts::efontCN_12},
        {"eFontCN", "efontCN_14", 14, &fonts::efontCN_14},
        {"eFontCN", "efontCN_16", 16, &fonts::efontCN_16},
        {"eFontCN", "efontCN_24", 24, &fonts::efontCN_24},
        {nullptr, nullptr, 0, nullptr} // End marker
    },
    // eFontJA family (Japanese)
    {
        {"eFontJA", "efontJA_10", 10, &fonts::efontJA_10},
        {"eFontJA", "efontJA_12", 12, &fonts::efontJA_12},
        {"eFontJA", "efontJA_14", 14, &fonts::efontJA_14},
        {"eFontJA", "efontJA_16", 16, &fonts::efontJA_16},
        {"eFontJA", "efontJA_24", 24, &fonts::efontJA_24},
        {nullptr, nullptr, 0, nullptr} // End marker
    }
    */
};

const int NUM_FONT_FAMILIES = sizeof(fontFamilies) / sizeof(fontFamilies[0]);

// Constructor implementation
FontDisplayManager::FontDisplayManager(DeviceInterface* deviceInterface) : 
    currentFamilyIndex(0), 
    currentFontIndex(0), 
    lastEncoderPosition(-999), 
    sampleText("Sample Text 123"),
    displayChanged(true),
    device(deviceInterface)
{
}

void FontDisplayManager::setDevice(DeviceInterface* deviceInterface)
{
    device = deviceInterface;
    displayChanged = true;
}

// Private method implementations
int FontDisplayManager::getFontsInFamily(int familyIndex) const
{
    if (familyIndex < 0 || familyIndex >= NUM_FONT_FAMILIES) {
        return 0;
    }
    
    int count = 0;
    while (fontFamilies[familyIndex][count].family != nullptr) {
        count++;
    }
    return count;
}

String FontDisplayManager::getFamilyName(int familyIndex) const
{
    if (familyIndex < 0 || familyIndex >= NUM_FONT_FAMILIES) {
        return "Unknown";
    }

    if (fontFamilies[familyIndex][0].family != nullptr) {
        return String(fontFamilies[familyIndex][0].family);
    }
    return "Empty Family";
}

String FontDisplayManager::getFontName(int familyIndex, int fontIndex) const
{
    if (familyIndex < 0 || familyIndex >= NUM_FONT_FAMILIES) {
        return "Invalid Family";
    }
    
    int fontsInFamily = getFontsInFamily(familyIndex);
    if (fontIndex < 0 || fontIndex >= fontsInFamily) {
        return "Invalid Font";
    }

    return String(fontFamilies[familyIndex][fontIndex].name);
}

void FontDisplayManager::mapEncoderToFont(long encoderPosition)
{
    // Calculate total number of fonts across all families
    int totalFonts = 0;
    for (int i = 0; i < NUM_FONT_FAMILIES; i++) {
        totalFonts += getFontsInFamily(i);
    }

    if (totalFonts == 0) {
        currentFamilyIndex = 0;
        currentFontIndex = 0;
        return;
    }

    // Ensure position is positive and wrap around
    long positivePosition = ((encoderPosition % totalFonts) + totalFonts) % totalFonts;
    
    // Find which family and font index this position corresponds to
    int currentPos = 0;
    for (int familyIdx = 0; familyIdx < NUM_FONT_FAMILIES; familyIdx++) {
        int fontsInThisFamily = getFontsInFamily(familyIdx);
        if (positivePosition < currentPos + fontsInThisFamily) {
            currentFamilyIndex = familyIdx;
            currentFontIndex = positivePosition - currentPos;
            return;
        }
        currentPos += fontsInThisFamily;
    }

    // Fallback
    currentFamilyIndex = 0;
    currentFontIndex = 0;
}

// Public method implementations
void FontDisplayManager::setSampleText(const char* text)
{
    sampleText = text;
    displayChanged = true;
}

void FontDisplayManager::update(long encoderPosition)
{
    // Check if encoder position has changed
    if (encoderPosition != lastEncoderPosition) {
        mapEncoderToFont(encoderPosition);
        lastEncoderPosition = encoderPosition;
        displayChanged = true;
    }

    // Update display if needed
    if (displayChanged) {
        displayCurrentFont();
        displayChanged = false;
    }
}

void FontDisplayManager::displayCurrentFont()
{
    if (device == nullptr) {
        return; // Cannot display without a device
    }
    
    String familyName = getFamilyName(currentFamilyIndex);
    String fontName = getFontName(currentFamilyIndex, currentFontIndex);
    int fontSize = getCurrentFontSize();
    const lgfx::IFont* fontPtr = getCurrentFontPtr();
    
    device->displayFont(familyName, fontName, fontSize, fontPtr, sampleText);
}


String FontDisplayManager::getCurrentFamilyName() const
{
    return getFamilyName(currentFamilyIndex);
}

String FontDisplayManager::getCurrentFontName() const
{
    return getFontName(currentFamilyIndex, currentFontIndex);
}

int FontDisplayManager::getTotalFamilies() const
{
    return NUM_FONT_FAMILIES;
}


void FontDisplayManager::forceUpdate()
{
    displayChanged = true;
}

int FontDisplayManager::getCurrentFontSize() const
{
    if (currentFamilyIndex >= 0 && currentFamilyIndex < NUM_FONT_FAMILIES &&
        currentFontIndex >= 0 && currentFontIndex < getFontsInFamily(currentFamilyIndex)) {
        return fontFamilies[currentFamilyIndex][currentFontIndex].size;
    }
    return 0;
}

const lgfx::IFont* FontDisplayManager::getCurrentFontPtr() const
{
    if (currentFamilyIndex >= 0 && currentFamilyIndex < NUM_FONT_FAMILIES &&
        currentFontIndex >= 0 && currentFontIndex < getFontsInFamily(currentFamilyIndex)) {
        return fontFamilies[currentFamilyIndex][currentFontIndex].fontPtr;
    }
    return nullptr;
}

// Global instance for easy access
FontDisplayManager fontManager;