#define BOARD_NAME "pico-pulse-integrated v0.2"

// PIO settings
#define PIO_BASE_GPIO 6
#define PIO_N_GPIO 5
#define PIO_BUF_LEN 65536
#define PIO_EXTRA_CYCLES 4

// Enable laser driver
#define USE_LASER_DRIVER 1

// i2c parameters
#define I2C_DEV i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_SPD 100000

// Rheostat config
#define MON_ADDR 0x2E
#define LIM_ADDR 0x2F
#define LIM_SERIES_RES 1000

// Laser driver pins
#define LASER_NERR 19
#define LASER_NSLP 20

