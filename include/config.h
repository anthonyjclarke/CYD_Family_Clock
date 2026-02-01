// CYD Family Clock - Hardware Configuration
// Pin definitions and sensor selection

#ifndef CONFIG_H
#define CONFIG_H

// =========================
// I2C Sensor Configuration
// =========================
// Uncomment ONE of the following sensor types to enable support
// Only one sensor type can be active at a time

#define USE_BMP280   // Bosch BMP280 - Temperature + Pressure
// #define USE_BME280   // Bosch BME280 - Temperature + Humidity + Pressure
// #define USE_SHT3X    // Sensirion SHT3X - Temperature + Humidity
// #define USE_HTU21D   // TE HTU21D - Temperature + Humidity

// I2C Pins (CYD Temp/Humidity Interface)
#define SENSOR_SDA_PIN  27   // I2C Data
#define SENSOR_SCL_PIN  22   // I2C Clock

// Sensor update interval (milliseconds)
#define SENSOR_UPDATE_INTERVAL 10000  // 10 seconds

#endif // CONFIG_H
