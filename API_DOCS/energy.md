# Energy Meter API

## Overview

API này dùng để lấy dữ liệu tiêu thụ năng lượng hiện tại từ các thiết bị trong phòng:
- **POWER_CONSUMPTION**: Công tơ điện tổng phòng (PZEM004Tv30 chính)
- **LIGHT, FAN, AIR_CONDITION**: Đo năng lượng riêng cho từng thiết bị (nếu có cấu hình `consumption.power.gpioPin`)

**Hỗ trợ:** PZEM004Tv30 (Modbus Serial) - Hiệu suất cao, độ chính xác cao

---

## GET `/devices/telemetry` - Lấy dữ liệu năng lượng

### Request

```bash
curl -X GET "http://172.16.64.200:8080/devices/telemetry?deviceCategory=POWER_CONSUMPTION&naturalId=POWER_001" \
  -H "Authorization: Bearer <your_jwt_token>"
```

**Query Parameters:**
| Tham số | Bắt buộc | Loại | Mô tả |
|---------|----------|------|-------|
| `deviceCategory` | Có | String | Loại thiết bị: `POWER_CONSUMPTION`, `LIGHT`, `FAN`, `AIR_CONDITION` |
| `naturalId` | Có | String | Định danh thiết bị duy nhất (VD: `POWER_001`, `FAN_01`, `LIGHT_01`) |

### Response - Success (200 OK)

```json
{
  "status": 200,
  "message": "Success",
  "timestamp": "2026-08-08T15:30:00Z",
  "data": {
    "timestamp": "2026-08-08T15:30:00Z",
    "voltage": 220.5,
    "current": 0.45,
    "power": 99.2,
    "energy": 45.32,
    "frequency": 50.0,
    "powerFactor": 0.95
  }
}
```

### Response Fields

| Tên trường | Loại | Đơn vị | Phạm vi | Mô tả |
|-----------|------|--------|---------|-------|
| timestamp | string | ISO-8601 | - | Thời gian ghi nhận chỉ số |
| voltage | number | V (Volt) | 0-300 | Điện áp dòng điện |
| current | number | A (Ampere) | 0-100 | Cường độ dòng điện |
| power | number | W (Watt) | 0-50000 | Công suất tiêu thụ tức thời |
| energy | number | kWh | 0-9999.99 | Tổng điện năng tiêu thụ tích lũy |
| frequency | number | Hz (Hertz) | 45-65 | Tần số dòng điện AC |
| powerFactor | number | - | 0.00-1.00 | Hệ số công suất (cos φ) |

### Response - Error Cases

**Error (400 Bad Request):**
```json
{
  "status": 400,
  "message": "Query parameters required: ?deviceCategory=...&naturalId=...",
  "timestamp": "2026-08-08T15:30:00Z"
}
```

**Error (401 Unauthorized):**
```json
{
  "status": 401,
  "message": "Token hết hạn hoặc không đúng",
  "timestamp": "2026-08-08T15:30:00Z"
}
```

**Error (404 Not Found):**
```json
{
  "status": 404,
  "message": "Không tìm thấy thiết bị có naturalId tương ứng",
  "timestamp": "2026-08-08T15:30:00Z"
}
```

**Error (500 Internal Server Error):**
```json
{
  "status": 500,
  "message": "Không thể đọc dữ liệu từ PZEM của thiết bị",
  "timestamp": "2026-08-08T15:30:00Z"
}
```

### Examples by Device Category

#### 1. POWER_CONSUMPTION - Công tơ điện tổng phòng

```bash
curl -H "Authorization: Bearer <JWT_TOKEN>" \
  "http://172.16.64.200:8080/devices/telemetry?deviceCategory=POWER_CONSUMPTION&naturalId=POWER_001"
```

**Response:**
```json
{
  "status": 200,
  "message": "Success",
  "timestamp": "2026-08-08T15:30:00Z",
  "data": {
    "timestamp": "2026-08-08T15:30:00Z",
    "voltage": 221.2,
    "current": 6.8,
    "power": 1504.16,
    "energy": 120.45,
    "frequency": 50.0,
    "powerFactor": 0.96
  }
}
```

#### 2. FAN - Đèn

```bash
curl -H "Authorization: Bearer <JWT_TOKEN>" \
  "http://172.16.64.200:8080/devices/telemetry?deviceCategory=LIGHT&naturalId=LIGHT_01"
```

**Response:**
```json
{
  "status": 200,
  "message": "Success",
  "timestamp": "2026-08-08T15:30:00Z",
  "data": {
    "timestamp": "2026-08-08T15:30:00Z",
    "voltage": 220.5,
    "current": 0.15,
    "power": 33.0,
    "energy": 1.25,
    "frequency": 50.0,
    "powerFactor": 0.95
  }
}
```

#### 3. FAN - Quạt

```bash
curl -H "Authorization: Bearer <JWT_TOKEN>" \
  "http://172.16.64.200:8080/devices/telemetry?deviceCategory=FAN&naturalId=FAN_01"
```

**Response:**
```json
{
  "status": 200,
  "message": "Success",
  "timestamp": "2026-08-08T15:30:00Z",
  "data": {
    "timestamp": "2026-08-08T15:30:00Z",
    "voltage": 220.0,
    "current": 0.25,
    "power": 55.0,
    "energy": 3.42,
    "frequency": 50.0,
    "powerFactor": 0.98
  }
}
```

#### 4. AIR_CONDITION - Điều hòa

```bash
curl -H "Authorization: Bearer <JWT_TOKEN>" \
  "http://172.16.64.200:8080/devices/telemetry?deviceCategory=AIR_CONDITION&naturalId=AC_LAB_01"
```

**Response:**
```json
{
  "status": 200,
  "message": "Success",
  "timestamp": "2026-08-08T15:30:00Z",
  "data": {
    "timestamp": "2026-08-08T15:30:00Z",
    "voltage": 219.8,
    "current": 4.5,
    "power": 989.1,
    "energy": 24.85,
    "frequency": 50.0,
    "powerFactor": 0.92
  }
}
```

---

## POST `/devices/reset-energy` - Reset bộ đếm năng lượng

API này dùng để reset bộ đếm tích lũy năng lượng trên công tơ điện hoặc thiết bị đo.

### Request

```bash
curl -X POST "http://172.16.64.200:8080/devices/reset-energy" \
  -H "Authorization: Bearer <your_jwt_token>" \
  -H "Content-Type: application/json" \
  -d '{
    "deviceCategory": "POWER_CONSUMPTION",
    "naturalId": "POWER_001"
  }'
```

**Request Body:**
```json
{
  "deviceCategory": "POWER_CONSUMPTION",
  "naturalId": "POWER_001"
}
```

### Response - Success (200 OK)

```json
{
  "status": 200,
  "message": "Energy metric reset successfully",
  "data": "OK",
  "timestamp": "2026-08-08T15:35:00Z"
}
```

### Response - Error Cases

**Error (400 Bad Request):**
```json
{
  "status": 400,
  "message": "Body required: {\"deviceCategory\": \"...\", \"naturalId\": \"...\"}",
  "timestamp": "2026-08-08T15:35:00Z"
}
```

**Error (401 Unauthorized):**
```json
{
  "status": 401,
  "message": "Token hết hạn hoặc không đúng",
  "timestamp": "2026-08-08T15:35:00Z"
}
```

**Error (500 Internal Server Error):**
```json
{
  "status": 500,
  "message": "Lỗi: Không thể reset năng lượng",
  "timestamp": "2026-08-08T15:35:00Z"
}
```

### Examples

#### Reset tổng phòng

```bash
curl -X POST \
  -H "Authorization: Bearer <JWT_TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{"deviceCategory":"POWER_CONSUMPTION","naturalId":"POWER_001"}' \
  http://172.16.64.200:8080/devices/reset-energy
```

#### Reset một device cụ thể

```bash
curl -X POST \
  -H "Authorization: Bearer <JWT_TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{"deviceCategory":"FAN","naturalId":"FAN_01"}' \
  http://172.16.64.200:8080/devices/reset-energy
```

---

## Device Configuration

### Công tơ điện tổng (POWER_CONSUMPTION)

```json
{
  "naturalId": "POWER_001",
  "category": "POWER_CONSUMPTION",
  "specificType": "PZEM",
  "controlType": "GPIO",
  "gpioPin": [16, 17],
  "name": "Power Meter",
  "translations": {
    "vi": {
      "name": "Đo năng lượng",
      "description": "PZEM004T v3.0"
    },
    "en": {
      "name": "Energy Meter",
      "description": "PZEM004T v3.0"
    }
  },
  "isActive": true
}
```

### Thiết bị riêng lẻ với đo năng lượng (LIGHT, FAN, AIR_CONDITION)

```json
{
  "naturalId": "FAN_01",
  "category": "FAN",
  "specificType": "GPIO",
  "controlType": "GPIO",
  "gpioPin": [14],
  "consumption": {
    "power": {
      "gpioPin": [16, 17]
    }
  },
  "translations": {
    "vi": {
      "name": "Quạt Lab",
      "description": "Quạt 3 tốc độ"
    },
    "en": {
      "name": "Lab Fan",
      "description": "3-speed fan"
    }
  },
  "internal": {
    "peripheralType": "RELAY"
  }
}
```

**Chú ý:**
- `gpioPin[0]` = Chân điều khiển thiết bị (relay)
- `consumption.power.gpioPin[0]` = GPIO RX của PZEM
- `consumption.power.gpioPin[1]` = GPIO TX của PZEM

---

## Hardware Configuration

### Wiring Diagram

```
┌─────────────────┐        ┌──────────────────┐
│     ESP32       │        │  PZEM004Tv30     │
├─────────────────┤        ├──────────────────┤
│ GPIO 16 (RX2)   │───────→│ TX (Transmit)    │
│ GPIO 17 (TX2)   │←───────│ RX (Receive)     │
│ GND             │───────→│ GND              │
│ 5V              │───────→│ VCC              │
└─────────────────┘        └──────────────────┘
```

### Thông số kỹ thuật

| Tính chất | Giá trị | Ghi chú |
|-----------|--------|--------|
| Module | PZEM004Tv30 | Modbus RTU Serial |
| Giao tiếp | Serial (UART) | Baud rate 9600 |
| GPIO RX | 16 (ESP32 RX2) | Nhận từ PZEM TX |
| GPIO TX | 17 (ESP32 TX2) | Truyền tới PZEM RX |
| Địa chỉ Slave | 0x01 | Mặc định PZEM |
| Điện áp đo | 0-300V | AC |
| Dòng đo | 0-100A | AC |
| Công suất | 0-50000W | |
| Độ chính xác | ±0.5% | Hiệu suất cao |

### Cảnh báo quan trọng

⚠️ **GND phải kết nối** giữa ESP32 và PZEM  
⚠️ **Baud rate**: 9600 bps, 8 data bits, No parity, 1 stop bit  
⚠️ Nếu PZEM sử dụng 5V, hãy dùng **level shifter** để chuyển đổi từ 3.3V ESP32  
⚠️ **Kết nối đúng RX/TX** - Tránh hoán đổi RX và TX

---

## Performance & Limitations

| Tính chất | Giá trị | Ghi chú |
|-----------|--------|--------|
| Update Interval | 30 giây | Cho POWER_CONSUMPTION (global) |
| Device-specific Response Time | <100ms | Đọc dữ liệu tức thì từ GPIO |
| Memory Usage | ~100 bytes/device | Cho PZEM cached data |
| Baud Rate | 9600 bps | Tốc độ truyền serial |
| Max Devices | Unlimited | Tùy thuộc vào số GPIO pins |

---

## Lưu ý

- Token phải được gửi trong header `Authorization: Bearer <token>`
- Token có hiệu lực mặc định 1 giờ (3600 giây)
- Lần đầu gọi API có thể chờ tới 30 giây nếu dữ liệu chưa sẵn sàng
- Nếu nhận lỗi 500, hãy chờ khoảng 1 phút rồi thử lại
- Reset năng lượng sẽ **xóa bộ đếm tích lũy** (không thể khôi phục)
- Để đo năng lượng cho các device riêng lẻ, phải cấu hình `consumption.power.gpioPin` trong config

---

## Troubleshooting

| Vấn đề | Nguyên nhân | Giải pháp |
|--------|------------|----------|
| Status 500 "Không thể đọc dữ liệu" | UART lỗi | Kiểm tra kết nối GPIO 16/17 |
| NaN readings | Lỗi Modbus | Kiểm tra cổng serial, baud rate |
| Device không found | Config sai | Kiểm tra naturalId đúng chưa |
| Token không hợp lệ | Token hết hạn | Lấy token mới từ `/auth/login` |
| PZEM không phản hồi | Kết nối vật lý | Kiểm tra GND, power, RX/TX |

