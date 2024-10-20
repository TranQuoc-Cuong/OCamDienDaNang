# OCamDienDaNang
Đồ Án Ổ Cắm Điện Đăng + ESP32 Arduino IDE + code STM32 + Flask 

Công dụng: bật tắt từ xa, có server lưu giữ liệu dòng điện, điện áp, công suất.

Đồ án bao gồm:
Device:
  ESP32, STM32F103C8T6, Mạch đóng ngắt (Triac, MOC3041, tụ, điện trở), Cảm biến dòng (ZMCT103C), Module Cảm biến áp (ZMPT101B), Ổn áp AMS1117, HLK-PM01 AC-DC(200VAC - 5VDC).
Data:
  Splite
Aplication:
  Flask

Devices: Khả năng đo dòng điện, điện áp, công suất, khả năng kết nối Wifi, đóng ngắt từ xa.
  Trường hợp hoạt động:
1. Khi không kết nối được wifi or flask: tự động phát wifi ra cho người dùng nhập tên wifi và mật khẩu wifi, và địa chỉ IP flask lưu vào flask của esp32.
2. Khi kết nối được với flask: Nhận lệnh từ flask và truyền dòng điện và điện áp, công suất

Server:
Flask: Giao diện web để người dùng sử dụng:
Bao gồm:
Nút bật tắt
Dòng hiển thị điện áp, dòng điện, công suất, trạng thái bật tắt.
Lịch sử dòng điện, điện áp, công suất, trang thái bật tắt -- thời gian
Lưu dữ liệu Sqlite

Sqlite: Lưu trữ dữ liệu dòng điện, điện áp công suất trạng thái nút nhấn.


Code Stm32: bao gồm cấu hình bằng STM32CubeMX, code bằng keilC.
  Có sử dụng GPIO, UART (có RingBuffer), ADC.

Code ESP32: code kết nối với flask thông qua http và phát wifi để người dùng nhập địa chỉ flask. 
