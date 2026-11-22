# Flying Survey

Flying Survey là hệ thống mô phỏng điều khiển thiết bị bay khảo sát môi trường cho học phần IT4062. Dự án dùng C trên Ubuntu hoặc WSL chạy Ubuntu, TCP socket để trao đổi bản tin, `poll()` để xử lý nhiều kết nối trên server và SQLite để lưu dữ liệu.

README này mô tả thiết kế dự kiến. Kho mã hiện chưa có mã nguồn hay `Makefile`. Các lệnh bên dưới là quy ước dự kiến cho bản demo.

## Ba chương trình

| Chương trình | Trách nhiệm |
| --- | --- |
| Server | Quản lý phiên đăng nhập và kết nối thiết bị, chuyển lệnh và nhận xác nhận. Lưu tài khoản, thiết bị, quyền truy cập, lịch bay, các chặng và báo cáo trong SQLite. |
| Client người dùng | Cung cấp menu CLI để đăng ký, đăng nhập, đăng xuất, liệt kê và xem chi tiết thiết bị, thêm/sửa/xóa thiết bị, xem báo cáo, bật/tắt, đặt tốc độ và độ cao, xem lịch bay và thêm/sửa/xóa các chặng. |
| Thiết bị mô phỏng | Kết nối TCP tới server, nhận lệnh và danh sách chặng của lịch bay, trả kết quả thực thi, cập nhật vị trí theo thứ tự chặng và gửi báo cáo định kỳ. Dùng cùng một tuyến mẫu và vị trí xuất phát cho mỗi lần demo. |

## Luồng lệnh và báo cáo

1. Người dùng gửi lệnh từ CLI. Server kiểm tra phiên và quyền trên thiết bị trước khi chuyển lệnh hợp lệ tới thiết bị đang kết nối.
2. Thiết bị trả xác nhận kèm kết quả thực thi. Server ghép xác nhận với yêu cầu đang chờ và trả kết quả về CLI. Server báo riêng trường hợp thiết bị ngoại tuyến và trường hợp hết thời gian chờ xác nhận.
3. Thiết bị gửi báo cáo định kỳ về server. Báo cáo gồm thời điểm đo, nhiệt độ, độ ẩm, chỉ số chất lượng không khí, tọa độ GPS, độ cao, tốc độ, trạng thái hoạt động, năng lượng còn lại và quãng đường còn có thể bay.
4. Server lưu báo cáo theo thiết bị và thời điểm. Client truy vấn qua server để xem thông tin mới nhất hoặc lịch sử báo cáo theo quyền của tài khoản.

## Giao thức TCP và xử lý vào/ra

- Mỗi khung bắt đầu bằng trường độ dài phần thân tính theo byte. Server kiểm tra giới hạn kích thước trước khi chờ hoặc cấp phát cho phần thân.
- Yêu cầu và phản hồi mang mã yêu cầu (`request_id`). Với lệnh chuyển tiếp, server lưu ánh xạ giữa yêu cầu từ client và xác nhận từ thiết bị để trả về đúng phiên. Báo cáo định kỳ có loại bản tin riêng để server xử lý độc lập với lệnh đang chờ.
- Server dùng socket không chặn và `poll()`. Mỗi kết nối có bộ đệm nhận để ghép khung bị chia qua nhiều lần `recv()` và tách các khung được nhận cùng lúc.
- Mỗi kết nối có hàng đợi gửi và lưu vị trí đã gửi trong khung đầu hàng đợi. Server theo dõi `POLLOUT` khi còn dữ liệu và tiếp tục từ byte kế tiếp nếu `send()` chỉ ghi được một phần.

## Môi trường và cách chạy dự kiến

Trong terminal Ubuntu hoặc WSL, mở thư mục dự án và cài công cụ biên dịch cùng thư viện phát triển SQLite.

```bash
sudo apt update
sudo apt install build-essential libsqlite3-dev
make
```

Sau khi có ba chương trình, mở ba terminal trong cùng thư mục dự án. Bản demo dự kiến dùng địa chỉ `127.0.0.1`, cổng `9000` và tệp SQLite `flying-survey.db`.

**Terminal 1 · Server**

```bash
./bin/server --host 127.0.0.1 --port 9000 --db flying-survey.db
```

**Terminal 2 · Client người dùng**

```bash
./bin/client --host 127.0.0.1 --port 9000
```

Đăng ký, đăng nhập và tạo thiết bị `SIM-01` bằng CLI trước khi khởi động thiết bị mô phỏng.

**Terminal 3 · Thiết bị mô phỏng**

```bash
./bin/device --host 127.0.0.1 --port 9000 --device-id SIM-01
```

## Kịch bản demo

| Bước | Thao tác và kết quả cần quan sát |
| --- | --- |
| 1. Tài khoản và quyền | Đăng ký, đăng nhập và tạo `SIM-01`. Kiểm tra danh sách, chi tiết và thao tác sửa thông tin thiết bị. Dùng tài khoản chưa được cấp quyền để thử điều khiển `SIM-01` và kiểm tra server từ chối. |
| 2. Lịch và chặng bay | Xem lịch bay của `SIM-01`, thêm các chặng của tuyến mẫu cố định, thử sửa và xóa một chặng, rồi khôi phục tuyến mẫu. Kiểm tra thứ tự các chặng trên CLI. |
| 3. Điều khiển thiết bị | Khởi động tiến trình mô phỏng. Từ CLI gửi lệnh bật, đặt tốc độ và đặt độ cao. Đối chiếu lệnh mà thiết bị nhận với xác nhận hiển thị trên CLI. |
| 4. Theo dõi báo cáo | Chờ ít nhất hai kỳ báo cáo. Kiểm tra vị trí thay đổi theo thứ tự chặng và các thông số xuất hiện trong lịch sử. |
| 5. Kết thúc và kiểm tra lưu trữ | Gửi lệnh tắt và kiểm tra xác nhận. Dùng một thiết bị thử riêng để thao tác xóa, rồi đăng xuất và kiểm tra lệnh yêu cầu phiên bị từ chối. Khởi động lại server với cùng tệp SQLite, kết nối lại client, đăng nhập và kiểm tra lịch sử báo cáo của `SIM-01`. |

Khi kiểm tra giao thức, gửi một khung qua nhiều lần ghi và gửi nhiều khung trong cùng một lần ghi. Kiểm tra thêm trường hợp `send()` ghi thiếu byte để xác nhận server gửi tiếp phần còn lại đúng thứ tự.

## Thống nhất trong tuần đầu

Chốt đặc tả chung trước khi tách nhánh cho server, client và thiết bị mô phỏng.

- Định nghĩa loại bản tin, trường bắt buộc, cách mã hóa phần thân, số byte và thứ tự byte của trường độ dài, kích thước khung tối đa và quy tắc `request_id`.
- Chốt mã thành công và mã lỗi cho dữ liệu sai, phiên không hợp lệ, thiếu quyền, thiết bị ngoại tuyến và hết thời gian chờ xác nhận.
- Chốt đơn vị đo và định dạng thời gian. Các trường cần thống nhất gồm nhiệt độ, độ ẩm, chỉ số chất lượng không khí, tọa độ GPS, tốc độ, độ cao, năng lượng và quãng đường còn lại.
- Chốt cách tạo và kết thúc phiên, thời hạn phiên, xử lý khi mất kết nối, quyền theo từng thiết bị và cách server xác thực thiết bị mô phỏng.
- Lưu tuyến mẫu với thứ tự chặng, vị trí xuất phát và chu kỳ báo cáo để cả nhóm dùng cùng dữ liệu demo.

## Căn cứ thiết kế
- **Lựa chọn triển khai của dự án** gồm C trên Ubuntu/WSL, TCP, `poll()`, SQLite, CLI, tiến trình mô phỏng riêng và giao thức đóng khung theo độ dài có mã yêu cầu.
