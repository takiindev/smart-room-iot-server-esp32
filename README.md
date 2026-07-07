# SmartRoom IoT Server - ESP32 Firmware Documentation

Tài liệu hoàn chỉnh cho firmware điều khiển thiết bị smart home trên ESP32.

## 📋 Danh sách tài liệu

1. **[auth.md](auth.md)** - API đăng nhập và quản lý JWT token
2. **[config.md](config.md)** - API lấy và cập nhật cấu hình thiết bị
3. **[control.md](control.md)** - API điều khiển thiết bị (LIGHT, FAN, AIR_CONDITION)

## 🔧 Cấu trúc dự án

```
SmartRoom-Client/
├── SmartRoom-Client.ino          # Firmware chính
├── config.json                    # File cấu hình thiết bị
├── setup.json                     # File cấu hình WiFi & Server
├── SERIAL_LOGGING_GUIDE.md       # Hướng dẫn debug qua Serial
├── RELAY_CONTROL_TROUBLESHOOTING.md
└── smart-room-iot-server-esp32/
    ├── README.md                  # (File này) Tài liệu tổng quát
    ├── auth.md                    # Tài liệu API Login
    ├── config.md                  # Tài liệu API Config
    ├── control.md                 # Tài liệu API Control
    └── code.ino                   # Version server cũ (không sử dụng)
```

## 🚀 Bắt đầu nhanh

### 1. Chuẩn bị thiết bị
- ESP32 Dev Module
- DHT22 sensor (nhiệt độ độ ẩm)
- IR Sender module (điều khiển AC)
- Relay module (điều khiển đèn, quạt)
- WiFi router (2.4GHz)

### 2. Upload firmware
```bash
# Sử dụng Arduino IDE
1. Cài đặt board: ESP32 Dev Module
2. Cài đặt thư viện: ArduinoJson, IRremoteESP8266, CustomJWT, DHT
3. Upload SmartRoom-Client.ino vào ESP32
```

### 3. Cấu hình thiết bị
```bash
# Chỉnh sửa setup.json cho WiFi & Server
# Chỉnh sửa config.json cho các thiết bị
# Upload vào ESP32
```

### 4. Test API
```bash
# Login
curl -X POST http://192.168.1.100:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"vuesp","password":"123456789"}'

# Lấy config
curl -X GET http://192.168.1.100:8080/setup \
  -H "Authorization: Bearer <token>"

# Điều khiển đèn
curl -X POST http://192.168.1.100:8080/control \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"naturalId":"LIGHT_01","category":"LIGHT","power":"ON"}'
```

## 📡 API Endpoints

| Method | Endpoint | Mô tả |
| :----: | :------- | :---- |
| POST | `/auth/login` | Đăng nhập lấy JWT token |
| GET | `/setup` | Lấy cấu hình thiết bị hiện tại |
| PATCH | `/config` | Cập nhật cấu hình thiết bị |
| POST | `/control` | Điều khiển thiết bị (LIGHT, FAN, AC) |
| POST | `/get/temperature` | Lấy nhiệt độ hiện tại |
| GET | `/debug/clear-config` | Xóa config từ preferences (debug) |

## 🔐 Bảo mật

- Tất cả API đều yêu cầu JWT token trừ `/auth/login`
- Token có thời gian sống 1 giờ
- Username/password mặc định: `vuesp` / `123456789`
- CORS được kích hoạt cho toàn bộ origin

## 📦 Cấu trúc Config JSON

### Toàn bộ Config

```json
{
  "roomCode": "R-VU",
  "devices": [
    { ... device objects ... }
  ]
}
```

### Device - Relay (LIGHT, FAN)

```json
{
  "naturalId": "LIGHT_01",
  "category": "LIGHT",
  "translations": {
    "vi": { "name": "Đèn A101 1", "description": "..." },
    "en": { "name": "Light A101 1", "description": "..." }
  },
  "specificType": "GPIO",
  "controlType": "GPIO",
  "gpioPin": [13],
  "internal": {
    "peripheralType": "RELAY"
  }
}
```

**Giải thích:**
- `naturalId`: Định danh duy nhất của thiết bị
- `category`: Loại thiết bị (`LIGHT`, `FAN`, `AIR_CONDITION`, `TEMPERATURE`, `POWER_CONSUMPTION`)
- `gpioPin`: Mảng số pin GPIO (Relay device có 1-3 pins)
- `internal.peripheralType`: Loại peripheral (`RELAY`, `IR_SENDER`)

### Device - IR Sender (AIR_CONDITION)

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
      "power": {
        "ON": "0x20DF10EF",
        "OFF": "0x20DF906F"
      },
      "mode": {
        "COOL": "0x20DF40BF",
        "HEAT": "0x20DFC03F",
        "DRY": "0x20DF00FF",
        "FAN": "0x20DF807F",
        "AUTO": "0x20DFA05F"
      },
      "speed": {
        "1": "0x20DF30CF",
        "2": "0x20DFB04F",
        "3": "0x20DF708F",
        "4": "0x20DFF00F",
        "5": "0x20DF50AF"
      },
      "temperature": {
        "16": "0x20DF906F",
        "17": "0x20DF10EF",
        ...
        "30": "0x20DF40BF"
      },
      "swing": {
        "ON": "0x20DFA05F",
        "OFF": "0x20DFC03F"
      }
    }
  }
}
```

**Giải thích:**
- `internal.brand`: Hãng AC (`LG`, `SAMSUNG`, `PANASONIC`)
- `internal.codeConfigs.bits`: Số bit của mã IR (thường 28 cho LG)
- `codeConfigs.power`, `mode`, `speed`, `temperature`, `swing`: Mã IR hex cho mỗi lệnh

## 🛠️ Mở rộng - Thêm hãng AC mới

### 1. Thêm hàm gửi IR

```cpp
bool sendDaikinIrCode(uint32_t code, uint16_t bits)
{
  // Daikin có protocol riêng, cần xử lý khác
  // irsend.sendDaikin(...);
  return true;
}
```

### 2. Đăng ký vào registry

```cpp
const BrandIrSender kBrandIrSenders[] = {
    {"LG", sendLgIrCode},
    {"SAMSUNG", sendSamsung36IrCode},
    {"PANASONIC", sendPanasonicIrCode},
    {"DAIKIN", sendDaikinIrCode},  // Thêm dòng này
};
```

### 3. Cập nhật config với hãng mới

```json
{
  "naturalId": "LAB_AC_DAIKIN",
  "category": "AIR_CONDITION",
  "...": "...",
  "internal": {
    "peripheralType": "IR_SENDER",
    "brand": "DAIKIN",
    "codeConfigs": { ... }
  }
}
```

## 🐛 Debug & Troubleshooting

### Serial Monitor Output

```
[LOGIN ATTEMPT] username: vuesp
[LOGIN SUCCESS]
[TOKEN] eyJhbGc...

[REQUEST] naturalId: LIGHT_01, category: LIGHT
[DEVICE FOUND] naturalId: LIGHT_01
[LIGHT CONTROL] GPIO: 13, Power: ON
[RELAY ACTION] Setting GPIO 13 to ON
[GPIO WRITE] Pin 13 <- 1 (HIGH)
[GPIO READ] Pin 13 = 1

[API RESPONSE]
{
  "status": 200,
  "message": "Điều khiển thành công",
  "timestamp": "2026-07-07T03:28:27Z"
}
[END RESPONSE]
```

### Nếu AC không hoạt động

1. Kiểm tra `internalObj` có đúng cấu trúc không
   - Phải có `peripheralType: IR_SENDER`
   - Phải có `brand` và `codeConfigs`

2. Kiểm tra mã IR hex
   - Mã phải bắt đầu với `0x`
   - Số bit phải đúng với hãng

3. Gọi endpoint `/debug/clear-config` để reset config cũ
   - Firmware sẽ sử dụng config mặc định từ code

### Serial Logging Guide

Xem file [SERIAL_LOGGING_GUIDE.md](../SERIAL_LOGGING_GUIDE.md) để hiểu thêm về các log message.

## 📝 Thay đổi gần đây

### v2.1 (2026-07-07)
- ✅ Thêm hỗ trợ `AIR_CONDITION` (AC control)
- ✅ Thêm `BrandIrSender` registry cho mở rộng AC brands
- ✅ Cập nhật cấu trúc config: `internal` object cho `peripheralType`, `brand`, `codeConfigs`
- ✅ Thêm endpoint `/debug/clear-config` cho debug
- ✅ Cải thiện Serial logging cho debug

### v2.0 (2026-07-03)
- ✅ Thêm hỗ trợ `roomCode` trong config
- ✅ Thay đổi response format: `{status, message, data, timestamp}`
- ✅ Hỗ trợ cả format cũ (array-only) và mới (object + array)

### v1.0 (2026-06-00)
- ✅ API đăng nhập/login
- ✅ Điều khiển `LIGHT` và `FAN`
- ✅ Lấy cấu hình thiết bị

## 📞 Support

Nếu gặp vấn đề:
1. Kiểm tra Serial Monitor output
2. Xem file `RELAY_CONTROL_TROUBLESHOOTING.md`
3. Xem file `SERIAL_LOGGING_GUIDE.md`
4. Gọi `/debug/clear-config` để reset config
