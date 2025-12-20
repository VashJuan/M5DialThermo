#include "encoder.hpp"

// Constructor
Encoder::Encoder() : oldPosition(-999) {
    // Initialize with default position
}

// Destructor
Encoder::~Encoder() {
    // Cleanup if needed
}

void Encoder::setup() {
    M5Dial.Encoder.begin(); 
    oldPosition = -999;
}

long Encoder::getPosition() {
    return M5Dial.Encoder.read();
}

bool Encoder::hasPositionChanged() {
    long currentPosition = M5Dial.Encoder.read();
    if (currentPosition == oldPosition) 
        return false;
        
    oldPosition = currentPosition;
    return true;
}

// Global instance for easy access
Encoder encoder;