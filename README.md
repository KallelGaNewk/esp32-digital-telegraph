# Digital telegraph using ESP32, Arduino compatible

## Components

- Adafruit SSD1306 128x64 I2C (4 pins), if you have a different display, change in the lines 13 and 14.
- A button.

## Configuration

- WiFi:
    - Change the `ssid` and `password` in lines 10 and 11.

- Display (optional):
    - Change the `SCREEN_ADDRESS` in line 13.
    - Change the `displayWidth` and `displayHeight` in line 14.

- Button (optional):
    - Change the `BUTTON_PIN` in line 16.

## Connections

Display:
- GND to GND,
- VCC to 3.3V or 5V
- SCL and SDA pins (these are defined by Wire lib):<br>
**ESP32:** SDA on D21, SCL on D22;<br>
**Arduino UNO:** SDA on A4, SCL on A5;<br>
**Arduino MEGA 2560:** SDA on 20, SCL on 21;<br>
**Arduino LEONARDO:** SDA on 2, SCL on 3;

Button: between GND and D23.
