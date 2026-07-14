# Humidity API

## GET `/humidity` - Lấy dữ liệu độ ẩm từ cảm biến

API này dùng để lấy dữ liệu độ ẩm hiện tại từ cảm biến được chỉ định.

**Hỗ trợ:** SCD40 (I2C)

### Request

```bash
curl -X GET "http://172.16.64.200:8080/humidity?naturalId=HUM_ESP32_01" \
  -H "Authorization: Bearer <your_jwt_token>"
```

**Query Parameters:**
- `naturalId` (required): Định danh cảm biến độ ẩm

### Response

**Success (200 OK):**
```json
{
  "status": 200,
  "message": "Lấy độ ẩm thành công",
  "data": {
    "humidity": "45.200"
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
  "message": "Không tìm thấy cảm biến độ ẩm có naturalId tương ứng",
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
| humidity | string | Giá trị độ ẩm tương đối (%) (2 chữ số thập phân) |

### Cảm biến được hỗ trợ

| Module | Giao tiếp | Phạm vi | Độ chính xác |
|--------|----------|--------|-------------|
| SCD40 | I2C (0x62) | 0% - 100% RH | ±3% RH |

### Device Config Example

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

### Lưu ý

- Token phải được gửi trong header `Authorization: Bearer <token>`
- Token có hiệu lực mặc định 1 giờ (3600 giây)
- SCD40 cần kết nối I2C (SDA, SCL)
- Độ trễ ~5 giây khi khởi động lần đầu, sau đó ~1 giây cho mỗi lần đọc
- SCD40 đo độ ẩm cùng lúc với nhiệt độ và CO2
