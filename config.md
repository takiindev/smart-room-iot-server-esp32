# Config Module - SmartRoom Client

## Config API Documentation

---

### **GET** `/setup` - Lấy JSON cấu hình thiết bị hiện tại

> API này dùng để lấy toàn bộ JSON cấu hình thiết bị đang được firmware sử dụng.
> 
> Request bắt buộc phải có JWT token hợp lệ trong header `Authorization`.

### Request Headers

| Tên header | Giá trị | Bắt buộc | Mô tả |
| :--------- | :------ | :------- | :---- |
| Authorization | Bearer <token> | Có | JWT token lấy từ API đăng nhập |
| Origin | string | Không | Nguồn gốc request (hỗ trợ CORS) |

### Response (200 OK)

```json
{
	"status": 200,
	"message": "Lấy JSON cấu hình thiết bị thành công",
	"data": {
		"roomCode": "R-VU",
		"devices": [
			{
				"naturalId": "LABAC1",
				"category": "AIR_CONDITION",
				"controlType": "GPIO",
				"specificType": "IR_SEND",
				"gpioPin": [18],
				"translations": {
					"en": { "name": "Lab AC 1" },
					"vi": { "name": "Máy lạnh phòng lab 1" }
				},
				"internal": {
					"peripheralType": "IR_SENDER",
					"brand": "LG",
					"codeConfigs": {
						"bits": 28,
						"power": { "ON": "0x20DF10EF", "OFF": "0x20DF906F" },
						"mode": { "COOL": "0x20DF40BF", "HEAT": "0x20DFC03F", "DRY": "0x20DF00FF", "FAN": "0x20DF807F", "AUTO": "0x20DFA05F" },
						"speed": { "1": "0x20DF30CF", "2": "0x20DFB04F", "3": "0x20DF708F", "4": "0x20DFF00F", "5": "0x20DF50AF" },
						"temperature": { "16": "0x20DF906F", "17": "0x20DF10EF", ... },
						"swing": { "ON": "0x20DFA05F", "OFF": "0x20DFC03F" }
					}
				}
			},
			{
				"naturalId": "LIGHT_01",
				"category": "LIGHT",
				"translations": {
					"vi": { "name": "Đèn A101 1", "description": "Đèn số 01 của phòng lab A101" },
					"en": { "name": "Light A101 1", "description": "Light 01 of Lab A101" }
				},
				"specificType": "GPIO",
				"controlType": "GPIO",
				"gpioPin": [13],
				"internal": { "peripheralType": "RELAY" }
			},
			{
				"naturalId": "Fan_01",
				"category": "FAN",
				"translations": {
					"vi": { "name": "Quạt A101 1", "description": "Quạt số 01 của phòng lab A101" },
					"en": { "name": "Fan A101 1", "description": "Fan 01 of Lab A101" }
				},
				"specificType": "GPIO",
				"controlType": "GPIO",
				"gpioPin": [14, 27, 26],
				"internal": { "peripheralType": "RELAY" }
			}
		]
	},
	"timestamp": "2026-07-07T03:28:27Z"
}
```

### Response (401 Unauthorized)

```json
{
	"status": 401,
	"message": "Token hết hạn hoặc không đúng",
	"timestamp": "2026-07-07T03:28:27Z"
}
```

---

### **PATCH** `/config` - Cập nhật JSON cấu hình thiết bị

> API này dùng để cập nhật toàn bộ JSON cấu hình thiết bị đang được firmware sử dụng trong bộ nhớ.
> 
> Dữ liệu mới sẽ được ghi đè lên cấu hình hiện tại và áp dụng lại cho các relay sau khi lưu thành công.
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
| roomCode | string | Có | Mã phòng |
| devices | array | Có | Mảng JSON cấu hình thiết bị mới |

### Request Example

```json
{
	"roomCode": "R-VU",
	"devices": [
		{
			"naturalId": "LABAC1",
			"category": "AIR_CONDITION",
			"controlType": "GPIO",
			"specificType": "IR_SEND",
			"gpioPin": [18],
			"translations": {
				"en": { "name": "Lab AC 1" },
				"vi": { "name": "Máy lạnh phòng lab 1" }
			},
			"internal": {
				"peripheralType": "IR_SENDER",
				"brand": "LG",
				"codeConfigs": {
					"bits": 28,
					"power": { "ON": "0x20DF10EF", "OFF": "0x20DF906F" },
					"mode": { "COOL": "0x20DF40BF", "HEAT": "0x20DFC03F", "DRY": "0x20DF00FF", "FAN": "0x20DF807F", "AUTO": "0x20DFA05F" },
					"speed": { "1": "0x20DF30CF", "2": "0x20DFB04F", "3": "0x20DF708F", "4": "0x20DFF00F", "5": "0x20DF50AF" },
					"temperature": { "16": "0x20DF906F", "17": "0x20DF10EF", "18": "0x20DF50AF", "19": "0x20DFF00F", "20": "0x20DF708F", "21": "0x20DFC03F", "22": "0x20DF40BF", "23": "0x20DFB04F", "24": "0x20DF30CF", "25": "0x20DFA05F", "26": "0x20DFE01F", "27": "0x20DF609F", "28": "0x20DFA05F", "29": "0x20DFC03F", "30": "0x20DF40BF" },
					"swing": { "ON": "0x20DFA05F", "OFF": "0x20DFC03F" }
				}
			}
		},
		{
			"naturalId": "LIGHT_01",
			"category": "LIGHT",
			"translations": {
				"vi": { "name": "Đèn A101 1", "description": "Đèn số 01 của phòng lab A101" },
				"en": { "name": "Light A101 1", "description": "Light 01 of Lab A101" }
			},
			"specificType": "GPIO",
			"controlType": "GPIO",
			"gpioPin": [13],
			"internal": { "peripheralType": "RELAY" }
		},
		{
			"naturalId": "Fan_01",
			"category": "FAN",
			"translations": {
				"vi": { "name": "Quạt A101 1", "description": "Quạt số 01 của phòng lab A101" },
				"en": { "name": "Fan A101 1", "description": "Fan 01 of Lab A101" }
			},
			"specificType": "GPIO",
			"controlType": "GPIO",
			"gpioPin": [14, 27, 26],
			"internal": { "peripheralType": "RELAY" }
		}
	]
}
```

### Response (200 OK)

```json
{
	"status": 200,
	"message": "Cập nhật JSON cấu hình thiết bị thành công",
	"timestamp": "2026-07-07T03:28:27Z"
}
```

### Response (400 Bad Request)

> Xảy ra khi body trống, JSON không hợp lệ, hoặc thiếu trường `roomCode` hoặc `devices`.

```json
{
	"status": 400,
	"message": "Thiếu trường roomCode hoặc devices",
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

### Cấu trúc Device

#### Thiết bị Relay (LIGHT, FAN)

```json
{
	"naturalId": "LIGHT_01",
	"category": "LIGHT",
	"translations": {
		"vi": { "name": "Tên tiếng Việt", "description": "Mô tả" },
		"en": { "name": "English Name", "description": "Description" }
	},
	"specificType": "GPIO",
	"controlType": "GPIO",
	"gpioPin": [13],
	"internal": {
		"peripheralType": "RELAY"
	}
}
```

#### Thiết bị IR Sender (AIR_CONDITION)

```json
{
	"naturalId": "LABAC1",
	"category": "AIR_CONDITION",
	"controlType": "GPIO",
	"specificType": "IR_SEND",
	"gpioPin": [18],
	"translations": { ... },
	"internal": {
		"peripheralType": "IR_SENDER",
		"brand": "LG",
		"codeConfigs": {
			"bits": 28,
			"power": { "ON": "0x...", "OFF": "0x..." },
			"mode": { "COOL": "0x...", "HEAT": "0x...", ... },
			"speed": { "1": "0x...", "2": "0x...", ... },
			"temperature": { "16": "0x...", "17": "0x...", ... },
			"swing": { "ON": "0x...", "OFF": "0x..." }
		}
	}
}
```

### Lưu ý

- Cấu hình phải có `roomCode` và mảng `devices`
- Mỗi device phải có `internal` object với `peripheralType`
- Thiết bị IR_SENDER phải có `internal.brand` và `internal.codeConfigs` 
- Sau khi cập nhật thành công, firmware sẽ lưu cấu hình mới vào bộ nhớ
- JWT token phải được gửi qua header `Authorization: Bearer <token>`
