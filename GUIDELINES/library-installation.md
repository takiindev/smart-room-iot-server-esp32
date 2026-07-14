# Hướng dẫn cài đặt thư viện cho SmartRoom-Client

## 1. Thư viện cần thiết

### Core Libraries
- ✅ `ArduinoJson` (6.x hoặc cao hơn) - Quản lý JSON
- ✅ `CustomJWT` - JWT Token (nếu chưa có, cài từ `.zip`)
- ✅ `IRremoteESP8266` - Điều khiển IR
- ✅ `DallasTemperature` - Cảm biến DS18B20
- ✅ `OneWire` - Giao tiếp 1-Wire

### Sensor Libraries
- ✅ `SensirionI2cScd4x` - **Cảm biến SCD40** (QUAN TRỌNG!)
- ✅ `SensirionCore` - Core library của Sensirion (cài tự động)

---

## 2. Cài đặt chi tiết

### Bước 1: Mở Arduino IDE

### Bước 2: Cài đặt Sketch → Include Library → Manage Libraries

### Bước 3: Cài từng library

#### a) **ArduinoJson**
```
Tìm: "ArduinoJson"
Author: Benoit Blanchon
Phiên bản: 6.21.0 (hoặc mới hơn)
```
Click **Install**

#### b) **IRremoteESP8266**
```
Tìm: "IRremoteESP8266"
Author: David Conran
Phiên bản: 1.2.x (hoặc mới hơn)
```
Click **Install**

#### c) **DallasTemperature**
```
Tìm: "DallasTemperature"
Author: Miles Burton
Phiên bản: 3.9.0 (hoặc mới hơn)
```
Click **Install**

#### d) **SensirionI2cScd4x** ⭐ QUAN TRỌNG!
```
Tìm: "SensirionI2cScd4x"
Author: Sensirion
Phiên bản: 0.2.0 (hoặc mới hơn)
```
Click **Install** (sẽ cài SensirionCore tự động)

### Bước 4: Cài CustomJWT (nếu chưa có)

Nếu không tìm được CustomJWT trong Library Manager:

1. Tải từ GitHub: `https://github.com/trescomuna/CustomJWT`
2. Download ZIP
3. Arduino IDE → Sketch → Include Library → Add .ZIP Library
4. Chọn file ZIP CustomJWT

---

## 3. Kiểm tra cài đặt

Sau khi cài xong, verify include paths trong SmartRoom-Client.ino:

```cpp
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CustomJWT.h>
#include "time.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SensirionI2cScd4x.h>    // ← Chữ hoa I, thường c
#include <SensirionCore.h>
```

**⚠️ CHÍNH XÁC:**
- ✅ `SensirionI2cScd4x.h` (I: hoa, c: thường)
- ❌ `SensirionI2CScd4x.h` (sai - C hoa)

---

## 4. Compile & Upload

### Kiểm tra Compile
```
Sketch → Verify/Compile
```

Nếu thành công → Không có lỗi red

### Upload lên ESP32
```
Sketch → Upload
```

Chọn board: **ESP32 Dev Module**
Chọn COM port: `dev/cu.usbserial-XXXX`

---

## 5. Troubleshooting

### Lỗi: "fatal error: SensirionI2cScd4x.h: No such file or directory"

**Nguyên nhân**: Library chưa cài hoặc cài sai tên

**Fix**:
1. Vào Arduino IDE → Sketch → Include Library → Manage Libraries
2. Gõ "SensirionI2cScd4x"
3. Kiểm tra author là "Sensirion"
4. Click Install

### Lỗi: "Khởi tạo SCD40 thất bại"

**Nguyên nhân**: Hardware issue

**Check**:
- GPIO pins: SDA=21, SCL=22
- Pull-up resistors: 4.7kΩ trên SDA + SCL
- Kết nối VCC (3.3V) + GND
- Cảm biến phản hồi đúng I2C address: 0x62

### Lỗi: "Compilation error: multiple definitions"

**Nguyên nhân**: Duplicate include

**Fix**: Xóa duplicate `#include` statements

---

## 6. Library Versions (Recommended)

| Library | Version | Author |
|---------|---------|--------|
| ArduinoJson | 6.21.0+ | Benoit Blanchon |
| IRremoteESP8266 | 1.2.0+ | David Conran |
| DallasTemperature | 3.9.0+ | Miles Burton |
| OneWire | 2.3.8+ | Paul Stoffregen |
| SensirionI2cScd4x | 0.2.0+ | Sensirion |
| SensirionCore | 0.x.x | Sensirion (auto) |
| CustomJWT | Latest | trescomuna |

---

## 7. Xác nhận môi trường

Sau khi setup xong:

```bash
# Compile project
Sketch → Verify

# Nếu thành công:
# "Sketch uses 1234567 bytes of program storage space"

# Upload lên board
Sketch → Upload

# Xem serial output
Tools → Serial Monitor (115200 baud)
```

Bạn sẽ thấy:
```
16:12:34.968 -> Đang kết nối WiFi...
16:12:35.000 -> Kết nối WiFi thành công!
16:12:35.214 -> [SCD40] Khởi tạo thành công lần đầu!
```

---

## 8. Quick Check List

- [ ] Arduino IDE 1.8.x+ hoặc 2.0+
- [ ] Board: ESP32 Dev Module
- [ ] Port: COM XX (hoặc `/dev/cu.usbserial-XX`)
- [ ] Baud Rate: 115200
- [ ] ArduinoJson cài xong
- [ ] IRremoteESP8266 cài xong
- [ ] DallasTemperature cài xong
- [ ] SensirionI2cScd4x cài xong ⭐
- [ ] Include paths đúng (case-sensitive)
- [ ] Config JSON có `"module": "SCD40"`
- [ ] GPIO pins: SDA=21, SCL=22

Nếu tất cả ✅ → Ready to compile & upload!
