# SmartRoom IoT Server - ESP32 Firmware Documentation

Tài liệu hoàn chỉnh cho firmware điều khiển thiết bị smart home trên ESP32.

## Cấu trúc Thư mục Tài liệu

Tài liệu được chia thành 2 thư mục chính:

### API_DOCS - Hướng dẫn sử dụng API

Chứa tài liệu chi tiết về từng endpoint API, cách gọi, request/response format.

| File | Nội dung |
|------|---------|
| `README.md` | Tổng quan tất cả API endpoints |
| `authentication.md` | API đăng nhập: `POST /auth/login` |
| `temperature.md` | API đọc nhiệt độ: `GET /temperature?naturalId=...` |
| `humidity.md` | API đọc độ ẩm: `GET /humidity?naturalId=...` |
| `telemetry.md` | API lấy dữ liệu toàn hệ thống: `GET /telemetry` |
| `control.md` | API điều khiển thiết bị: `POST /control` |
| `setup.md` | API lấy cấu hình: `GET /setup` |

Khi nào đọc: Khi muốn biết cách gọi API, format request/response, lỗi có thể xảy ra.

### GUIDELINES - Hướng dẫn lập trình & kỹ thuật

Chứa hướng dẫn chi tiết về cách cài đặt, cấu hình, debug hệ thống.

| File | Nội dung |
|------|---------|
| `README.md` | Overview + Quick start + Common tasks |
| `library-installation.md` | Cách cài đặt tất cả thư viện cần thiết |
| `sensor-initialization.md` | Chiến lược khởi tạo sensor (Early Init strategy) |
| `device-config-structure.md` | Chi tiết cấu trúc JSON config cho mỗi loại device |
| `scd40-sensor.md` | Hướng dẫn kỹ thuật chi tiết về cảm biến SCD40 |

Khi nào đọc: Khi cần setup environment, debug sensor, thêm device mới, hiểu cách hệ thống hoạt động.

## Quick Start

1. Lần đầu setup: Đọc `GUIDELINES/README.md` → `library-installation.md`
2. Hiểu hệ thống: Đọc `GUIDELINES/sensor-initialization.md`
3. Cấu hình device: Đọc `GUIDELINES/device-config-structure.md`
4. Gọi API: Đọc `API_DOCS/README.md` + file endpoint cụ thể
5. Debug SCD40: Đọc `GUIDELINES/scd40-sensor.md`

## Bắt đầu nhanh

### Cho lần đầu setup

1. **Cài đặt thư viện:** Xem [`GUIDELINES/library-installation.md`](GUIDELINES/library-installation.md)
   - Cài ArduinoJson, IRremoteESP8266, DallasTemperature, **SensirionI2cScd4x**

2. **Upload firmware:**
   - Upload `SmartRoom-Client.ino` vào ESP32 Dev Module hoặc khác tuỳ vào board bạn đang dùng

3. **Cấu hình device:**
   - Sửa `config.json` theo hướng dẫn [`GUIDELINES/device-config-structure.md`](GUIDELINES/device-config-structure.md)

4. **Hiểu hệ thống:**
   - Đọc [`GUIDELINES/sensor-initialization.md`](GUIDELINES/sensor-initialization.md) để hiểu cách sensor khởi tạo

### Cho developer muốn debug

- **Debug SCD40:** Xem [`GUIDELINES/scd40-sensor.md`](GUIDELINES/scd40-sensor.md)
- **Hiểu cấu trúc config:** Xem [`GUIDELINES/device-config-structure.md`](GUIDELINES/device-config-structure.md)

### Cho lập trình viên client

- **Gọi API:** Bắt đầu từ [`API_DOCS/README.md`](API_DOCS/README.md)
- **Login:** Xem [`API_DOCS/authentication.md`](API_DOCS/authentication.md)
- **Đọc sensor:** Xem [`API_DOCS/temperature.md`](API_DOCS/temperature.md), [`humidity.md`](API_DOCS/humidity.md), [`telemetry.md`](API_DOCS/telemetry.md)
- **Điều khiển device:** Xem [`API_DOCS/control.md`](API_DOCS/control.md)

## API Endpoints

Tất cả chi tiết về API xem `API_DOCS/README.md`

| Method | Endpoint | Mô tả | Tài liệu |
| :----: | :------- | :---- | :------- |
| POST | `/auth/login` | Đăng nhập lấy JWT token | `authentication.md` |
| GET | `/setup` | Lấy cấu hình thiết bị hiện tại | `setup.md` |
| GET | `/temperature?naturalId=...` | Lấy dữ liệu nhiệt độ | `temperature.md` |
| GET | `/humidity?naturalId=...` | Lấy dữ liệu độ ẩm | `humidity.md` |
| GET | `/telemetry` | Lấy dữ liệu từ tất cả sensor | `telemetry.md` |
| POST | `/control` | Điều khiển thiết bị (LIGHT, FAN, AC) | `control.md` |

Note: Tất cả endpoint (ngoại trừ `/auth/login`) đều yêu cầu header `Authorization: Bearer <token>`

## Cấu trúc Config JSON

Chi tiết đầy đủ xem `GUIDELINES/device-config-structure.md`

### Toàn bộ Config

```json
{
  "roomCode": "R-VU",
  "devices": [
    { "naturalId": "...", "category": "...", ... }
  ]
}
```

### Loại Device được hỗ trợ

| Loại | Category | Module | Ví dụ |
|------|----------|--------|-------|
| Cảm biến nhiệt độ 1-Wire | TEMPERATURE | DS18B20 | Đo nhiệt độ phòng |
| Cảm biến SCD40 | TEMPERATURE, HUMIDITY | SCD40 | Đo T°, độ ẩm, CO2 |
| Đèn | LIGHT | RELAY | Bật/tắt đèn phòng |
| Quạt | FAN | RELAY | Điều chỉnh tốc độ quạt |
| Điều hòa | AIR_CONDITION | IR_SENDER | Điều khiển AC qua IR |

Xem chi tiết từng loại tại `GUIDELINES/device-config-structure.md`

## Bảo mật

- Tất cả API (ngoại trừ `/auth/login`) đều yêu cầu JWT token
- Token lifetime: 1 giờ (3600 giây)
- Default credentials: `vuesp` / `123456789`
- CORS: Được kích hoạt cho tất cả origin
- IMPORTANT: Thay đổi password trong production!

## Danh sách tài liệu ngoài

Có các file markdown khác trong thư mục gốc (SmartRoom-Client/):

| File | Mô tả |
|------|-------|
| `SERIAL_LOGGING_GUIDE.md` | Hướng dẫn đọc serial logs |
| `RELAY_CONTROL_TROUBLESHOOTING.md` | Debug relay/LED issues |
| `SCD40_DEBUGGING.md` | Debug cảm biến SCD40 |
| `test_scd.md` | Test code ví dụ cho SCD40 |

## Liên kết nhanh

- Setup environment: `GUIDELINES/library-installation.md`
- Hiểu flow init: `GUIDELINES/sensor-initialization.md`
- Cấu hình device: `GUIDELINES/device-config-structure.md`
- Debug SCD40: `GUIDELINES/scd40-sensor.md`
- API overview: `API_DOCS/README.md`
- Gọi API login: `API_DOCS/authentication.md`
