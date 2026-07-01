# Control Module - SmartRoom Client

## Control API Documentation

---

<details>
<summary><b>POST</b> <code>/control</code> - Điều khiển thiết bị theo <code>naturalId</code> và <code>category</code></summary>

> API này dùng để điều khiển các thiết bị đang được hỗ trợ trong firmware hiện tại. 
> 
> Hiện tại chỉ hỗ trợ 2 nhóm: <code>LIGHTING</code> và <code>FAN</code>.
> 
> Request bắt buộc phải có JWT token hợp lệ trong header <code>Authorization</code>.

### Request Headers

| Tên header | Giá trị | Bắt buộc | Mô tả |
| :--------- | :------ | :------- | :---- |
| Content-Type | application/json | Có | Định dạng dữ liệu gửi lên |
| Authorization | Bearer <token> | Có | JWT token lấy từ API đăng nhập |
| Origin | string | Không | Nguồn gốc request (hỗ trợ CORS) |

### Request Body

| Tên trường | Loại | Bắt buộc | Mô tả |
| :--------- | :--- | :------- | :---- |
| naturalId | string | Có | Định danh thiết bị cần điều khiển |
| category | string | Có | Loại thiết bị. Hiện tại chỉ nhận <code>LIGHTING</code> hoặc <code>FAN</code> |
| power | string | Có | Trạng thái điều khiển. Với <code>LIGHTING</code> nhận <code>ON</code> / <code>OFF</code>, với <code>FAN</code> nhận <code>ON</code> / <code>OFF</code> |
| speed | number | Không | Chỉ dùng cho <code>FAN</code> khi <code>power = ON</code>. Giá trị hợp lệ: 1, 2, 3 |

### Request Example - LIGHTING

```json
{
	"naturalId": "LIGHT_01",
	"category": "LIGHTING",
	"power": "ON"
}
```

### Request Example - FAN

```json
{
	"naturalId": "Fan_01",
	"category": "FAN",
	"power": "ON",
	"speed": 2
}
```

### Response (200 OK)

```json
{
	"statusCode": 200,
	"success": true,
	"message": "Điều khiển thành công"
}
```

### Response (200 OK - Không có thay đổi)

```json
{
	"statusCode": 200,
	"success": true,
	"message": "Không có cấu hình nào được thay đổi"
}
```

### Response (400 Bad Request)

> Xảy ra khi body thiếu trường bắt buộc, định dạng dữ liệu sai, hoặc giá trị không hợp lệ.

```json
{
	"statusCode": 400,
	"success": false,
	"message": "Body bắt buộc phải có các trường: naturalId, category"
}
```

### Response (401 Unauthorized)

> Xảy ra khi thiếu header Authorization, token không đúng định dạng, hoặc token không hợp lệ/hết hạn.

```json
{
	"statusCode": 401,
	"success": false,
	"message": "Token hết hạn hoặc không đúng"
}
```

### Response (404 Not Found)

> Xảy ra khi `naturalId` không khớp với bất kỳ thiết bị nào trong cấu hình hiện tại.

```json
{
	"statusCode": 404,
	"success": false,
	"message": "Không tìm thấy thiết bị có naturalId tương ứng"
}
```

### Response (500 Internal Server Error)

> Xảy ra khi firmware không parse được JSON cấu hình thiết bị.

```json
{
	"status": 500,
	"message": "Lỗi thiết bị khi parse JSON"
}
```

### Hỗ trợ theo category

#### LIGHTING

- `power` bắt buộc có giá trị `ON` hoặc `OFF`
- Thiết bị được map theo `naturalId`
- Firmware điều khiển relay ở chân GPIO đầu tiên của thiết bị

#### FAN

- `power = OFF`: tắt toàn bộ GPIO của quạt
- `power = ON`: phải có thêm `speed`
- `speed` chỉ nhận giá trị `1`, `2`, `3`
- Firmware sẽ tắt toàn bộ chân quạt trước khi bật chân tương ứng với tốc độ

### Lưu ý

- API này chỉ hỗ trợ đúng 2 category đang có trong firmware: `LIGHTING` và `FAN`
- `naturalId` phải trùng với cấu hình thiết bị đã nạp trong firmware
- JWT token phải được gửi qua header `Authorization: Bearer <token>`
- Nếu body trống hoặc JSON không hợp lệ, server sẽ trả lỗi `400`

</details>
