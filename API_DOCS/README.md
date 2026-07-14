# SmartRoom IoT Server - API Documentation

Tài liệu API cho SmartRoom ESP32 Client.

## Danh sách API

### Authentication
- **[Authentication](./authentication.md)** - Đăng nhập và lấy JWT token
  - `POST /auth/login` - Đăng nhập

### Sensors
- **[Temperature](./temperature.md)** - Đọc dữ liệu nhiệt độ
  - `GET /temperature?naturalId=...` - Lấy dữ liệu từ cảm biến nhiệt độ
  
- **[Humidity](./humidity.md)** - Đọc dữ liệu độ ẩm
  - `GET /humidity?naturalId=...` - Lấy dữ liệu từ cảm biến độ ẩm
  
- **[Telemetry](./telemetry.md)** - Lấy dữ liệu từ tất cả cảm biến
  - `GET /telemetry` - Lấy dữ liệu từ tất cả thiết bị cảm biến

### Device Control
- **[Control](./control.md)** - Điều khiển thiết bị
  - `POST /control` - Điều khiển đèn, quạt, điều hòa

### Configuration
- **[Setup](./setup.md)** - Lấy cấu hình thiết bị
  - `GET /setup` - Lấy toàn bộ cấu hình

## Quy tắc chung

### Authentication
- Tất cả các API (ngoại trừ `/auth/login`) đều yêu cầu header `Authorization: Bearer <token>`
- Token có thể lấy từ endpoint `/auth/login`
- Token hết hạn sau 1 giờ

### Response Format
Tất cả response đều theo format:
```json
{
  "status": 200,
  "message": "Success message",
  "data": { /* data object */ },
  "timestamp": "2026-07-14T10:30:45Z"
}
```

### Query Parameters vs Body
- Sensor API (temperature, humidity, telemetry) dùng **query parameters** (GET)
- Control API dùng **request body** (POST)
- Setup API dùng **GET** (không body)

## Error Status Codes

| Status | Ý nghĩa |
|--------|---------|
| 200 | OK - Thành công |
| 400 | Bad Request - Lỗi input |
| 401 | Unauthorized - Token không hợp lệ |
| 404 | Not Found - Không tìm thấy resource |
| 500 | Internal Server Error - Lỗi phía server |
| 501 | Not Implemented - Chưa hỗ trợ |

## Cảm biến được hỗ trợ

| Loại | Module | Giao tiếp | Chú ý |
|------|--------|----------|-------|
| Temperature | DS18B20 | 1-Wire | Phạm vi: -55 đến 125°C |
| Temperature | SCD40 | I2C | Phạm vi: -40 đến 85°C |
| Humidity | SCD40 | I2C | Phạm vi: 0-100% RH |

## Thiết bị điều khiển được hỗ trợ

| Loại | Mô tả | Chế độ |
|------|-------|-------|
| LIGHT | Đèn | ON/OFF |
| FAN | Quạt | Speed 1-3, ON/OFF |
| AIR_CONDITION | Điều hòa | Power, Mode, Temp, Fan Speed, Swing |

## Ví dụ sử dụng

### 1. Đăng nhập
```bash
curl -X POST http://172.16.64.200:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"vuesp","password":"123456789"}'
```

### 2. Lấy dữ liệu nhiệt độ
```bash
curl -X GET "http://172.16.64.200:8080/temperature?naturalId=TEMP_ESP32_01" \
  -H "Authorization: Bearer <token>"
```

### 3. Lấy toàn bộ dữ liệu cảm biến
```bash
curl -X GET http://172.16.64.200:8080/telemetry \
  -H "Authorization: Bearer <token>"
```

### 4. Điều khiển AC
```bash
curl -X POST http://172.16.64.200:8080/control \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{
    "naturalId":"LABAC1",
    "category":"AIR_CONDITION",
    "power":"ON",
    "temperature":24,
    "mode":"COOL"
  }'
```

## Endpoint Map

```
/auth/login                     (POST)   - Đăng nhập
/temperature                   (GET)    - Lấy nhiệt độ
/humidity                      (GET)    - Lấy độ ẩm
/telemetry                     (GET)    - Lấy dữ liệu toàn hệ thống
/control                       (POST)   - Điều khiển thiết bị
/setup                         (GET)    - Lấy cấu hình
```

## Cấu trúc Config

Device config được lưu trong `config` string trong firmware. Mỗi device có cấu trúc:

```json
{
  "naturalId": "unique identifier",
  "category": "TEMPERATURE|HUMIDITY|LIGHT|FAN|AIR_CONDITION",
  "internal": {
    "module": "DS18B20|SCD40|...",
    "peripheralType": "SENSOR|IR_SENDER|..."
  },
  "gpioPin": [pin numbers]
}
```

Xem chi tiết tại [Setup API](./setup.md).
