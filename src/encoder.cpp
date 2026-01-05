#include "encoder.hpp"

// Global instance for easy access
Encoder encoder;

// Constructor
Encoder::Encoder() : oldPosition(0)
{
    // Initialize with default position
}

// Destructor
Encoder::~Encoder()
{
    // Cleanup if needed
}

void Encoder::setup()
{
    // Initialize encoder position from M5's encoder
    oldPosition = M5.Encoder.read();

    Serial.println("Encoder initialized using M5Unified encoder");
}

long Encoder::getPosition()
{
    // Read directly from M5's encoder hardware
    return M5.Encoder.read();
}

bool Encoder::hasPositionChanged()
{
    long currentPosition = M5.Encoder.read();
    if (currentPosition != oldPosition)
    {
        oldPosition = currentPosition;
        return true;
    }
    return false;
}