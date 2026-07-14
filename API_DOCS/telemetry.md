# Telemetry API

## GET `/telemetry` - Lấy dữ liệu cảm biến từ tất cả các thiết bị

API này dùng để lấy dữ liệu từ tất cả các cảm biến nhiệt độ được cấu hình trong hệ thống.

### Request

```bash
curl -X GET "http://172.16.64.200:8080/telemetry" \
  -H "Authorization: Bearer <your_jwt_token>"
```

### Response

**Success (200 OK):**
```json
{
  "status": 200,
  "message": "Lấy dữ liệu telemetry thành công",
  "data": {
    "roomCode": "R-VU",
    "devices": [
      {
        "naturalId": "TEMP_ESP32_01",
        "category": "TEMPERATURE",
        "tempC": "25.500"
      },
      {
        "naturalId": "HUM_ESP32_01",
        "category": "HUMIDITY",
        "humidity": "45.200"
      },
      {
        "naturalId": "TEMP_ESP32_02",
        "category": "TEMPERATURE",
        "tempC": "26.300"
      }
    ]
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

### Response Fields

| Tên trường | Loại | Mô tả |
|-----------|------|-------|
| roomCode | string | Mã phòng từ config |
| devices | array | Mảng các cảm biến |
| devices[].naturalId | string | Định danh thiết bị |
| devices[].category | string | Loại sensor (TEMPERATURE, HUMIDITY) |
| devices[].tempC | string | Giá trị nhiệt độ (chỉ có nếu category là TEMPERATURE) |
| devices[].humidity | string | Giá trị độ ẩm (chỉ có nếu category là HUMIDITY) |

### Chú ý

- Endpoint này quét tất cả devices có `category: "TEMPERATURE"` hoặc `category: "HUMIDITY"` trong config
- Nếu không thể đọc dữ liệu từ cảm biến nào, device đó sẽ **bị bỏ qua** trong response (không trả về lỗi)
- Token phải được gửi trong header `Authorization: Bearer <token>`
- Thời gian response phụ thuộc vào số lượng cảm biến và trạng thái của chúng

### Cảm biến được hỗ trợ

- TEMPERATURE: DS18B20, SCD40
- HUMIDITY: SCD40

### Ví dụ sử dụng

**JavaScript/Fetch:**
```javascript
const response = await fetch('http://172.16.64.200:8080/telemetry', {
  method: 'GET',
  headers: {
    'Authorization': `Bearer ${token}`
  }
});

const data = await response.json();
console.log('Room:', data.data.roomCode);
data.data.devices.forEach(device => {
  if (device.category === 'TEMPERATURE') {
    console.log(`${device.naturalId}: ${device.tempC}°C`);
  } else if (device.category === 'HUMIDITY') {
    console.log(`${device.naturalId}: ${device.humidity}% RH`);
  }
});
```

**Python:**
```python
import requests

headers = {
    'Authorization': f'Bearer {token}'
}

response = requests.get(
    'http://172.16.64.200:8080/telemetry',
    headers=headers
)

data = response.json()
if response.status_code == 200:
    print(f"Room: {data['data']['roomCode']}")
    for device in data['data']['devices']:
        if device['category'] == 'TEMPERATURE':
            print(f"{device['naturalId']}: {device['tempC']}°C")
        elif device['category'] == 'HUMIDITY':
            print(f"{device['naturalId']}: {device['humidity']}% RH")
else:
    print(f"Error: {data['message']}")
```
