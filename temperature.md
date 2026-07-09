# Temperature Module - SmartRoom Client

## Temperature API Documentation

---

### **POST** `/temperature` - Lấy dữ liệu nhiệt độ từ cảm biến

> API này dùng để lấy dữ liệu nhiệt độ hiện tại từ cảm biến được chỉ định.
> 
> Hiện tại chỉ hỗ trợ cảm biến **DS18B20** sử dụng giao tiếp 1-Wire.
> 
> Request bắt buộc phải có JWT token hợp lệ trong header `Authorization`.

### Request Headers

| Tên header | Giá trị | Bắt buộc | Mô tả |
| :--------- | :------ | :------- | :---- |
| Content-Type | application/json | Có | Định dạng dữ liệu gửi lên |
| Authorization | Bearer <token> | Có | JWT token lấy từ API đăng nhập |
| Origin | string | Không | Nguồn gốc request (hỗ trợ CORS) |

### Request Body

| Tên trường | Loại | Bắt buộc | Mô tả |
| :--------- | :--- | :------- | :---- |
| naturalId | string | Có | Định danh cảm biến nhiệt độ |

### Request Example

```json
{
	"naturalId": "TEMP_ESP32_01"
}
```

### Response (200 OK)

```json
{
	"status": 200,
	"message": "Lấy nhiệt độ thành công",
	"data": {
		"tempC": "25.500"
	},
	"timestamp": "2026-07-07T03:28:27Z"
}
```

### Response Fields

| Tên trường | Loại | Mô tả |
| :--------- | :--- | :---- |
| tempC | string | Giá trị nhiệt độ theo độ Celsius (3 chữ số thập phân) |

---

## Lỗi chung

### Response (400 Bad Request)

> Xảy ra khi body trống hoặc thiếu trường `naturalId`.

```json
{
	"status": 400,
	"message": "Body bắt buộc phải có trường: naturalId",
	"timestamp": "2026-07-07T03:28:27Z"
}
```

### Response (401 Unauthorized)

> Xảy ra khi thiếu header Authorization, token không đúng định dạng, hoặc token không hợp lệ/hết hạn.

```json
{
	"status": 401,
	"message": "Token hết hạn hoặc không đúng",
	"timestamp": "2026-07-07T03:28:27Z"
}
```

### Response (404 Not Found)

> Xảy ra khi không tìm thấy cảm biến có `naturalId` tương ứng trong config.

```json
{
	"status": 404,
	"message": "Không tìm thấy cảm biến nhiệt độ có naturalId tương ứng",
	"timestamp": "2026-07-07T03:28:27Z"
}
```

### Response (500 Internal Server Error)

> Xảy ra khi:
> - Cảm biến thiếu thông tin cấu hình (module, GPIO pin)
> - Không thể đọc dữ liệu từ cảm biến (cảm biến không phản hồi)
> - Lỗi khi parse JSON cấu hình

```json
{
	"status": 500,
	"message": "Lỗi: Không thể đọc dữ liệu từ cảm biến",
	"timestamp": "2026-07-07T03:28:27Z"
}
```

### Response (501 Not Implemented)

> Xảy ra khi module cảm biến chưa được hỗ trợ (không phải DS18B20).

```json
{
	"status": 501,
	"message": "Module cảm biến chưa được hỗ trợ",
	"timestamp": "2026-07-07T03:28:27Z"
}
```

---

## Cấu hình Cảm biến Nhiệt độ

### Module được hỗ trợ

Hiện tại chỉ hỗ trợ:
- **DS18B20**: Cảm biến nhiệt độ 1-Wire, phạm vi -55-125°C, độ chính xác ±0.5°C

Các module khác (DHT11, DHT22, BME680, ...) sẽ trả về lỗi 501 Not Implemented.

### Cấu hình Cảm biến trong Device Config

```json
{
	"naturalId": "TEMP_ESP32_01",
	"category": "TEMPERATURE",
	"specificType": "GPIO",
	"controlType": "GPIO",
	"gpioPin": [4],
	"translations": {
		"vi": {
			"name": "Cảm biến nhiệt độ A101 1",
			"description": "Cảm biến nhiệt độ DS18B20 phòng học A101 sử dụng chuẩn giao tiếp 1-Wire"
		},
		"en": {
			"name": "Temperature Sensor A101 1",
			"description": "DS18B20 temperature sensor in room A101 using 1-Wire protocol"
		}
	},
	"internal": {
		"peripheralType": "SENSOR",
		"module": "DS18B20"
	}
}
```

### Cấu trúc Internal

```json
{
	"internal": {
		"peripheralType": "SENSOR",
		"module": "DS18B20"
	}
}
```

### Các tham số cấu hình

| Tham số | Loại | Bắt buộc | Giá trị | Mô tả |
| :------ | :--- | :------- | :------ | :---- |
| peripheralType | string | Có | `SENSOR` | Loại thiết bị ngoại vi |
| module | string | Có | `DS18B20` | Module cảm biến được sử dụng |
| gpioPin | array | Có | `[pin_number]` | GPIO pin kết nối với cảm biến (phần tử đầu tiên được sử dụng) |

---

## Lưu ý kỹ thuật

### DS18B20 (1-Wire Protocol)

- **Giao tiếp**: 1-Wire (giao tiếp đơn dây)
- **Phạm vi nhiệt độ**: -55°C đến 125°C
- **Độ chính xác**: ±0.5°C
- **Độ phân giải**: 0.0625°C
- **Thời gian đọc**: ~750ms mỗi lần (đã bao gồm trong hàm `readDS18B20Temperature()`)

### Xử lý lỗi

- Nếu cảm biến không phản hồi (trả về `DEVICE_DISCONNECTED_C`), firmware sẽ ghi log lỗi và trả về mã lỗi 500
- Kiểm tra kết nối vật lý (pull-up resistor 4.7kΩ giữa data line và VCC)
- Đảm bảo GPIO pin được chỉ định chính xác trong config

### Độ phân giải dữ liệu

- Nhiệt độ trả về với **3 chữ số thập phân** (ví dụ: "25.500")
- Định dạng là chuỗi JSON (không phải số)

### JWT Token

- Token phải được gửi qua header `Authorization: Bearer <token>`
- Token có hiệu lực mặc định 3600 giây (1 giờ)
- Nếu token hết hạn hoặc không đúng, API sẽ trả về lỗi 401

---

## Ví dụ sử dụng

### cURL

```bash
curl -X POST http://192.168.1.200:8080/temperature \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <jwt_token>" \
  -d '{"naturalId": "TEMP_ESP32_01"}'
```

### JavaScript/Fetch

```javascript
const response = await fetch('http://192.168.1.200:8080/temperature', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Authorization': `Bearer ${token}`
  },
  body: JSON.stringify({
    naturalId: 'TEMP_ESP32_01'
  })
});

const data = await response.json();
console.log('Nhiệt độ:', data.data.tempC, '°C');
```

### Python

```python
import requests

headers = {
    'Content-Type': 'application/json',
    'Authorization': f'Bearer {token}'
}

payload = {
    'naturalId': 'TEMP_ESP32_01'
}

response = requests.post(
    'http://192.168.1.200:8080/temperature',
    json=payload,
    headers=headers
)

data = response.json()
if response.status_code == 200:
    print(f"Nhiệt độ: {data['data']['tempC']}°C")
else:
    print(f"Lỗi: {data['message']}")
```

---

## Ghi chú

- API hiện tại chỉ hỗ trợ **một cảm biến DS18B20** trên mỗi thiết bị
- Có thể mở rộng để hỗ trợ nhiều cảm biến khác nhau bằng cách thêm logic xử lý module mới
- Độ trễ đọc dữ liệu từ DS18B20 là khoảng 750ms
- GPIO pin phải được cấu hình đúng trong device config để tránh lỗi
- Nếu cảm biến không phản hồi, kiểm tra kết nối vật lý và pull-up resistor

