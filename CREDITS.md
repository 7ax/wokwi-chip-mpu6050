# Credits

## Wokwi API Header (`src/wokwi-api.h`)

- **Source:** https://github.com/wokwi/inverter-chip
- **License:** MIT
- **Author:** Uri Shaked / Wokwi
- **What was borrowed:** The `wokwi-api.h` header file providing the Wokwi custom chip C API definitions (pin, I2C, timer, attribute, and framebuffer APIs).
- **Modifications:** None. Used as-is.

## Build Infrastructure (`Makefile`, `.github/workflows/build.yaml`)

- **Source:** https://github.com/wokwi/inverter-chip
- **License:** MIT (SPDX: `SPDX-FileCopyrightText: © 2022 Uri Shaked <uri@wokwi.com>`)
- **What was borrowed:** Makefile structure for wasm32-wasi compilation and GitHub Actions workflow for automated builds and releases.
- **Modifications:** Added `-Wall -Wextra` compiler flags; added board file copying to build and release steps; updated source file references.

## MPU-6050 Register Map

- **Source:** InvenSense/TDK MPU-6000/MPU-6050 Register Map and Descriptions (Revision 4.2)
- **URL:** https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
- **What was used:** Register addresses, default values, bit field definitions, and sensor sensitivity specifications used to implement the I2C register emulation.
