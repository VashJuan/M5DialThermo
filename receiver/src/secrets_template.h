/**
 * @file secrets_template.h
 * @brief Template for LoRaWAN and other sensitive configuration constants (Receiver)
 * @note Copy this file to secrets.h and update with your actual credentials
 * 
 * This is a template file that can be safely committed to version control.
 * It shows the structure of the secrets.h file without exposing actual credentials.
 */

#pragma once

// LoRaWAN Configuration - Replace with your actual LoRaWAN credentials in secrets.h
// These MUST match the same values used in the transmitter (M5Dial)
// Obtain these from your LoRaWAN network provider (e.g., The Things Network)
#define LORAWAN_APP_EUI "0000000000000000"        // 16 hex chars - Application EUI from network provider
#define LORAWAN_APP_KEY "00000000000000000000000000000000"  // 32 hex chars - Application Key from network provider

// Important Notes:
// 1. Both transmitter AND receiver must use the SAME AppEUI and AppKey
// 2. These credentials allow devices to join the same LoRaWAN application
// 3. Never commit your real secrets.h file to version control
// 4. Get these values from your LoRaWAN network provider (TTN, ChirpStack, etc.)

// Optional: WiFi Configuration (if receiver needs WiFi connectivity)
// #define DEFAULT_WIFI_SSID "your_wifi_network_name"
// #define DEFAULT_WIFI_PASSWORD "your_wifi_password"

// You can add other sensitive configuration here as needed
// For example:
// #define API_KEY "your_api_key_here"
// #define MQTT_USERNAME "your_mqtt_username"
// #define MQTT_PASSWORD "your_mqtt_password"