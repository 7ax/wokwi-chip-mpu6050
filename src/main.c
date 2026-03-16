#include "wokwi-api.h"
#include <string.h>

/* MPU-6050 Register Addresses */
#define REG_SMPRT_DIV        0x19
#define REG_CONFIG           0x1A
#define REG_GYRO_CONFIG      0x1B
#define REG_ACCEL_CONFIG     0x1C
#define REG_FIFO_EN          0x23
#define REG_INT_PIN_CFG      0x37
#define REG_INT_ENABLE       0x38
#define REG_INT_STATUS       0x3A
#define REG_ACCEL_XOUT_H     0x3B
#define REG_TEMP_OUT_H       0x41
#define REG_GYRO_XOUT_H      0x43
#define REG_SIGNAL_PATH_RST  0x68
#define REG_USER_CTRL        0x6A
#define REG_PWR_MGMT_1       0x6B
#define REG_PWR_MGMT_2       0x6C
#define REG_WHO_AM_I         0x75

#define WHO_AM_I_VALUE       0x68
#define PWR_MGMT_1_DEFAULT   0x40  /* SLEEP bit set */
#define SENSOR_DATA_LEN      14    /* 0x3B..0x48 */

typedef struct {
  uint8_t  registers[128];
  uint8_t  reg_pointer;
  bool     first_byte;

  pin_t    vcc_pin;
  pin_t    gnd_pin;
  pin_t    scl_pin;
  pin_t    sda_pin;
  pin_t    ad0_pin;
  pin_t    int_pin;
  pin_t    xda_pin;
  pin_t    xcl_pin;

  uint32_t attr_accel_x;
  uint32_t attr_accel_y;
  uint32_t attr_accel_z;
  uint32_t attr_gyro_x;
  uint32_t attr_gyro_y;
  uint32_t attr_gyro_z;
  uint32_t attr_temp;
} chip_state_t;

static chip_state_t chip_state;

static int16_t clamp_i16(double value) {
  if (value > 32767.0) return 32767;
  if (value < -32768.0) return -32768;
  return (int16_t)value;
}

static void reset_registers(chip_state_t *state) {
  memset(state->registers, 0x00, 128);
  state->registers[REG_WHO_AM_I]   = WHO_AM_I_VALUE;
  state->registers[REG_PWR_MGMT_1] = PWR_MGMT_1_DEFAULT;
  state->registers[REG_INT_STATUS] = 0x00; /* No data ready while sleeping */
}

static void store_i16(uint8_t *dst, int16_t value) {
  dst[0] = (uint8_t)((uint16_t)value >> 8);
  dst[1] = (uint8_t)((uint16_t)value & 0xFF);
}

static void on_timer(void *user_data) {
  chip_state_t *state = (chip_state_t *)user_data;

  float accel_x = attr_read_float(state->attr_accel_x);
  float accel_y = attr_read_float(state->attr_accel_y);
  float accel_z = attr_read_float(state->attr_accel_z);
  float gyro_x  = attr_read_float(state->attr_gyro_x);
  float gyro_y  = attr_read_float(state->attr_gyro_y);
  float gyro_z  = attr_read_float(state->attr_gyro_z);
  float temp    = attr_read_float(state->attr_temp);

  /* Check sleep mode (bit 6 of PWR_MGMT_1) */
  if (state->registers[REG_PWR_MGMT_1] & 0x40) {
    memset(&state->registers[REG_ACCEL_XOUT_H], 0, SENSOR_DATA_LEN);
    state->registers[REG_INT_STATUS] = 0x00;
    return;
  }

  state->registers[REG_INT_STATUS] = 0x01; /* DATA_RDY */

  /* Accelerometer: sensitivity = 16384 >> AFS_SEL */
  uint8_t afs_sel = (state->registers[REG_ACCEL_CONFIG] >> 3) & 0x03;
  double accel_sens = (double)(16384 >> afs_sel);

  store_i16(&state->registers[0x3B], clamp_i16(accel_x * accel_sens));
  store_i16(&state->registers[0x3D], clamp_i16(accel_y * accel_sens));
  store_i16(&state->registers[0x3F], clamp_i16(accel_z * accel_sens));

  /* Temperature: raw = (temp_c - 36.53) * 340.0 */
  store_i16(&state->registers[0x41], clamp_i16((temp - 36.53) * 340.0));

  /* Gyroscope: sensitivity = 131.0 / (1 << FS_SEL) */
  uint8_t fs_sel = (state->registers[REG_GYRO_CONFIG] >> 3) & 0x03;
  double gyro_sens = 131.0 / (double)(1 << fs_sel);

  store_i16(&state->registers[0x43], clamp_i16(gyro_x * gyro_sens));
  store_i16(&state->registers[0x45], clamp_i16(gyro_y * gyro_sens));
  store_i16(&state->registers[0x47], clamp_i16(gyro_z * gyro_sens));
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *state = (chip_state_t *)user_data;
  (void)address;
  (void)read;
  state->first_byte = true;
  return true; /* ACK */
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *state = (chip_state_t *)user_data;
  uint8_t value = state->registers[state->reg_pointer & 0x7F];
  state->reg_pointer = (state->reg_pointer + 1) & 0x7F;
  return value;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *state = (chip_state_t *)user_data;

  if (state->first_byte) {
    state->reg_pointer = data & 0x7F;
    state->first_byte = false;
    return true; /* ACK */
  }

  uint8_t reg = state->reg_pointer & 0x7F;

  if (reg == REG_WHO_AM_I) {
    /* Read-only: ignore writes */
  } else if (reg == REG_PWR_MGMT_1) {
    state->registers[reg] = data;
    if (data & 0x80) {
      /* DEVICE_RESET: restore defaults, bit auto-clears */
      reset_registers(state);
    }
  } else if (reg == REG_SIGNAL_PATH_RST) {
    /* Clear sensor registers and auto-clear reset register */
    memset(&state->registers[REG_ACCEL_XOUT_H], 0, SENSOR_DATA_LEN);
    state->registers[REG_SIGNAL_PATH_RST] = 0x00;
  } else {
    state->registers[reg] = data;
  }

  state->reg_pointer = (state->reg_pointer + 1) & 0x7F;
  return true; /* ACK */
}

static void on_i2c_disconnect(void *user_data) {
  (void)user_data;
}

void chip_init(void) {
  chip_state_t *state = &chip_state;
  memset(state, 0, sizeof(*state));

  /* Initialize pins */
  state->vcc_pin = pin_init("VCC", INPUT);
  state->gnd_pin = pin_init("GND", INPUT);
  state->scl_pin = pin_init("SCL", INPUT);
  state->sda_pin = pin_init("SDA", INPUT);
  state->ad0_pin = pin_init("AD0", INPUT_PULLDOWN);
  state->int_pin = pin_init("INT", INPUT);
  state->xda_pin = pin_init("XDA", INPUT);
  state->xcl_pin = pin_init("XCL", INPUT);

  /* I2C address: 0x68 when AD0=LOW/floating, 0x69 when AD0=HIGH */
  uint32_t address = pin_read(state->ad0_pin) ? 0x69 : 0x68;

  /* Initialize register defaults */
  reset_registers(state);

  /* Initialize I2C slave */
  const i2c_config_t i2c_config = {
    .user_data   = state,
    .address     = address,
    .scl         = state->scl_pin,
    .sda         = state->sda_pin,
    .connect     = on_i2c_connect,
    .read        = on_i2c_read,
    .write       = on_i2c_write,
    .disconnect  = on_i2c_disconnect,
  };
  i2c_init(&i2c_config);

  /* Initialize control attributes */
  state->attr_accel_x = attr_init_float("accelX", 0.0f);
  state->attr_accel_y = attr_init_float("accelY", 0.0f);
  state->attr_accel_z = attr_init_float("accelZ", 1.0f);
  state->attr_gyro_x  = attr_init_float("gyroX", 0.0f);
  state->attr_gyro_y  = attr_init_float("gyroY", 0.0f);
  state->attr_gyro_z  = attr_init_float("gyroZ", 0.0f);
  state->attr_temp    = attr_init_float("temperature", 25.0f);

  /* Start periodic sensor update timer (10 ms) */
  const timer_config_t timer_cfg = {
    .user_data = state,
    .callback  = on_timer,
  };
  timer_t tmr = timer_init(&timer_cfg);
  timer_start(tmr, 10000, true);
}
