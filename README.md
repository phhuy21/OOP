# SkyGate — Hệ thống Quản lý Sân bay & Đặt vé

Hệ thống quản lý sân bay (SkyGate) được xây dựng trên nền tảng **C++ hướng đối tượng (OOP)** ở Backend, kết hợp giao diện **Web (HTML/CSS/JS)** ở Frontend thông qua **REST API**. Đây là Bài tập lớn môn Lập trình hướng đối tượng (LTHDT) của Nhóm 5 — Trường Đại học Công Nghệ Kỹ thuật TP.HCM (HCMUTE).

🔗 **Link trải nghiệm trực tuyến:** [http://159.65.141.209:8888](http://159.65.141.209:8888)

> Tài khoản demo: **admin / admin123** · **staff / staff123** · **customer / cus123**

---

## 1. Tổng quan dự án

SkyGate mô phỏng hoạt động điều hành tại **một sân bay** (sân bay nhà), với **trọng tâm là nghiệp vụ đặt/huỷ vé và giám sát hành khách**. Hệ thống phân tách thành **3 phân hệ người dùng** với quyền hạn khác nhau, quản lý vòng đời chuyến bay (lịch trình, gán máy bay & cổng đỗ, đường băng) và theo dõi hành trình của từng hành khách từ lúc đặt vé đến khi lên máy bay.

Các chuyến bay đều **khởi hành từ sân bay nhà** đến một **thành phố đích**. Mỗi khi một hành khách mua vé, hệ thống tạo một vé (Ticket) và **trừ đi một ghế trống** của chuyến; huỷ vé sẽ trả lại ghế.

### Ba phân hệ người dùng (phân quyền)

| Vai trò | Quyền hạn chính |
|---|---|
| **Admin** | Toàn quyền: quản lý hạ tầng (máy bay, đường băng), tạo/xoá chuyến bay, **cấp & xoá tài khoản** Staff/Customer. |
| **Staff** | Vận hành: tạo chuyến bay, gán máy bay & cổng, đổi trạng thái chuyến, check-in / boarding, đặt/huỷ vé, **giám sát hành khách**. |
| **Customer** | Tra cứu chuyến bay, **mua / huỷ vé** của chính mình. |

> Phân quyền được kiểm tra **phía máy chủ (C++)**, không chỉ ẩn nút ở giao diện.

---

## 2. Các tính năng cốt lõi

### Đặt & huỷ vé (trọng tâm)
* Customer chọn chuyến còn ghế trống, nhập tên hành khách và mua vé. Hệ thống gọi `Flight::bookSeat()` (giảm ghế trống), sinh ra một `Ticket` và một `Passenger` gắn với chuyến.
* Huỷ vé trả lại ghế cho chuyến.
* Báo lỗi khi chuyến đã hết ghế hoặc đã kết thúc.

### Giám sát hành khách
* **Hành trình từng khách:** Đã đặt chỗ → Check-in → Lên máy bay.
* **Tra cứu hành khách:** lọc theo tên / mã khách / hộ chiếu / mã chuyến.
* **Theo dõi theo chuyến / cổng:** đếm số khách đã đặt, đã check-in, đã lên máy bay, số ghế trống, mức lấp đầy.
* **Cảnh báo tự động:** khách chưa check-in khi sắp tới giờ bay, chuyến sắp đầy / hết ghế, hành lý quá cân.

### Quản lý chuyến bay & hạ tầng
* Vòng đời chuyến bay theo máy trạng thái: `Scheduled → Check-in → Boarding → Gate Closed → Ready → Takeoff → In Air → Landed → Turnaround → Completed` (kèm `Delayed` / `Cancelled`).
* **Đồng hồ mô phỏng:** tua nhanh thời gian ảo để chuyến bay tự chuyển trạng thái theo lịch.
* Gán máy bay & cổng đỗ (kiểm tra tương thích loại gate, đường băng đủ dài, sức chứa).
* Admin quản lý đội máy bay và đường băng của sân bay nhà.

### Ký gửi hành lý & sơ đồ ghế
* Cảnh báo cước phí khi hành lý vượt **23 kg/kiện**.
* Chọn ghế trực quan khi check-in.

---

## 3. Kiến trúc và Thiết kế hướng đối tượng (OOP)

Dự án áp dụng đầy đủ các nguyên lý OOP:

* **Đóng gói (Encapsulation):** dữ liệu nội tại của các thực thể (`Flight`, `Aircraft`, `Passenger`, `Ticket`, `User`...) là `private`/`protected`, truy xuất qua getter/setter.
* **Kế thừa (Inheritance):**
  * **Người dùng:** lớp cơ sở `User` → `Admin`, `Staff`, `Customer`.
  * **Máy bay:** lớp cơ sở `Aircraft` → `WideBodyAircraft`, `NarrowBodyAircraft`, `TurbopropAircraft`.
  * **Con người:** `Person` → `Passenger` (và `Staff` → `Pilot`/`GroundStaff` cho mô hình nhân sự).
* **Đa hình (Polymorphism):** hàm ảo như `User::menu()` / `User::role()` (mỗi vai trò có giao diện riêng — đúng tinh thần `virtual showMenu()`), `Aircraft::requiredRunwayLength()` / `minTurnaroundMinutes()`.
* **Mẫu Factory:** `AircraftFactory::create(...)` khởi tạo máy bay theo phân loại; `auth::makeUser(...)` khởi tạo tài khoản theo vai trò.

**Lớp điều khiển trung tâm `AirportSystem`** là nơi chứa toàn bộ logic nghiệp vụ. Cả hai frontend (console & web) chỉ là lớp bọc mỏng gọi các phương thức công khai của nó.

---

## 4. Cấu trúc mã nguồn

```text
├── docs/images/          # Ảnh chụp màn hình giao diện
├── src/                  # Backend C++ (OOP)
│   ├── aircraft/         # Máy bay + AircraftFactory
│   ├── auth/             # User → Admin / Staff / Customer (đăng nhập, phân quyền)
│   ├── common/           # Tiện ích chung: DateTime, Enums, OpResult, Utils
│   ├── operations/       # Flight, Airport, Gate, Runway, Crew, Ticket, Baggage
│   ├── people/           # Person → Passenger; Staff → Pilot / GroundStaff
│   ├── system/           # AirportSystem — lớp điều khiển trung tâm
│   └── web/              # REST API + Web server (httplib), JSON thủ công
├── web/                  # Frontend (HTML / CSS / Vanilla JS)
│   ├── index.html        # Bố cục giao diện (đăng nhập + các tab)
│   ├── style.css         # Giao diện (dark theme)
│   └── app.js            # Xử lý sự kiện & gọi API
├── data/                 # Dữ liệu lưu trữ dạng văn bản (.txt, ngăn bởi '|')
├── build.sh              # Build app console  -> skygate(.exe)
├── build_web.sh          # Build web server   -> skygate_web(.exe)
├── deploy.ps1            # Script deploy lên VPS
└── MoSkyGate.bat         # Khởi động nhanh web server trên Windows
```

---

## 5. Hướng dẫn cài đặt và vận hành

### Yêu cầu
* Bộ biên dịch C++ hỗ trợ **C++17** (khuyến nghị **MSYS2 UCRT64 / g++ 15.x** trên Windows).

### Chạy nhanh (Windows)
1. Tải mã nguồn, mở thư mục `skygate/`.
2. Kích đúp `MoSkyGate.bat` — server chạy ở cổng `3000` và mở trình duyệt.

*(Thủ công: `./skygate_web.exe 8080` rồi mở `http://localhost:8080`.)*

### Tự biên dịch (Backend C++)
Trên Windows, đặt MSYS2 UCRT64 lên đầu PATH trước khi build:
```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
bash build_web.sh     # web server -> skygate_web.exe
bash build.sh         # app console -> skygate.exe
```

### Lưu trữ
Toàn bộ trạng thái (chuyến bay, hành khách, **tài khoản**, **vé**) được lưu thành các file văn bản trong thư mục `data/`. Web server tự nạp khi khởi động và **tự lưu sau mỗi thao tác** — dữ liệu được giữ lại qua các lần khởi động lại.

---

## 6. Thành viên thực hiện

* **Nguyễn Ngọc Trường Phi**
* **Nguyễn Thị Thúy Hằng**
* **Vũ Minh Quang**
* **Nguyễn Đức Lộc**
* **Phạm Gia Huy**
