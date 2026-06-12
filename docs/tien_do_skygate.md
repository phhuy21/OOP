# SkyGate — Bảng tiến độ dự án (OOP)

> Hệ thống mô phỏng quản lý sân bay · C++ thuần OOP · Nhóm 5
> Cập nhật: 2026-06-09 · Nguồn: code thực tế trên đĩa (xác minh qua CodeGraph)

---

## 1. Tiến độ qua các giai đoạn

```mermaid
flowchart LR
    P1["1 · Phân tích đề tài"]:::done
    P2["2 · Thiết kế lớp (OOP)"]:::done
    P3["3 · Cài đặt code"]:::done
    P4["4 · Kiểm thử"]:::doing
    P5["5 · Báo cáo & bảo vệ"]:::doing

    P1 --> P2 --> P3 --> P4 --> P5

    classDef done   fill:#1f7a33,stroke:#0d3b18,color:#fff;
    classDef doing  fill:#c98a00,stroke:#5e4000,color:#fff;
    classDef todo   fill:#555,stroke:#222,color:#ccc;
```

Chú thích: 🟩 xong · 🟧 đang làm · ⬛ chưa bắt đầu

---

## 2. Sơ đồ Khung Dự án & Quy trình Phát triển (Top-Down Flow)

Dưới đây là sơ đồ chi tiết về cấu trúc hệ thống từ trên xuống và quy trình thực hiện dự án:

### A. Sơ đồ Kiến trúc Hệ thống (Top-Down System Architecture)
Sơ đồ này mô tả cách các thành phần trong dự án SkyGate tương tác với nhau, từ giao diện người dùng đến mã nguồn lõi C++ và lưu trữ:

```mermaid
graph TD
    classDef orchestrator fill:#f9f9f9,stroke:#333,stroke-width:2px;
    classDef check fill:#e1f5fe,stroke:#0288d1,stroke-width:1px;
    classDef data fill:#fff3e0,stroke:#f57c00,stroke-width:1px;
    classDef success fill:#e8f5e9,stroke:#388e3c,stroke-width:2px;
    classDef fail fill:#ffebee,stroke:#d32f2f,stroke-width:2px;

    SYS["<b>AirportSystem</b><br/>(Lớp điều khiển chính)"]:::orchestrator

    %% Khoi tao
    SYS -->|"1. Lap lich Chuyen bay"| Create["Tao doi tuong <b>Flight</b>"]
    
    %% Kiem dinh co so vat chat
    Create -->|"2. Kiem tra San bay & Duong bang"| CheckRunway{"<b>Kiem tra Runway:</b><br/>Chieu dai Runway >=<br/>Yeu cau cua Aircraft?"}:::check
    CheckRunway -->|Khong dat| FailRunway["Tu choi lap lich<br/>(Loi: Runway qua ngan)"]:::fail
    
    CheckRunway -->|Dat| CheckGate{"<b>Kiem tra Gate:</b><br/>San bay co Gate trong<br/>du hang cho Aircraft?"}:::check
    CheckGate -->|Khong dat| FailGate["Tu choi lap lich<br/>(Loi: Khong co Gate phu hop)"]:::fail

    %% Phan cong nhan su
    CheckGate -->|Dat| AssignCrew["3. Phan cong To bay (Crew)"]
    AssignCrew --> CheckPilot{"<b>Kiem tra Phi cong:</b><br/>1. Co chung chi lai loai Aircraft này?<br/>2. Tong gio bay thang nay <= 100h?<br/>3. Thoi gian nghi giua 2 chuyen >= 8h?"}:::check
    CheckPilot -->|Khong dat| FailCrew["Tu choi phan cong<br/>(Loi: Vi pham quy dinh an toan)"]:::fail

    %% Dat ve & check-in
    CheckPilot -->|Dat| Book["4. Dat ve & Lam thu tuc"]
    Book --> CheckSeat{"Hanh khach chon ghe?<br/>(Kiem tra ghe trong trong Capacity)"}:::check
    CheckSeat -->|Da co nguoi| FailSeat["Yeu cau chon ghe khac"]:::fail
    
    CheckSeat -->|Trong| CheckLuggage["Can hanh ly & Khai bao Baggage<br/>(Lien ket hanh ly voi Passenger)"]:::data

    %% Van hanh
    CheckLuggage --> Board["5. Len may bay (Boarding)<br/>Chuyen trang thai: BOARDED"]
    Board --> Depart["6. Cat canh (Departed)<br/>Chuyen trang thai: DEPARTED"]
    
    %% Dieu kien bat thuong
    Depart --> SimWeather{"<b>Giam sat Thoi tiet:</b><br/>Neu thoi tiet chuyen xau (Bao)?"}:::check
    SimWeather -->|Bao/Suong mu nang| Cancel["Huy chuyen / Tri hoan<br/>(CANCELLED / DELAYED)"]:::fail
    SimWeather -->|Thoi tiet binh thuong| Arrive["7. Ha canh an toan<br/>Chuyen trang thai: ARRIVED"]:::success
```


### B. Quy trình thực hiện qua các giai đoạn (Development Workflow)
Sơ đồ mô tả quy trình xây dựng dự án từ lúc nhận đề bài cho đến khi hoàn thành đóng gói:

```mermaid
flowchart TD
    Phase1["<b>Bước 1: Phân tích Đề tài</b><br/>- Trích xuất yêu cầu từ đề bài<br/>- Xác định các thực thể và ràng buộc nghiệp vụ"] --> Phase2
    
    Phase2["<b>Bước 2: Thiết kế sơ đồ lớp (UML)</b><br/>- Thiết kế các lớp kế thừa Person, Aircraft<br/>- Xác định quan hệ Composition, Association"] --> Phase3
    
    Phase3["<b>Bước 3: Code Core C++ OOP</b><br/>- Viết các file .h và .cpp ở thư mục src/<br/>- Cài đặt thuật toán kiểm tra ràng buộc (giờ bay, runway)"] --> Phase4
    
    Phase4["<b>Bước 4: Cài đặt Web API (Crow C++)</b><br/>- Tạo server lắng nghe cổng 8888<br/>- Chuyển đổi dữ liệu C++ thành định dạng JSON để trả về Web"] --> Phase5
    
    Phase5["<b>Bước 5: Phát triển Giao diện Web</b><br/>- Thiết kế giao diện Glassmorphism (HTML/CSS)<br/>- Viết JS gọi API cập nhật trạng thái thời tiết, chọn ghế"] --> Phase6
    
    Phase6["<b>Bước 6: Kiểm thử & Đóng gói</b><br/>- Chạy script kiểm thử tự động (test_run.py)<br/>- Đóng gói file chạy độc lập (.exe hoặc Docker)<br/>- Chuẩn bị báo cáo tài liệu (.docx/.md)"]
```

---

## 3. Trạng thái chi tiết


| # | Giai đoạn | Trạng thái | Bằng chứng |
|---|-----------|-----------|------------|
| 1 | Phân tích đề tài | ✅ Xong | `requirements.txt` đã trích đủ yêu cầu (sân bay, chuyến bay, máy bay, người, gate, runway, hành lý) |
| 2 | Thiết kế lớp | ✅ Xong | Cây kế thừa `Person`/`Staff`, `Aircraft` đã định hình; comment thiết kế trong mỗi header |
| 3 | Cài đặt code | ✅ Xong | Đủ `.h`/`.cpp` trong `skygate/src/`; build ra `skygate.exe` |
| 4 | Kiểm thử | 🟧 Đang làm | Có `test_run.py` + exe; **chưa xác minh test pass** |
| 5 | Báo cáo & bảo vệ | 🟧 Đang làm | Có `.docx` đề xuất/đề tài; chưa chốt bản cuối |

---

## 4. Sơ đồ lớp (sản phẩm của giai đoạn 2 — Thiết kế)

```mermaid
classDiagram
    class Person {
        <<abstract>>
        #id_ : string
        #fullName_ : string
        #age_ : int
        +role()* string
        +describe() string
    }
    class Staff {
        <<abstract>>
        #baseAirport_ : string
        +staffRole()* StaffRole
    }
    class Pilot {
        -certifications_ : set
        -monthlyFlightHours_ : double
        +isCertifiedFor() bool
        +hasEnoughRestBefore() bool
    }
    class GroundStaff {
        -department_ : string
    }
    class Passenger {
        -passport_ : string
        -checkedIn_ : bool
        -boarded_ : bool
    }
    class Baggage

    Person <|-- Staff
    Person <|-- Passenger
    Staff  <|-- Pilot
    Staff  <|-- GroundStaff
    Passenger *-- Baggage : có một

    class Aircraft {
        <<abstract>>
        #registration_ : string
        #capacity_ : int
        +category()* AircraftCategory
        +requiredRunwayLength()* int
        +minTurnaroundMinutes()* int
    }
    class NarrowBodyAircraft {
        runway >= 2500m
        turnaround 45'
    }
    class WideBodyAircraft {
        runway >= 2800m
        turnaround 90'
    }
    class TurbopropAircraft {
        runway >= 1500m
        turnaround 30'
    }
    Aircraft <|-- NarrowBodyAircraft
    Aircraft <|-- WideBodyAircraft
    Aircraft <|-- TurbopropAircraft
```

---

## 5. Lớp điều khiển & luồng nghiệp vụ chính

`AirportSystem` là lớp điều khiển trung tâm, nắm các danh sách đối tượng và
điều phối nghiệp vụ. Luồng vòng đời một chuyến bay:

```mermaid
flowchart TD
    A["createFlight()"] --> B["assignAircraft()"]
    B --> C["assignCrew()"]
    C --> D["assignGate()<br/>(tự tìm gate phù hợp)"]
    D --> E["bookPassenger()"]
    E --> F["checkIn() + hành lý"]
    F --> G["board()"]
    G --> H["advanceFlight()"]
    H -. bất thường .-> X["delayFlight() / cancelFlight()<br/>simulateBadWeather()"]
```

Quy tắc nghiệp vụ được kiểm tra (theo requirements):
- Phi công: ≤ 100 giờ bay/tháng, nghỉ ≥ 8 tiếng giữa 2 chuyến, đúng chứng chỉ loại máy bay.
- Đường băng: chiều dài thực tế ≥ yêu cầu của máy bay, nếu không → từ chối lập lịch.
- Gate: phải đủ hạng (`minGateRank`) cho loại máy bay.

---

## 6. Việc còn lại

- [ ] Chạy `test_run.py`, ghi lại kết quả pass/fail vào mục Kiểm thử.
- [ ] Bổ sung test cho các quy tắc biên (giờ bay = 100, runway = đúng ngưỡng).
- [ ] Hoàn thiện báo cáo `.docx`: chèn 3 sơ đồ ở trên.
- [ ] Chuẩn bị kịch bản demo khi bảo vệ (seed dữ liệu mẫu qua `seedDemoData()`).
