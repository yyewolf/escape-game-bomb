# Fake Bomb Device

A realistic-looking fake bomb device built for ESP32, featuring an LCD display, keypad input, LED indicators, buzzer alerts, and Bluetooth Low Energy (BLE) connectivity for remote configuration.

## Project Overview

This project creates an interactive prop device that simulates a bomb defusal scenario. The device requires a correct password to be entered via a 4x4 keypad to "defuse" the bomb. It includes visual and audio feedback through LEDs and a buzzer, making it perfect for escape rooms, training scenarios, or entertainment purposes.

**Important:** This is a prop device intended for educational and entertainment purposes only.

## Hardware Requirements

### Core Components
- ESP32 development board (ESP32 DevKit V1)
- 16x2 LCD display with I2C backpack
- 4x4 matrix keypad with I2C interface
- Active buzzer
- Red LED
- Green LED
- Resistors for LED current limiting
- Breadboard and jumper wires

### Pin Configuration

The following pin assignments are defined in the source code:

```cpp
#define RED_LIGHT_PIN 5        // Red LED indicator
#define GREEN_LIGHT_PIN 18     // Green LED indicator  
#define BUZZER_PIN 4           // Active buzzer
```

### I2C Device Addresses

```cpp
#define KEYPAD_ADDRESS 0x20    // 4x4 Keypad I2C address
#define LCD_ADDRESS 0x27       // LCD I2C backpack address
```

Note: These addresses may need adjustment based on your specific I2C modules.

## Software Dependencies

The project uses PlatformIO with the following libraries:
- LiquidCrystal_I2C_ESP32 (v1.1.6) - LCD control
- I2CKeyPad (v0.5.0) - Keypad interface
- Built-in ESP32 BLE libraries

## Device States

The bomb operates in three distinct states:

**ACTIVE**: Default operational state where the device waits for password input. Features periodic beeping and blinking red LED to create urgency.

**WRONG_PASSWORD**: Triggered when an incorrect password is entered. Displays error message, rapid red LED flashing, and error beeps before returning to ACTIVE state.

**DEFUSED**: Success state when correct password is entered. Shows congratulatory message with username, activates green LED, and automatically resets after 60 seconds.

## Password System

The default password is hardcoded as `"1075043"` but can be changed via BLE connection. The device accepts numeric input through the keypad, with '#' functioning as a backspace key.

## Bluetooth Low Energy Features

The device advertises as "4K LGTV" and provides three BLE characteristics:

**State Characteristic (Read-only)**: Reports current device state and user input progress

**Username Characteristic (Read/Write)**: Sets the username displayed upon successful defusal

**Password Characteristic (Read/Write)**: Allows remote password modification

Service UUID: `19B10010-E8F2-537E-4F6C-D104768A1214`

## Building and Deployment

### Prerequisites
Install PlatformIO Core or use the PlatformIO IDE extension in VS Code.

### Compilation
Navigate to the project directory and run:
```
pio run
```

### Upload to Device
Connect your ESP32 via USB and execute:
```
pio run --target upload
```

### Serial Monitoring
Monitor device output with:
```
pio device monitor
```

## Wiring Diagram

Connect components according to the pin definitions above:
- Connect I2C devices (LCD and keypad) to ESP32 I2C pins (SDA: GPIO21, SCL: GPIO22)
- Connect LEDs to their respective GPIO pins with appropriate current-limiting resistors
- Connect buzzer to GPIO 4
- Ensure proper power distribution (3.3V/5V as required by components)

## Customization Options

### Timing Adjustments
Modify beep intervals, flash patterns, and state transition delays in the state machine task functions.

### Password Length
The system automatically detects when enough characters are entered based on the configured password length.

### Audio Patterns
Customize beep frequencies and durations by modifying the `beep()` function calls throughout the code.

### Display Messages
Update LCD text strings in the state transition functions to customize user interface messages.

## Safety Considerations

This device is designed as a harmless prop. Ensure all electrical connections are properly insulated and consider adding clear labeling to identify it as a training device or prop to avoid any misunderstandings.

## Troubleshooting

**Device not responding**: Check I2C connections and verify device addresses match your hardware.

**BLE not discoverable**: Ensure ESP32 BLE stack is functioning and device name appears in Bluetooth scans.

**Keypad input issues**: Verify keypad layout matches the configured character map in the code.

**LCD display problems**: Confirm I2C address and check contrast adjustment on the LCD module.

## Technical Architecture

The system uses FreeRTOS tasks for concurrent operation:
- Main loop handles keypad input and BLE updates
- State machine task manages device state transitions
- Active beep task provides periodic audio alerts during ACTIVE state

This multi-threaded approach ensures responsive user interaction while maintaining consistent background operations.
