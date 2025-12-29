/**
 * @file receiver_main.cpp
 * @brief Thermostat Receiver - LoRaWAN relay for gas stove control
 * @url https://github.com/vashjuan/M5Dial_thermo
 * @author John Cornelison (john@vashonSoftware.com)
 * @brief XIAO ESP32S3 + Grove-Wio-E5 receiver for thermostat stove control
 * @version 1.0.0
 * @date 2025-12-29
 *
 * @hardware:
 *   - Seeed XIAO ESP32S3 
 *     https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html
 *   - Grove-Wio-E5 Wireless Module - STM32WLE5JC (LoRaWAN)
 *     https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html
 *
 * @functionality:
 *   - Receives LoRaWAN commands from M5Stack Dial thermostat
 *   - Controls gas stove via pin D10 (HIGH/LOW)
 *   - Provides status feedback via LED
 *   - Implements failsafe timeout for safety
 *
 * @pin_assignments:
 *   - D10: Gas stove control output (HIGH = ON, LOW = OFF)
 *   - D9: Status LED (optional)
 *   - D6/D7: Grove-Wio-E5 UART (RX/TX)
 *   
 * @safety_features:
 *   - Automatic timeout to turn OFF stove if no signal received
 *   - Watchdog timer protection
 *   - Status LED for visual feedback
 *   - Serial debugging for troubleshooting
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "lora_receiver.hpp"
#include "stove_relay.hpp"
#include "status_led.hpp"

// Pin definitions for XIAO ESP32S3
const int STOVE_CONTROL_PIN = D10;    // Output to gas stove control
const int STATUS_LED_PIN = D9;        // Status LED (optional)
const int LORA_RX_PIN = D6;          // Grove-Wio-E5 TX -> ESP32 RX
const int LORA_TX_PIN = D7;          // Grove-Wio-E5 RX -> ESP32 TX

// Safety timeout - turn off stove if no signal received (in milliseconds)
const unsigned long SAFETY_TIMEOUT = 10 * 60 * 1000; // 10 minutes

// Component instances
LoRaReceiver loraReceiver;
StoveRelay stoveRelay;
StatusLED statusLED;

// Global state tracking
unsigned long lastCommandTime = 0;
bool systemInitialized = false;

void setup() {
    Serial.begin(115200);
    delay(1000); // Wait for serial monitor
    
    Serial.println("====================================");
    Serial.println("Thermostat Receiver Starting...");
    Serial.println("Hardware: XIAO ESP32S3 + Grove-Wio-E5");
    Serial.println("====================================");
    
    // Configure watchdog timer
    esp_task_wdt_init(30, true); // 30 second timeout, panic on timeout
    esp_task_wdt_add(NULL);
    
    // Initialize status LED first for early feedback
    statusLED.setup(STATUS_LED_PIN);
    statusLED.setStatus(STATUS_INITIALIZING);
    
    // Initialize stove relay control
    Serial.println("Initializing stove relay...");
    if (!stoveRelay.setup(STOVE_CONTROL_PIN)) {
        Serial.println("ERROR: Failed to initialize stove relay!");
        statusLED.setStatus(STATUS_ERROR);
        while(1) {
            delay(1000);
            esp_task_wdt_reset();
        }
    }
    
    // Ensure stove is OFF during startup
    stoveRelay.turnOff();
    Serial.println("Stove relay initialized - SAFETY: Stove turned OFF");
    
    // Initialize LoRa receiver
    Serial.println("Initializing LoRa receiver...");
    if (!loraReceiver.setup(LORA_RX_PIN, LORA_TX_PIN)) {
        Serial.println("ERROR: Failed to initialize LoRa receiver!");
        statusLED.setStatus(STATUS_ERROR);
        while(1) {
            delay(1000);
            esp_task_wdt_reset();
        }
    }
    
    Serial.println("LoRa receiver initialized successfully");
    
    // System ready
    systemInitialized = true;
    lastCommandTime = millis();
    statusLED.setStatus(STATUS_WAITING);
    
    Serial.println("====================================");
    Serial.println("System Ready - Waiting for commands");
    Serial.printf("Safety timeout: %lu minutes\n", SAFETY_TIMEOUT / (60 * 1000));
    Serial.println("====================================");
    Serial.println();
    
    esp_task_wdt_reset();
}

void loop() {
    esp_task_wdt_reset(); // Feed watchdog
    
    if (!systemInitialized) {
        delay(100);
        return;
    }
    
    // Check for incoming LoRa commands
    String command = loraReceiver.checkForCommand();
    
    if (command.length() > 0) {
        Serial.printf("Received command: %s\n", command.c_str());
        lastCommandTime = millis();
        
        // Process the command
        bool commandSuccess = false;
        
        if (command.equalsIgnoreCase("STOVE_ON")) {
            stoveRelay.turnOn();
            statusLED.setStatus(STATUS_STOVE_ON);
            Serial.println("Command executed: Stove turned ON");
            commandSuccess = true;
            
        } else if (command.equalsIgnoreCase("STOVE_OFF")) {
            stoveRelay.turnOff();
            statusLED.setStatus(STATUS_STOVE_OFF);
            Serial.println("Command executed: Stove turned OFF");
            commandSuccess = true;
            
        } else if (command.equalsIgnoreCase("STATUS_REQUEST")) {
            // Send back current status
            String status = stoveRelay.isOn() ? "STOVE_ON" : "STOVE_OFF";
            loraReceiver.sendResponse(status);
            Serial.printf("Status response sent: %s\n", status.c_str());
            commandSuccess = true;
            
        } else {
            Serial.printf("Unknown command received: %s\n", command.c_str());
            loraReceiver.sendResponse("ERROR_UNKNOWN_COMMAND");
        }
        
        if (commandSuccess) {
            // Send acknowledgment
            loraReceiver.sendResponse("ACK");
        }
    }
    
    // Safety timeout check - turn off stove if no commands received
    unsigned long timeSinceLastCommand = millis() - lastCommandTime;
    if (timeSinceLastCommand > SAFETY_TIMEOUT) {
        if (stoveRelay.isOn()) {
            Serial.println("SAFETY TIMEOUT: No commands received, turning stove OFF");
            stoveRelay.turnOff();
            statusLED.setStatus(STATUS_TIMEOUT);
            
            // Send timeout notification if possible
            loraReceiver.sendResponse("SAFETY_TIMEOUT");
        }
        
        // Reset the timer to prevent spam
        lastCommandTime = millis();
    }
    
    // Update status LED
    statusLED.update();
    
    // Small delay to prevent excessive CPU usage
    delay(100);
}