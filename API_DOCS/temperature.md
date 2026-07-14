# Temperature API

## GET `/temperature` - Lấy dữ liệu nhiệt độ từ cảm biến

API này dùng để lấy dữ liệu nhiệt độ hiện tại từ cảm biến được chỉ định.

**Hỗ trợ:** DS18B20 (1-Wire), SCD40 (I2C)

### Request

```bash
curl -X GET "http://172.16.64.200:8080/temperature?naturalId=TEMP_ESP32_01" \
  -H "Authorization: Bearer <your_jwt_token>"
```

**Query Parameters:**
- `naturalId` (required): Định danh cảm biến nhiệt độ

### Response

**Success (200 OK):**
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

**Error (401 Unauthorized):**
```json
{
  "status": 401,
  "message": "Token hết hạn hoặc không đúng",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

**Error (404 Not Found):**
```json
{
  "status": 404,
  "message": "Không tìm thấy cảm biến nhiệt độ có naturalId tương ứng",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

**Error (500 Internal Server Error):**
```json
{
  "status": 500,
  "message": "Lỗi: Không thể đọc dữ liệu từ cảm biến SCD40",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

### Response Fields

| Tên trường | Loại | Mô tả |
|-----------|------|-------|
| tempC | string | Giá trị nhiệt độ theo độ Celsius (3 chữ số thập phân) |

### Cảm biến được hỗ trợ

| Module | Giao tiếp | Phạm vi | Độ chính xác |
|--------|----------|--------|-------------|
| DS18B20 | 1-Wire | -55°C đến 125°C | ±0.5°C |
| SCD40 | I2C (0x62) | -40°C đến 85°C | ±1.5°C |

### Device Config Example

**DS18B20:**
```json
{
  "naturalId": "TEMP_ESP32_01",
  "category": "TEMPERATURE",
  "gpioPin": [4],
  "internal": {
    "peripheralType": "SENSOR",
    "module": "DS18B20"
  }
}
```

**SCD40:**
```json
{
  "naturalId": "TEMP_ESP32_02",
  "category": "TEMPERATURE",
  "gpioPin": [21, 22],
  "internal": {
    "peripheralType": "SENSOR",
    "module": "SCD40"
  }
}
```

### Lưu ý

- Token phải được gửi trong header `Authorization: Bearer <token>`
- Token có hiệu lực mặc định 1 giờ (3600 giây)
- DS18B20 cần pull-up resistor 4.7kΩ giữa data line và VCC
- SCD40 cần kết nối I2C (SDA, SCL) và độ trễ ~5 giây khi khởi động lần đầu
