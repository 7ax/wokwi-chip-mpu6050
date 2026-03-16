# Wokwi MPU-6050 Custom Chip

A [Wokwi](https://wokwi.com/) custom chip emulating the InvenSense/TDK MPU-6050 6-axis accelerometer and gyroscope on a GY-521 breakout board. Communicates over I2C and provides interactive slider controls for accelerometer (X/Y/Z), gyroscope (X/Y/Z), and temperature values. Implements the MPU-6050 register map with correct power-on defaults, sleep mode, configurable full-scale ranges, and burst reads.

## Usage

Add the chip to your Wokwi project's `diagram.json`:

```json
{
  "version": 1,
  "author": "Your Name",
  "editor": "wokwi",
  "parts": [
    { "type": "wokwi-arduino-uno", "id": "uno", "top": 0, "left": 0 },
    {
      "type": "chip-mpu-6050",
      "id": "mpu",
      "top": -80,
      "left": 100,
      "attrs": { "github": "7ax/wokwi-chip-mpu6050@1.0.0" }
    }
  ],
  "connections": [
    ["uno:A4", "mpu:SDA", "blue", ["v-10"]],
    ["uno:A5", "mpu:SCL", "green", ["v-20"]],
    ["uno:5V", "mpu:VCC", "red", ["v-30"]],
    ["uno:GND", "mpu:GND", "black", ["v-40"]],
    ["uno:GND", "mpu:AD0", "black", ["v-50"]]
  ]
}
```

## Pin Configuration

| Pin | Function |
|-----|----------|
| VCC | Power supply (3.3V–5V) |
| GND | Ground |
| SCL | I2C clock |
| SDA | I2C data |
| XDA | Auxiliary I2C data (not emulated) |
| XCL | Auxiliary I2C clock (not emulated) |
| AD0 | I2C address select: LOW/floating → 0x68, HIGH → 0x69 |
| INT | Interrupt output (not emulated) |

## Slider Controls

| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| Accel X (g) | -2.0 to 2.0 | 0.0 | X-axis acceleration in g |
| Accel Y (g) | -2.0 to 2.0 | 0.0 | Y-axis acceleration in g |
| Accel Z (g) | -2.0 to 2.0 | 1.0 | Z-axis acceleration in g (1g = gravity) |
| Gyro X (°/s) | -250 to 250 | 0.0 | X-axis angular rate in °/s |
| Gyro Y (°/s) | -250 to 250 | 0.0 | Y-axis angular rate in °/s |
| Gyro Z (°/s) | -250 to 250 | 0.0 | Z-axis angular rate in °/s |
| Temp (°C) | -40 to 85 | 25.0 | Die temperature in °C |

## Supported Registers

| Address | Name | Default | Access | Notes |
|---------|------|---------|--------|-------|
| 0x19 | SMPRT_DIV | 0x00 | R/W | Stored, no functional effect |
| 0x1A | CONFIG | 0x00 | R/W | Stored, no functional effect |
| 0x1B | GYRO_CONFIG | 0x00 | R/W | Bits 4:3 select full-scale range |
| 0x1C | ACCEL_CONFIG | 0x00 | R/W | Bits 4:3 select full-scale range |
| 0x23 | FIFO_EN | 0x00 | R/W | Stored, no functional effect |
| 0x37 | INT_PIN_CFG | 0x00 | R/W | Stored, no functional effect |
| 0x38 | INT_ENABLE | 0x00 | R/W | Stored, no functional effect |
| 0x3A | INT_STATUS | 0x01 | R | 0x01 when awake, 0x00 when sleeping |
| 0x3B–0x40 | ACCEL_XOUT/YOUT/ZOUT | — | R | 16-bit big-endian accelerometer data |
| 0x41–0x42 | TEMP_OUT | — | R | 16-bit big-endian temperature data |
| 0x43–0x48 | GYRO_XOUT/YOUT/ZOUT | — | R | 16-bit big-endian gyroscope data |
| 0x68 | SIGNAL_PATH_RESET | 0x00 | W | Write clears sensor registers, auto-clears |
| 0x6A | USER_CTRL | 0x00 | R/W | Stored, no functional effect |
| 0x6B | PWR_MGMT_1 | 0x40 | R/W | Bit 7: device reset. Bit 6: sleep mode |
| 0x6C | PWR_MGMT_2 | 0x00 | R/W | Stored, no functional effect |
| 0x75 | WHO_AM_I | 0x68 | R | Always returns 0x68 |

## Known Limitations

- **Slider ranges are fixed.** Accelerometer sliders cover ±2g and gyroscope sliders cover ±250°/s. If firmware changes the full-scale range (e.g., to ±16g), slider values cannot exceed the fixed range. The sensitivity conversion is still applied correctly — values within slider range produce correct raw output at any full-scale setting.
- **AD0 is sampled once at initialization.** Changing the AD0 pin after simulation starts has no effect on the I2C address.
- **FIFO is not functional.** The FIFO_EN register accepts writes but the FIFO buffer is not implemented.
- **No I2C master passthrough.** The auxiliary I2C bus (XDA/XCL) is not emulated.
- **INT pin is not driven.** The interrupt pin does not assert on data-ready or other events.
- **DMP (Digital Motion Processor) is not emulated.**

## Register Map Reference

[MPU-6000/MPU-6050 Register Map and Descriptions (Rev 4.2)](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf)

## Building

The chip compiles to WebAssembly automatically via GitHub Actions on push. To build locally with Docker:

```bash
make
```

## License

MIT — see [LICENSE](LICENSE).
