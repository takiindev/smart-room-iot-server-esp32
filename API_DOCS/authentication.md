# Authentication API

## POST `/auth/login` - Đăng nhập và lấy JWT token

Endpoint này dùng để đăng nhập và nhận JWT token để sử dụng các API khác.

### Request

```bash
curl -X POST "http://172.16.64.200:8080/auth/login" \
  -H "Content-Type: application/json" \
  -d '{
    "username": "vuesp",
    "password": "123456789"
  }'
```

**Body Parameters:**
- `username` (required): Tên người dùng
- `password` (required): Mật khẩu

### Response

**Success (200 OK):**
```json
{
  "status": 200,
  "message": "Đăng nhập thành công",
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6InZ1ZXNwIiwicm9sZSI6InN1cGVyX3VzZXIiLCJleHAiOjE2MjY0MjU5OTl9.xxx"
  },
  "timestamp": "2026-07-14T10:30:45Z"
}
```

**Error (400 Bad Request):**
```json
{
  "status": 400,
  "message": "Thiếu body request hoặc body không đúng định dạng",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

**Error (401 Unauthorized):**
```json
{
  "status": 401,
  "message": "username hoặc password không đúng",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

**Error (500 Internal Server Error):**
```json
{
  "status": 500,
  "message": "Server chưa đồng bộ xong thời gian",
  "timestamp": "2026-07-14T10:30:45Z"
}
```

### Response Fields

| Tên trường | Loại | Mô tả |
|-----------|------|-------|
| token | string | JWT token để sử dụng cho các API khác |

### Token Information

| Thông tin | Giá trị |
|-----------|--------|
| **Lifetime** | 1 giờ (3600 giây) |
| **Format** | JWT (JSON Web Token) |
| **Algorithm** | HS256 (HMAC SHA-256) |
| **Payload** | username, role, exp |
| **Secret** | Hardcoded trong firmware |

### Token Usage

Sử dụng token nhận được trong header `Authorization` cho các request tiếp theo:

```bash
curl -X GET "http://172.16.64.200:8080/temperature?naturalId=TEMP_ESP32_01" \
  -H "Authorization: Bearer <your_token>"
```

### Default Credentials

```
username: vuesp
password: 123456789
```

⚠️ **IMPORTANT**: Thay đổi credentials trong production environment!

### Ví dụ sử dụng

**JavaScript/Fetch:**
```javascript
// Đăng nhập
const loginResponse = await fetch('http://172.16.64.200:8080/auth/login', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    username: 'vuesp',
    password: '123456789'
  })
});

const loginData = await loginResponse.json();
const token = loginData.data.token;

// Sử dụng token để gọi API khác
const dataResponse = await fetch('http://172.16.64.200:8080/temperature?naturalId=TEMP_ESP32_01', {
  method: 'GET',
  headers: {
    'Authorization': `Bearer ${token}`
  }
});

const data = await dataResponse.json();
console.log('Temperature:', data.data.tempC);
```

**Python:**
```python
import requests

# Đăng nhập
login_response = requests.post(
    'http://172.16.64.200:8080/auth/login',
    json={
        'username': 'vuesp',
        'password': '123456789'
    }
)

login_data = login_response.json()
token = login_data['data']['token']

# Sử dụng token để gọi API khác
headers = {
    'Authorization': f'Bearer {token}'
}

data_response = requests.get(
    'http://172.16.64.200:8080/temperature',
    params={'naturalId': 'TEMP_ESP32_01'},
    headers=headers
)

data = data_response.json()
print(f"Temperature: {data['data']['tempC']}°C")
```

### Chú ý

- Luôn gửi username và password qua HTTPS trong production
- Token hết hạn sau 1 giờ, cần đăng nhập lại để lấy token mới
- Mỗi lần đăng nhập nhận token mới, token cũ không bị vô hiệu hóa
- Không lưu token trong localStorage (security risk) nếu có thể tránh
- Kiểm tra status code response trước khi sử dụng token
