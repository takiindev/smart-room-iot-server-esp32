# Auth Module - SmartRoom Client

## Auth API Documentation

---

<details>
<summary><b>POST</b> <code>/auth/login</code> - Đăng nhập vào thiết bị SmartRoom</summary>

> Đăng nhập vào thiết bị SmartRoom để lấy JWT token. Token này được sử dụng để xác thực cho các request điều khiển thiết bị sau này.

### Request Headers

| Tên header | Giá trị | Bắt buộc | Mô tả |
| :--------- | :------ | :------- | :---- |
| Content-Type | application/json | Có | Định dạng dữ liệu gửi lên |
| Origin | string | Không | Nguồn gốc request (hỗ trợ CORS) |

### Request Body

| Tên trường | Loại | Bắt buộc | Mô tả |
| :--------- | :--- | :------- | :---- |
| username | string | Có | Tên đăng nhập |
| password | string | Có | Mật khẩu đăng nhập|

### Request Example

```json
{
	"username": "admin",
	"password": "admin123"
}
```

### Response (200 OK)

```json
{
	"statusCode": 200,
	"success": true,
	"message": "Đăng nhập thành công",
	"token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6ImFkbWluIiwicm9sZSI6InN1cGVyX3VzZXIiLCJleHAiOjE3NDg3NjU0MzJ9.signature"
}
```

### Response (401 Unauthorized)

```json
{
	"statusCode": 401,
	"success": false,
	"message": "username hoặc password không đúng"
}
```

### Response (500 Internal Server Error)

> Xảy ra khi thiết bị chưa đồng bộ thời gian thành công từ NTP server.

```json
{
	"statusCode": 500,
	"success": false,
	"message": "Server chưa đồng bộ xong thời gian, hãy thử lại sau vài giây!"
}
```

### Response (400 Bad Request)

```json
{
	"statusCode": 400,
	"success": false,
	"message": "Thiếu body request hoặc body không đúng định dạng"
}
```

### Thông tin JWT Token

Sau khi đăng nhập thành công, server trả về JWT token với các thông tin sau:

| Trường | Loại | Mô tả |
| :----- | :--- | :---- |
| username | string | Tên đăng nhập của client |
| role | string | Vai trò (mặc định: `super_user`) |
| exp | long | Thời gian hết hạn (epoch time), token có hiệu lực trong 1 giờ |

### Lưu ý

- Token được sử dụng trong header `Authorization: Bearer <token>` cho các API điều khiển thiết bị
- Token có thời gian sống là 1 giờ kể từ lúc phát hành

</details>