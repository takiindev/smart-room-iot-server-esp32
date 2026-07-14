# Control API

## POST `/control` - Điều khiển thiết bị

API này dùng để điều khiển các thiết bị được hỗ trợ (đèn, quạt, điều hòa).

**Hỗ trợ:** LIGHT, FAN, AIR_CONDITION

### Request

```bash
curl -X POST "http://172.16.64.200:8080/control" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <your_jwt_token>" \
  -d '{
    "naturalId": "LABAC1",
    "category": "AIR_CONDITION",
    "power": "ON",
    "temperature": 24,
    "mode": "COOL"
  }'
```

**Body Parameters:**
- `naturalId` (required): Định danh thiết bị
- `category` (required): Loại thiết bị (LIGHT, FAN, AIR_CONDITION)
- Tham số thêm phụ thuộc vào category (xem bên dưới)

### Response

**Success (200 OK):**
```json
{
  "status": 200,
  "message": "Điều khiển thành công",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

**Error (400 Bad Request):**
```json
{
  "status": 400,
  "message": "Body bắt buộc phải có các trường: naturalId, category",
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
  "message": "Không tìm thấy thiết bị có naturalId tương ứng",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

---

## LIGHT - Điều khiển đèn

### Request Example

```json
{
  "naturalId": "LIGHT_01",
  "category": "LIGHT",
  "power": "ON"
}
```

### Parameters

| Tên trường | Loại | Bắt buộc | Giá trị | Mô tả |
|-----------|------|---------|--------|-------|
| power | string | Không | ON, OFF | Bật/tắt đèn |

---

## FAN - Điều khiển quạt

### Request Example - Đặt tốc độ

```json
{
  "naturalId": "FAN_01",
  "category": "FAN",
  "speed": 2
}
```

### Request Example - Bật/tắt

```json
{
  "naturalId": "FAN_01",
  "category": "FAN",
  "power": "ON"
}
```

### Parameters

| Tên trường | Loại | Bắt buộc | Giá trị | Mô tả |
|-----------|------|---------|--------|-------|
| power | string | Không | ON, OFF | Bật/tắt quạt (ON = tốc độ 1) |
| speed | number | Không | 1, 2, 3 | Mức tốc độ quạt |

---

## AIR_CONDITION - Điều khiển điều hòa

Điều khiển AC thông qua gửi mã IR. Hỗ trợ các hãng: LG, SAMSUNG, PANASONIC, ...

### Request Example - Bật AC

```json
{
  "naturalId": "LABAC1",
  "category": "AIR_CONDITION",
  "power": "ON"
}
```

### Request Example - Kết hợp nhiều tham số

```json
{
  "naturalId": "LABAC1",
  "category": "AIR_CONDITION",
  "power": "ON",
  "temperature": 24,
  "mode": "COOL",
  "fanSpeed": 2,
  "swing": "ON"
}
```

### Parameters

| Tên trường | Loại | Bắt buộc | Giá trị | Mô tả |
|-----------|------|---------|--------|-------|
| power | string | Không | ON, OFF | Bật/tắt AC |
| mode | string | Không | COOL, HEAT, DRY, FAN, AUTO | Chế độ AC |
| temperature | number | Không | 16-30 | Nhiệt độ (°C) |
| fanSpeed, speed | number | Không | 1-3 | Tốc độ quạt |
| swing | string | Không | ON, OFF | Lưới tượng |

### Thứ tự xử lý

Khi gửi nhiều tham số, firmware xử lý theo thứ tự:
1. power
2. temperature
3. mode
4. speed/fanSpeed
5. swing

Giữa mỗi lệnh IR có độ trễ ~100ms.

### Device Config Example

```json
{
  "naturalId": "LABAC1",
  "category": "AIR_CONDITION",
  "internal": {
    "peripheralType": "IR_SENDER",
    "brand": "LG",
    "codeConfigs": {
      "power": {
        "ON": "0x8800F43",
        "OFF": "0x88C0051"
      },
      "mode": {
        "COOL": "0x8808F4B",
        "HEAT": "0x880C556",
        "DRY": "0x880910A",
        "FAN": "0x880A30D",
        "AUTO": "0x880B151"
      },
      "temperature": {
        "16": "0x880A14F",
        "17": "0x880A163",
        "24": "0x880A267",
        "30": "0x880A267"
      },
      "speed": {
        "1": "0x880A30D",
        "2": "0x880A32F",
        "3": "0x880A341"
      },
      "swing": {
        "ON": "0x8810001"
      }
    }
  }
}
```

### Chú ý

- IR code phải là chuỗi hex bắt đầu với `0x`
- Mỗi hãng AC có số bit khác nhau (LG: 28 bits, Samsung: 36 bits, Panasonic: 48 bits)
- Nếu thiếu một tham số trong `codeConfigs`, lệnh đó sẽ báo lỗi khi cố thực hiện
- Có thể mở rộng hỗ trợ hãng mới bằng cách thêm vào firmware
- Token phải được gửi trong header `Authorization: Bearer <token>`
