# Flying Survey

Flying Survey là hệ thống mô phỏng điều khiển thiết bị bay khảo sát môi trường cho học phần IT4062. Dự án dùng C trên Ubuntu hoặc WSL chạy Ubuntu, TCP socket để trao đổi bản tin, `poll()` để xử lý nhiều kết nối trên server và SQLite để lưu dữ liệu.

Server hiện có thể lắng nghe TCP, nhận và tách khung từ nhiều kết nối. Kịch bản demo bên dưới mô tả các chức năng điều khiển và báo cáo theo thiết kế dự kiến.

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

- Mỗi khung bắt đầu bằng 4 byte biểu diễn độ dài phần thân theo thứ tự byte mạng (big-endian). Phần thân dài từ 1 đến 16.384 byte. Ví dụ, khung chứa `abc` có dạng hex `00 00 00 03 61 62 63`. Server đóng kết nối gửi độ dài sai ngay khi đọc đủ phần đầu.
- Server dùng `poll()` và giữ bộ đệm riêng cho tối đa 64 kết nối đồng thời. Mỗi lần nhận đủ một khung, server ghi số byte của phần thân ra terminal. Chạy `make test-frame` để kiểm tra bộ đóng và tách khung.
- Yêu cầu và phản hồi mang mã yêu cầu (`request_id`). Với lệnh chuyển tiếp, server lưu ánh xạ giữa yêu cầu từ client và xác nhận từ thiết bị để trả về đúng phiên. Báo cáo định kỳ có loại bản tin riêng để server xử lý độc lập với lệnh đang chờ.
- Mỗi kết nối có hàng đợi gửi và lưu vị trí đã gửi trong khung đầu hàng đợi. Server theo dõi `POLLOUT` khi còn dữ liệu và tiếp tục từ byte kế tiếp nếu `send()` chỉ ghi được một phần.

## Bảng tài khoản SQLite

Schema trong `db/schema.sql` tạo bảng `accounts` gồm `id` làm khóa chính, `username` với ràng buộc `NOT NULL UNIQUE` và `password_hash` với ràng buộc `NOT NULL`. Tên đăng nhập phân biệt chữ hoa/chữ thường theo mặc định của SQLite. Cột `password_hash` dành cho chuỗi băm chứa cả thuật toán, tham số và salt do phần đăng ký tạo ra.

Khởi tạo tệp dữ liệu bằng SQLite CLI, có thể cài qua `sudo apt install sqlite3`.

```bash
sqlite3 flying-survey.db < db/schema.sql
```

Chạy kiểm thử schema từ thư mục dự án bằng Python 3.11 trở lên, trên Windows thay `python3` bằng `python`.

```bash
python3 -B -m unittest discover -s tests -v
```

Kiểm thử dùng SQLite trong bộ nhớ để xác nhận trùng tên bị từ chối, tên khác được chấp nhận và chạy lại schema giữ nguyên dữ liệu. Server sẽ cần nạp schema khi triển khai phần khởi tạo SQLite.

## Chạy cửa sổ đăng nhập Tkinter

Cài Python 3 có Tkinter. Trên Ubuntu/WSL có môi trường đồ họa, có thể cài bằng `sudo apt install python3-tk` rồi chạy từ thư mục dự án.

```bash
python3 src/client/login.py
```

Trên Windows, dùng `python src/client/login.py`. Cửa sổ có tên đăng nhập, mật khẩu được che và nút Đăng nhập. Nhấn nút hoặc Enter để kiểm tra hai trường bắt buộc. Trường chỉ chứa khoảng trắng được xem là bỏ trống. Khi nhập đủ, cửa sổ hiển thị trạng thái kiểm tra dữ liệu. Chức năng xác thực với server đang chờ triển khai.

## Chạy server TCP

Trong Ubuntu hoặc WSL với `build-essential` đã cài, chạy từ thư mục dự án.

```bash
make
./bin/server --port 9000
```

`--port` nhận số từ 1 đến 65535. Server mặc định lắng nghe tại `127.0.0.1` và nhận địa chỉ IPv4 khác qua `--host`. Server in lỗi ra `stderr` và thoát với mã khác 0 nếu cổng đã được sử dụng. Dừng server bằng `Ctrl+C`.

## Chạy simulator TCP

Sau khi chạy `make` và khởi động server, mở terminal khác trong cùng thư mục dự án.

```bash
./bin/simulator --host 127.0.0.1 --port 9000
```

`--host` nhận địa chỉ IPv4, mặc định là `127.0.0.1`. `--port` là bắt buộc và nhận số từ 1 đến 65535. Simulator in `Connected to ...` khi kết nối thành công và giữ tiến trình chạy đến khi nhấn `Ctrl+C`. Khi kết nối thất bại, chương trình in địa chỉ, cổng cùng lỗi hệ thống ra `stderr`, đóng socket và thoát với mã `1`. Vòng lặp nhận lệnh và gửi báo cáo đang chờ triển khai.

## Môi trường và cách chạy demo dự kiến

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

- Định nghĩa loại bản tin, trường bắt buộc, cách mã hóa phần thân và quy tắc `request_id` dựa trên định dạng khung ở trên.
- Chốt mã thành công và mã lỗi cho dữ liệu sai, phiên không hợp lệ, thiếu quyền, thiết bị ngoại tuyến và hết thời gian chờ xác nhận.
- Chốt đơn vị đo và định dạng thời gian. Các trường cần thống nhất gồm nhiệt độ, độ ẩm, chỉ số chất lượng không khí, tọa độ GPS, tốc độ, độ cao, năng lượng và quãng đường còn lại.
- Chốt cách tạo và kết thúc phiên, thời hạn phiên, xử lý khi mất kết nối, quyền theo từng thiết bị và cách server xác thực thiết bị mô phỏng.
- Lưu tuyến mẫu với thứ tự chặng, vị trí xuất phát và chu kỳ báo cáo để cả nhóm dùng cùng dữ liệu demo.

## Căn cứ thiết kế
- **Lựa chọn triển khai của dự án** gồm C trên Ubuntu/WSL, TCP, `poll()`, SQLite, CLI, tiến trình mô phỏng riêng và giao thức đóng khung theo độ dài có mã yêu cầu.
