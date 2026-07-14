# SCD40 Sensor - Technical Guide

Hướng dẫn kỹ thuật chi tiết về cảm biến Sensirion I2C SCD40 và cách sử dụng.

## Hardware Specifications

- **Địa chỉ I2C**: 0x62
- **Giao tiếp**: I2C (SDA, SCL)
- **Điện áp**: 3.3V
- **Phạm vi đo**:
  - Nhiệt độ: -10°C đến +60°C
  - Độ ẩm: 0% đến 100% RH
  - CO2: 0 đến 40,000 ppm

## Hardware Setup

### Kết nối I2C

```
ESP32 (SDA=21)  -------- SCD40 (SDA)
ESP32 (SCL=22)  -------- SCD40 (SCL)
ESP32 (GND)     -------- SCD40 (GND)
ESP32 (3.3V)    -------- SCD40 (VCC)
```

### Pull-up Resistors

**REQUIRED**: Pull-up resistors 4.7kΩ trên SDA và SCL:

```
        ↓ +3.3V
        |
      [4.7kΩ]
        |
SDA ----┼---- to SCD40 SDA
        |
       GND

        ↓ +3.3V
        |
      [4.7kΩ]
        |
SCL ----┼---- to SCD40 SCL
        |
       GND
```

Nếu không có pull-up, sensor sẽ không phản hồi!

## Device Configuration

```json
{
  "naturalId": "TEMP_ESP32_02",
  "category": "TEMPERATURE",
  "specificType": "GPIO",
  "controlType": "GPIO",
  "gpioPin": [21, 22],
  "translations": {
    "vi": {
      "name": "Cảm biến nhiệt độ A101 2",
      "description": "SCD40 temperature sensor"
    },
    "en": {
      "name": "Temperature Sensor A101 2",
      "description": "SCD40 temperature sensor"
    }
  },
  "internal": {
    "peripheralType": "SENSOR",
    "module": "SCD40"
  }
}
```

### GPIO Pin Mapping

- `gpioPin[0]`: SDA (Serial Data)
- `gpioPin[1]`: SCL (Serial Clock)

**Mặc định**: SDA=21, SCL=22

## Library Integration

### Include

```cpp
#include <SensirionI2cScd4x.h>
#include <SensirionCore.h>
```

### Global Object

```cpp
SensirionI2cScd4x scd4x;
bool isScd40Initialized = false;
```

### Initialization

```cpp
void readSCD40Data(int sdaPin, int sclPin, float &temperature, float &humidity)
{
  // Initialize once
  if (!isScd40Initialized) {
    Wire.begin(sdaPin, sclPin);
    scd4x.begin(Wire, 0x62);
    
    // Stop previous measurements
    scd4x.stopPeriodicMeasurement();
    
    // Get serial number (verify sensor)
    uint64_t serialNumber;
    scd4x.getSerialNumber(serialNumber);
    Serial.print("Serial Number: 0x");
    Serial.println((unsigned long long)serialNumber, HEX);
    
    // Start measurement
    scd4x.startPeriodicMeasurement();
    
    // Mark as initialized
    isScd40Initialized = true;
    
    // WAIT 5 SECONDS for first measurement
    delay(5000);
  }
  
  // Check if data ready (retry up to 10 times)
  bool dataReady = false;
  int retries = 0;
  
  while (retries < 10) {
    scd4x.getDataReadyStatus(dataReady);
    if (dataReady) break;
    delay(1000);
    retries++;
  }
  
  if (!dataReady) {
    return false;
  }
  
  // Read measurement
  uint16_t co2;
  scd4x.readMeasurement(co2, temperature, humidity);
  
  return true;
}
```

## Measurement Values

SCD40 cung cấp 3 giá trị trong mỗi lần đọc:

| Giá trị | Kiểu | Phạm vi | Chính xác | Mô tả |
|---------|------|--------|----------|-------|
| CO2 | uint16_t | 0-40000 ppm | ±40 ppm | Nồng độ CO2 (phần triệu) |
| Temperature | float | -10 đến +60°C | ±1.5°C | Nhiệt độ tuyệt đối |
| Humidity | float | 0-100 %RH | ±3% RH | Độ ẩm tương đối |

## Boot Sequence

### Khi firmware start:

```
1. Wire.begin(21, 22)              // Init I2C
2. scd4x.begin(Wire, 0x62)         // Attach sensor to I2C
3. scd4x.stopPeriodicMeasurement() // Stop previous session
4. scd4x.getSerialNumber()         // Verify sensor exists
5. scd4x.startPeriodicMeasurement()// Start measurement cycle
6. delay(5000)                      // WAIT 5 SECONDS!
7. Ready for API calls
```

### Serial Log Output:

```
[SCD40] Serial Number: 0xC962B5073B26
[SCD40] Khởi tạo thành công lần đầu!
[SCD40] Chờ dữ liệu sẵn sàng (5 giây)...
[SCD40] I2C (SDA=21, SCL=22) -> CO2: 420 ppm, Temp: 25.50°C, Humidity: 45.20%RH
```

## API Usage

### GET /temperature?naturalId=TEMP_ESP32_02

```bash
curl -X GET "http://172.16.64.200:8080/temperature?naturalId=TEMP_ESP32_02" \
  -H "Authorization: Bearer <token>"
```

**Response:**
```json
{
  "status": 200,
  "message": "Lấy nhiệt độ thành công",
  "data": {
    "tempC": "25.500"
  },
  "timestamp": "2026-07-14T10:30:45Z"
}
```

### GET /humidity?naturalId=HUM_ESP32_01

```bash
curl -X GET "http://172.16.64.200:8080/humidity?naturalId=HUM_ESP32_01" \
  -H "Authorization: Bearer <token>"
```

**Response:**
```json
{
  "status": 200,
  "message": "Lấy độ ẩm thành công",
  "data": {
    "humidity": "45.50"
  },
  "timestamp": "2026-07-14T10:30:45Z"
}
```

### GET /telemetry

```bash
curl -X GET "http://172.16.64.200:8080/telemetry" \
  -H "Authorization: Bearer <token>"
```

## Timing

### Initialization Phase

| Stage | Duration | Note |
|-------|----------|------|
| I2C Init | Instant | Wire.begin() |
| Sensor Init | Instant | scd4x.begin() |
| Stop Measurement | ~50ms | scd4x.stopPeriodicMeasurement() |
| Get Serial | ~100ms | Verify sensor exists |
| Start Measurement | ~100ms | scd4x.startPeriodicMeasurement() |
| **Warm-up Wait** | **5000ms** | **MUST WAIT 5s!** |

### Reading Phase (per API call)

| Stage | Duration | Note |
|-------|----------|------|
| Check Data Ready | ~50ms | getDataReadyStatus() |
| Read Measurement | ~50ms | readMeasurement() |
| **Total** | **~100ms** | **If data ready** |

If data not ready → Retry (wait 1000ms per retry, max 10 retries)

## Troubleshooting

### Lỗi: "Sensor not found"

```
[SCD40] Sensor not found: command exec error
```

**Nguyên nhân:**
- I2C không khởi tạo đúng
- Sensor không kết nối I2C address 0x62
- Pull-up resistors bị thiếu

**Fix:**
1. Kiểm tra GPIO pins: SDA=21, SCL=22
2. Kiểm tra Pull-up resistors 4.7kΩ
3. Kiểm tra kết nối VCC (3.3V) + GND
4. Dùng I2C Scanner để verify address 0x62

### Lỗi: "Data ready error"

```
[SCD40] Data ready error: command exec error
```

**Nguyên nhân:**
- I2C line bị conflict
- Sensor bị lockup
- Tần suất I2C quá cao

**Fix:**
- Restart firmware
- Đảm bảo không có devices khác trên I2C
- Kiểm tra I2C frequency (phải < 400kHz)

### Sensor không đọc được data

```
[SCD40] Data not ready yet, retry...
```

**Nguyên nhân:**
- Sensor chưa warm-up đủ 5 giây
- Sensor busy

**Fix:**
- Đợi 5 giây sau init
- Nếu vẫn fail → Retry loop sẽ chờ tối đa 10 giây

## Performance Notes

- **Boot time**: +5 seconds (sensor warm-up)
- **First read**: ~100ms (if ready)
- **Retry time**: 10 seconds max (10 retries × 1s)
- **Idle power**: ~2mA @ 3.3V
- **Measurement frequency**: 1Hz (default)

## Advanced Configuration

### Change I2C Frequency

```cpp
Wire.setClock(400000);  // 400kHz (standard I2C)
// or
Wire.setClock(100000);  // 100kHz (slower, more stable)
```

### Measurement Interval

```cpp
// SCD40 supports: 5s, 10s, or periodic measurement
// Current: periodic (fastest, ~2 seconds between readings)
scd4x.startPeriodicMeasurement();
```

### Single Shot Mode (not used currently)

```cpp
// Alternative: single measurement
// scd4x.measureSingleShot();
```

## References

- Sensirion SCD40 Datasheet: https://sensirion.com/products/catalog/SCD40/
- Arduino Library: https://github.com/Sensirion/arduino-i2c-scd4x
- I2C Specification: https://en.wikipedia.org/wiki/I%C2%B2C
