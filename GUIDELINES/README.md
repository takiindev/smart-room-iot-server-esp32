# SmartRoom IoT - Implementation Guidelines

Tài liệu hướng dẫn cho developer về cách implement, cấu hình, và debug hệ thống SmartRoom IoT.

## Danh sách Guides

### 1. Library Installation
- File: `library-installation.md`
- Nội dung: Cách cài đặt tất cả thư viện cần thiết
- Khi dùng: Setup environment lần đầu

### 2. Sensor Initialization
- File: `sensor-initialization.md`
- Nội dung: Chiến lược init sensor từ sớm (Early Init)
- Lợi ích: Tránh timeout khi API call lần đầu
- Khi dùng: Hiểu flow init sensor trong `setup()`

### 3. Device Config Structure
- File: `device-config-structure.md`
- Nội dung: Cấu trúc JSON config cho tất cả loại thiết bị
- Bao gồm: LIGHT, FAN, AIR_CONDITION, TEMPERATURE, HUMIDITY
- Khi dùng: Cấu hình device mới hoặc sửa config

### 4. SCD40 Sensor
- File: `scd40-sensor.md`
- Nội dung: Hướng dẫn kỹ thuật chi tiết về cảm biến SCD40
- Bao gồm: Hardware setup, I2C config, timing, troubleshooting
- Khi dùng: Debug sensor SCD40 hoặc thêm cảm biến mới

---

## Quick Start

### 1. First Time Setup
1. Đọc: `library-installation.md`
2. Cài đặt tất cả libraries
3. Upload firmware

### 2. Understanding the System
1. Đọc: `sensor-initialization.md`
   - Hiểu tại sao dùng Early Init
   - Xem serial log output

### 3. Configuring Devices
1. Đọc: `device-config-structure.md`
2. Edit `config` JSON trong firmware
3. Thêm device mới vào mảng `devices`

### 4. Debugging SCD40
1. Đọc: `scd40-sensor.md`
2. Kiểm tra hardware connections
3. Xem serial logs

---

## Common Tasks

### Thêm cảm biến nhiệt độ DS18B20 mới

1. Mở `device-config-structure.md`
2. Tìm section "TEMPERATURE Sensor"
3. Thêm device object vào mảng `devices`
4. Chỉ định GPIO pin
5. Upload firmware

Ví dụ config:
```json
{
  "naturalId": "TEMP_ESP32_03",
  "category": "TEMPERATURE",
  "gpioPin": [5],
  "internal": {
    "peripheralType": "SENSOR",
    "module": "DS18B20"
  }
}
```

### Thêm cảm biến SCD40 (nhiệt độ + độ ẩm)

1. Đọc: `scd40-sensor.md` - Check hardware setup
2. Đọc: `library-installation.md` - Cài SensirionI2cScd4x
3. Mở `device-config-structure.md`
4. Thêm device object với `"module": "SCD40"`
5. Upload firmware
6. Chờ 5 giây boot để sensor warm-up

Ví dụ config:
```json
{
  "naturalId": "HUM_ESP32_01",
  "category": "HUMIDITY",
  "gpioPin": [21, 22],
  "internal": {
    "peripheralType": "SENSOR",
    "module": "SCD40"
  }
}
```

### Thêm thiết bị AC (điều hòa) mới

1. Mở `device-config-structure.md`
2. Tìm section "AIR_CONDITION"
3. Copy template hoàn chỉnh
4. Sửa `naturalId`, `brand`, IR codes
5. Upload firmware
6. Test điều khiển qua API `/control`

Bước tìm IR codes:
- Dùng IR analyzer/scanner
- Hoặc tham khảo database IR trực tuyến
- Hoặc dùng IRremoteESP8266 examples để scan

### Debug sensor không phản hồi

1. Đọc: `scd40-sensor.md` - Troubleshooting section
2. Kiểm tra GPIO pins khớp với config
3. Kiểm tra Pull-up resistors 4.7kΩ
4. Kiểm tra VCC 3.3V + GND
5. Xem serial logs để tìm error message
6. Upload đặc biệt với `-v` flag để debug I2C

### Đọc dữ liệu telemetry

```bash
# Đăng nhập lấy token
curl -X POST http://172.16.64.200:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"vuesp","password":"123456789"}'

# Lấy dữ liệu telemetry
curl -X GET http://172.16.64.200:8080/telemetry \
  -H "Authorization: Bearer <token>"
```

---

## Files Structure

```
smart-room-iot-server-esp32/
├── API_DOCS/                      <- API Documentation
│   ├── README.md                  <- API Overview
│   ├── authentication.md           <- /auth/login
│   ├── temperature.md              <- /temperature
│   ├── humidity.md                 <- /humidity
│   ├── telemetry.md                <- /telemetry
│   ├── control.md                  <- /control
│   └── setup.md                    <- /setup
│
├── GUIDELINES/                    <- Implementation Guides
│   ├── README.md                  <- This file
│   ├── library-installation.md    <- Setup thư viện
│   ├── sensor-initialization.md   <- Init strategy
│   ├── device-config-structure.md <- Config guide
│   └── scd40-sensor.md             <- SCD40 technical
│
├── README.md                      <- System Overview
└── [other files]
```

---

## Serial Debugging

### Enable Serial Monitor

```
Arduino IDE -> Tools -> Serial Monitor
Baud Rate: 115200
```

### Typical Boot Sequence

```
13:45:00.123 -> Đang kết nối WiFi...
13:45:01.456 -> Kết nối WiFi thành công!
13:45:02.000 -> [TIME SYNC SUCCESS]
13:45:02.100 -> Re-init RELAY...

========================================
[SENSOR INITIALIZATION] Bắt đầu...
========================================
[SENSOR INIT] Found SCD40: HUM_ESP32_01 (SDA=21, SCL=22)
[SENSOR INIT] Initializing SCD40...
[SCD40] Serial Number: 0xC962B5073B26
[SCD40] Khởi tạo thành công lần đầu!
[SCD40] I2C (SDA=21, SCL=22) -> CO2: 420 ppm, Temp: 25.50°C, Humidity: 45.20%RH
[SENSOR INIT] SCD40 initialized successfully!
========================================
[SENSOR INITIALIZATION] Hoàn tát
========================================

========================================
[SERVER STARTED] Listening on port 8080
========================================
```

### Debugging API Calls

Khi gọi API, serial sẽ in:

```
------------------------------------------
Endpoint:/temperature
[DEBUG] Request received!
[DEBUG] Query param naturalId found: TEMP_ESP32_01
[REQUEST] naturalId: TEMP_ESP32_01
[DEVICE FOUND] naturalId: TEMP_ESP32_01
[SCD40] Data ready after 0 retries
[SCD40] I2C (SDA=21, SCL=22) -> CO2: 420 ppm, Temp: 25.50°C, Humidity: 45.20%RH
[API RESPONSE]
{
  "status": 200,
  "message": "Lấy nhiệt độ thành công",
  "data": {"tempC": "25.500"},
  "timestamp": "2026-07-14T10:30:45Z"
}
[END RESPONSE]
------------------------------------------
```

---

## Troubleshooting Checklist

### Sensor không khởi tạo
- Đọc: `scd40-sensor.md`
- Kiểm tra GPIO pins: SDA=21, SCL=22
- Kiểm tra Pull-up resistors 4.7kΩ
- Kiểm tra VCC 3.3V + GND
- Xem serial logs

### API timeout
- Kiểm tra sensor warm-up (5 giây)
- Kiểm tra config có device không
- Xem `sensor-initialization.md`

### Config parse error
- Kiểm tra JSON syntax
- Không có comment trong JSON
- Tất cả string dùng double quotes
- Xem serial logs để tìm error message

### Compilation error
- Đọc: `library-installation.md`
- Cài đủ libraries
- Kiểm tra include case-sensitive: `SensirionI2cScd4x.h`

---

## References

- SmartRoom API Docs: `../API_DOCS/README.md`
- System README: `../README.md`
- Sensirion SCD40: https://sensirion.com/products/catalog/SCD40/
- Arduino I2C: https://www.arduino.cc/en/Reference/Wire
- IRremoteESP8266: https://github.com/crankyoldgit/IRremoteESP8266

---

Last Updated: 2026-07-14
Version: 1.0.0
