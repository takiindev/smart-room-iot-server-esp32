# SmartRoom IoT Server - ESP32 Firmware Documentation

Tài liệu hoàn chỉnh cho firmware điều khiển thiết bị smart home trên ESP32.

## Danh sách tài liệu

1. **[auth.md](auth.md)** - API đăng nhập và quản lý JWT token
2. **[config.md](config.md)** - API lấy và cập nhật cấu hình thiết bị
3. **[control.md](control.md)** - API điều khiển thiết bị (LIGHT, FAN, AIR_CONDITION)
4. **[DEVICE_CONFIG_STRUCTURE.md](DEVICE_CONFIG_STRUCTURE.md)** - Tham khảo nhanh cấu trúc config cho tất cả loại thiết bị

## Cấu trúc dự án

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

## Bắt đầu nhanh

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

## API Endpoints

| Method | Endpoint | Mô tả |
| :----: | :------- | :---- |
| POST | `/auth/login` | Đăng nhập lấy JWT token |
| GET | `/setup` | Lấy cấu hình thiết bị hiện tại |
| PATCH | `/config` | Cập nhật cấu hình thiết bị |
| POST | `/control` | Điều khiển thiết bị (LIGHT, FAN, AC) |
| POST | `/get/temperature` | Lấy nhiệt độ hiện tại |
| GET | `/debug/clear-config` | Xóa config từ preferences (debug) |

## Bảo mật

- Tất cả API đều yêu cầu JWT token trừ `/auth/login`
- Token có thời gian sống 1 giờ
- Username/password mặc định: `vuesp` / `123456789`
- CORS được kích hoạt cho toàn bộ origin

## Cấu trúc Config JSON

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
      "speed": {
        "1": "0x880A30D",
        "2": "0x880A32F",
        "3": "0x880A341"
      },
      "temperature": {
        "16": "0x880A14F",
        "17": "0x880A163",
        ...
        "30": "0x880A267"
      },
      "swing": {
        "ON": "0x8810001"
      }
    }
  }
}
```

**Giải thích:**
- `internal.brand`: Hãng AC (`LG`, `SAMSUNG`, `PANASONIC`)
- `internal.codeConfigs`: Object chứa tất cả mã IR cho các lệnh
- `codeConfigs.power`, `mode`, `speed`, `temperature`, `swing`: Mã IR hex cho mỗi lệnh
- Không cần field `bits` (mặc định LG dùng 28 bits)
- Mã IR phải là chuỗi hex bắt đầu với `0x`

## Mở rộng - Thêm hãng AC mới

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

## Debug & Troubleshooting

### Serial Monitor Output

```
[LOGIN ATTEMPT] username: vuesp
[LOGIN SUCCESS]
[TOKEN] eyJhbGc...

[REQUEST] naturalId: LABAC1, category: AIR_CONDITION
[DEVICE FOUND] naturalId: LABAC1
[Thực thi] setTemperatureAC() -> Hãng LG, mã 0x880A1EF, bits 28

[API RESPONSE]
{
  "status": 200,
  "message": "Điều khiển AC thành công",
  "timestamp": "2026-07-07T03:28:27Z"
}
[END RESPONSE]
```

### Nếu AC không hoạt động

1. **Kiểm tra cấu trúc config:**
   - Device phải có `internal` object
   - `internal.peripheralType` phải là `"IR_SENDER"`
   - `internal.brand` phải là một trong: `LG`, `SAMSUNG`, `PANASONIC`
   - `internal.codeConfigs` phải có đầy đủ sections: `power`, `mode`, `speed`, `temperature`, `swing`

2. **Kiểm tra mã IR hex:**
   - Mã phải bắt đầu với `0x`
   - Ví dụ: `"ON": "0x8800F43"` (đúng), `"ON": "8800F43"` (sai)
   - Mỗi key trong section phải có giá trị (ví dụ: `temperature` phải có keys từ `"16"` đến `"30"`)

3. **Kiểm tra Serial Monitor:**
   - Xem log `[Thực thi]` để biết lệnh nào đang được gửi
   - Nếu thấy lỗi `"Lỗi: Thiếu mã config..."` nghĩa là thiếu key trong `codeConfigs`
   - Nếu thấy `"Lỗi: Không có hàm gửi IR..."` nghĩa là brand chưa được đăng ký

4. **Reset config và thử lại:**
   ```bash
   # Gọi endpoint clear config
   curl http://172.16.64.200:8080/debug/clear-config
   
   # ESP32 sẽ sử dụng config mặc định từ code
   # Sau đó upload config mới qua /config endpoint
   ```

5. **Kiểm tra hardware:**
   - IR LED có kết nối đúng GPIO pin không (mặc định GPIO 18)
   - IR LED có nguồn điện đủ không
   - Khoảng cách giữa IR sender và AC receiver (nên < 5m)

### Serial Logging Guide

Xem file [SERIAL_LOGGING_GUIDE.md](../SERIAL_LOGGING_GUIDE.md) để hiểu thêm về các log message.

## Thay đổi gần đây

### v2.1 (2026-07-07)
- ✅ Thêm hỗ trợ `AIR_CONDITION` (AC control) qua IR sender
- ✅ Thêm `BrandIrSender` registry cho mở rộng AC brands (LG, Samsung, Panasonic)
- ✅ Cập nhật cấu trúc config: `internal` object chứa `peripheralType`, `brand`, `codeConfigs`
- ✅ Loại bỏ field `bits` (mặc định 28 cho LG, tự động theo brand)
- ✅ Thêm endpoint `/debug/clear-config` cho việc debug và reset config
- ✅ Cải thiện Serial logging với markers rõ ràng: `[LOGIN]`, `[DEVICE FOUND]`, `[Thực thi]`
- ✅ Sửa lỗi validation: AC speed chỉ hỗ trợ 1-3 (không phải 1-5)
- ✅ Cập nhật IR codes từ LG AC thực tế (đã test và hoạt động)

### v2.0 (2026-07-03)
- ✅ Thêm hỗ trợ `roomCode` trong config
- ✅ Thay đổi response format: `{status, message, data, timestamp}`
- ✅ Hỗ trợ cả format cũ (array-only) và mới (object + array)

### v1.0 (2026-06-00)
- ✅ API đăng nhập/login
- ✅ Điều khiển `LIGHT` và `FAN`
- ✅ Lấy cấu hình thiết bị

## Support

Nếu gặp vấn đề:
1. Kiểm tra Serial Monitor output
2. Xem file `RELAY_CONTROL_TROUBLESHOOTING.md`
3. Xem file `SERIAL_LOGGING_GUIDE.md`
4. Gọi `/debug/clear-config` để reset config
