# Setup API

## GET `/setup` - Lấy cấu hình thiết bị

API này dùng để lấy toàn bộ cấu hình thiết bị hiện tại từ firmware.

### Request

```bash
curl -X GET "http://172.16.64.200:8080/setup" \
  -H "Authorization: Bearer <your_jwt_token>"
```

### Response

**Success (200 OK):**
```json
{
  "status": 200,
  "message": "Lấy JSON cấu hình thiết bị thành công",
  "data": {
    "timeApi": "http://172.16.64.195:1234/time",
    "roomCode": "R-VU",
    "I2C": {
      "SDA": 21,
      "SCL": 22
    },
    "devices": [
      {
        "naturalId": "TEMP_ESP32_01",
        "category": "TEMPERATURE",
        "gpioPin": [4],
        "internal": {
          "peripheralType": "SENSOR",
          "module": "DS18B20"
        }
      },
      {
        "naturalId": "HUM_ESP32_01",
        "category": "HUMIDITY",
        "gpioPin": [21, 22],
        "internal": {
          "peripheralType": "SENSOR",
          "module": "SCD40"
        }
      },
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
            }
          }
        }
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

**Error (500 Internal Server Error):**
```json
{
  "status": 500,
  "message": "Lỗi thiết bị khi parse JSON",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

### Response Fields

| Tên trường | Loại | Mô tả |
|-----------|------|-------|
| timeApi | string | URL API đồng bộ thời gian |
| roomCode | string | Mã phòng |
| I2C | object | Cấu hình I2C (SDA, SCL pins) |
| devices | array | Mảng tất cả thiết bị được cấu hình |

### Device Config Structure

Mỗi device trong mảng `devices` có cấu trúc:

```json
{
  "naturalId": "string (unique identifier)",
  "category": "string (TEMPERATURE, HUMIDITY, LIGHT, FAN, AIR_CONDITION, ...)",
  "specificType": "string (GPIO, IR_SEND, ...)",
  "controlType": "string (GPIO, IR, ...)",
  "gpioPin": [integer array (GPIO pins if applicable)],
  "translations": {
    "vi": {
      "name": "string",
      "description": "string"
    },
    "en": {
      "name": "string",
      "description": "string"
    }
  },
  "internal": {
    "peripheralType": "string (SENSOR, IR_SENDER, RELAY, ...)",
    "module": "string (DS18B20, SCD40, SHT40, ...)",
    "brand": "string (LG, SAMSUNG, ... for AC)",
    "codeConfigs": {
      "object (IR codes for AC devices)"
    }
  }
}
```

### Chú ý

- Token phải được gửi trong header `Authorization: Bearer <token>`
- Cấu hình lưu trữ **trong RAM**, không có persistent storage
- Nếu firmware restart, cấu hình sẽ được load từ hardcoded `config` string trong code
- Để thay đổi cấu hình, cần edit file firmware và upload lại
- Endpoint này chỉ dùng để **xem** cấu hình, không có endpoint PATCH/PUT để sửa

### Ví dụ sử dụng

**JavaScript/Fetch:**
```javascript
const response = await fetch('http://172.16.64.200:8080/setup', {
  method: 'GET',
  headers: {
    'Authorization': `Bearer ${token}`
  }
});

const data = await response.json();
console.log('Room:', data.data.roomCode);
console.log('Total devices:', data.data.devices.length);
data.data.devices.forEach(device => {
  console.log(`- ${device.naturalId} (${device.category})`);
});
```

**Python:**
```python
import requests
import json

headers = {
    'Authorization': f'Bearer {token}'
}

response = requests.get(
    'http://172.16.64.200:8080/setup',
    headers=headers
)

data = response.json()
if response.status_code == 200:
    config = data['data']
    print(f"Room: {config['roomCode']}")
    print(f"Total devices: {len(config['devices'])}")
    for device in config['devices']:
        print(f"  - {device['naturalId']} ({device['category']})")
else:
    print(f"Error: {data['message']}")
```
