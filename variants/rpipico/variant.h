// #define RADIOLIB_CUSTOM_ARDUINO 1
// #define RADIOLIB_TONE_UNSUPPORTED 1
// #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED 1

#define ARDUINO_ARCH_AVR

// default I2C pins:
// SDA = 4
// SCL = 5

// Recommended pins for SerialModule:
// txd = 8
// rxd = 9

#define OVERRIDE_PUBLIC_KEY { 0xbc, 0x8a, 0xbd, 0x69, 0xea, 0x1c, 0x2c, 0xba, 0xfc, 0x67, 0x1f, 0x11, 0x2b, 0x56, 0x7b, 0xe0, 0xd0, 0xb1, 0xa8, 0x55, 0x84, 0x2b, 0xc0, 0x8d, 0x84, 0x7e, 0xd4, 0xe8, 0x0b, 0x04, 0xac, 0x66 }
#define OVERRIDE_PRIVATE_KEY { 0x10, 0x94, 0x45, 0x0c, 0x04, 0xdd, 0x24, 0x18, 0x08, 0x24, 0x22, 0x8d, 0x94, 0x8a, 0x22, 0x1a, 0xdc, 0x9a, 0x03, 0xa1, 0xf7, 0xa3, 0x7e, 0x6b, 0x69, 0x3c, 0x4c, 0x9d, 0x5f, 0xae, 0xe8, 0x45 }

#define EXT_NOTIFY_OUT 22
#define BUTTON_PIN 17

#define LED_PIN PIN_LED

#define BATTERY_PIN 26
// ratio of voltage divider = 3.0 (R17=200k, R18=100k)
#define ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic
#define BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION

#define USE_SX1262

#undef LORA_SCK
#undef LORA_MISO
#undef LORA_MOSI
#undef LORA_CS

#define LORA_SCK 10
#define LORA_MISO 12
#define LORA_MOSI 11
#define LORA_CS 3

#define LORA_DIO0 RADIOLIB_NC
#define LORA_RESET 15
#define LORA_DIO1 20
#define LORA_DIO2 2
#define LORA_DIO3 RADIOLIB_NC

#ifdef USE_SX1262
#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_DIO2
#define SX126X_RESET LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#endif
