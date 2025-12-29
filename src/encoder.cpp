#include "encoder.hpp"

// Global instance for easy access
Encoder encoder;

// Constructor
Encoder::Encoder() : oldPosition(0) {
    // Initialize with default position
}

// Destructor
Encoder::~Encoder() {
    // Cleanup if needed
}

void Encoder::setup() {
    // Initialize encoder position  
    oldPosition = 0;
    
    // Note: M5Dial encoder handling will be done via interrupts
    // The main loop will handle encoder changes through interrupt flags
}

long Encoder::getPosition() {
    // Return simple position counter
    return oldPosition;
}

bool Encoder::hasPositionChanged() {
    // This will be controlled by interrupt flags in the main program
    // For now, return false as position changes are handled elsewhere
    return false;
}