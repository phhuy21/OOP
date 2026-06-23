// ===========================================================================
//  SkyGate Airport Management System - Chương trình chính (giao diện console)
//  Nhóm 5 - Lập trình hướng đối tượng
//
//  Áp dụng: Đóng gói, Kế thừa, Đa hình, Kết hợp/Thành phần.
//  Lưu trữ: file text đơn giản trong thư mục ./data.
// ===========================================================================
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "common/Enums.h"
#include "common/OpResult.h"
#include "system/AirportSystem.h"

using namespace skygate;

namespace {

const std::string kDataDir = "data";

// --------------------------- Tiện ích nhập liệu ---------------------------
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int readInt(const std::string& prompt, int fallback = 0) {
    std::string s = readLine(prompt);
    try {
        return std::stoi(s);
    } catch (...) {
        return fallback;
    }
}

double readDouble(const std::string& prompt, double fallback = 0.0) {
    std::string s = readLine(prompt);
    try {
        return std::stod(s);
    } catch (...) {
        return fallback;
    }
}

bool readYesNo(const std::string& prompt) {
    std::string s = readLine(prompt + " (y/n): ");
    return !s.empty() && (s[0] == 'y' || s[0] == 'Y');
}

void printResult(const OpResult& r) {
    std::cout << (r.ok ? "  [OK] " : "  [LỖI] ") << r.message << "\n";
}

void pause() {
    readLine("\nNhấn Enter để tiếp tục...");
}

GateType readGateType() {
    std::cout << "  Loại gate: 1) Ống lồng đôi  2) Ống lồng đơn  3) Bãi ngoài\n";
    int c = readInt("  Chọn: ", 3);
    if (c == 1) return GateType::DoubleJetBridge;
    if (c == 2) return GateType::SingleJetBridge;
    return GateType::RemoteStand;
}

AircraftCategory readCategory() {
    std::cout << "  Phân loại: 1) Thân rộng  2) Thân hẹp  3) Cánh quạt\n";
    int c = readInt("  Chọn: ", 2);
    if (c == 1) return AircraftCategory::WideBody;
    if (c == 3) return AircraftCategory::Turboprop;
    return AircraftCategory::NarrowBody;
}

// ------------------------------ Liệt kê -----------------------------------
void listAirports(const AirportSystem& sys) {
    std::cout << "\n--- DANH SÁCH SÂN BAY ---\n";
    if (sys.airports().empty()) { std::cout << "  (trống)\n"; return; }
    for (auto& a : sys.airports()) {
        std::cout << "  " << a->describe() << "\n";
        for (auto& r : a->runways()) std::cout << "      đường băng: " << r.describe() << "\n";
        for (auto& g : a->gates()) std::cout << "      gate: " << g.describe() << "\n";
    }
}

void listAircraft(const AirportSystem& sys) {
    std::cout << "\n--- DANH SÁCH MÁY BAY ---\n";
    if (sys.aircrafts().empty()) { std::cout << "  (trống)\n"; return; }
    for (auto& a : sys.aircrafts()) std::cout << "  " << a->describe() << "\n";
}

void listPeople(const AirportSystem& sys) {
    std::cout << "\n--- PHI CÔNG ---\n";
    for (auto& p : sys.pilots()) std::cout << "  " << p->describe() << "\n";
    if (sys.pilots().empty()) std::cout << "  (trống)\n";
    std::cout << "--- NHÂN VIÊN MẶT ĐẤT ---\n";
    for (auto& g : sys.groundStaff()) std::cout << "  " << g->describe() << "\n";
    if (sys.groundStaff().empty()) std::cout << "  (trống)\n";
    std::cout << "--- TỔ BAY ---\n";
    for (auto& c : sys.crews()) std::cout << "  " << c->describe() << "\n";
    if (sys.crews().empty()) std::cout << "  (trống)\n";
}

void listPassengers(const AirportSystem& sys) {
    std::cout << "\n--- DANH SÁCH HÀNH KHÁCH ---\n";
    if (sys.passengers().empty()) { std::cout << "  (trống)\n"; return; }
    for (auto& p : sys.passengers()) std::cout << "  " << p->describe() << "\n";
}

void listFlights(const AirportSystem& sys) {
    std::cout << "\n--- DANH SÁCH CHUYẾN BAY ---\n";
    if (sys.flights().empty()) { std::cout << "  (trống)\n"; return; }
    for (auto& f : sys.flights()) std::cout << "  " << f->describe() << "\n";
}

// ------------------------------ Submenu -----------------------------------
void menuAirports(AirportSystem& sys) {
    while (true) {
        std::cout << "\n=== QUẢN LÝ SÂN BAY / GATE / ĐƯỜNG BĂNG ===\n"
                  << "1) Thêm sân bay   2) Thêm đường băng   3) Thêm gate\n"
                  << "4) Xem danh sách   0) Quay lại\n";
        int c = readInt("Chọn: ", 0);
        if (c == 0) return;
        if (c == 1) {
            std::string code = readLine("  Mã sân bay: ");
            std::string name = readLine("  Tên sân bay: ");
            std::string note = readLine("  Ghi chú: ");
            printResult(sys.addAirport(code, name, note));
        } else if (c == 2) {
            std::string ap = readLine("  Mã sân bay: ");
            std::string rw = readLine("  Mã đường băng: ");
            int len = readInt("  Chiều dài (m): ");
            printResult(sys.addRunway(ap, rw, len));
        } else if (c == 3) {
            std::string ap = readLine("  Mã sân bay: ");
            std::string g = readLine("  Mã gate: ");
            GateType t = readGateType();
            printResult(sys.addGate(ap, g, t));
        } else if (c == 4) {
            listAirports(sys);
        }
        pause();
    }
}

void menuAircraft(AirportSystem& sys) {
    while (true) {
        std::cout << "\n=== QUẢN LÝ MÁY BAY ===\n"
                  << "1) Thêm máy bay   2) Xem danh sách   0) Quay lại\n";
        int c = readInt("Chọn: ", 0);
        if (c == 0) return;
        if (c == 1) {
            AircraftCategory cat = readCategory();
            std::string reg = readLine("  Số hiệu (registration): ");
            std::string model = readLine("  Model: ");
            int cap = readInt("  Sức chứa: ");
            printResult(sys.addAircraft(cat, reg, model, cap));
        } else if (c == 2) {
            listAircraft(sys);
        }
        pause();
    }
}

void menuPeople(AirportSystem& sys) {
    while (true) {
        std::cout << "\n=== QUẢN LÝ NHÂN SỰ & TỔ BAY ===\n"
                  << "1) Thêm phi công        2) Cấp chứng chỉ phi công\n"
                  << "3) Thêm NV mặt đất      4) Tạo tổ bay\n"
                  << "5) Thêm phi công vào tổ 6) Thêm NV vào tổ\n"
                  << "7) Xem danh sách        0) Quay lại\n";
        int c = readInt("Chọn: ", 0);
        if (c == 0) return;
        if (c == 1) {
            std::string id = readLine("  Mã phi công: ");
            std::string name = readLine("  Họ tên: ");
            int age = readInt("  Tuổi: ");
            std::string phone = readLine("  SĐT: ");
            std::string base = readLine("  Sân bay cơ sở: ");
            printResult(sys.addPilot(id, name, age, phone, base));
        } else if (c == 2) {
            std::string id = readLine("  Mã phi công: ");
            AircraftCategory cat = readCategory();
            printResult(sys.addPilotCertification(id, cat));
        } else if (c == 3) {
            std::string id = readLine("  Mã nhân viên: ");
            std::string name = readLine("  Họ tên: ");
            int age = readInt("  Tuổi: ");
            std::string phone = readLine("  SĐT: ");
            std::string base = readLine("  Sân bay cơ sở: ");
            std::string dep = readLine("  Bộ phận: ");
            printResult(sys.addGroundStaff(id, name, age, phone, base, dep));
        } else if (c == 4) {
            std::string id = readLine("  Mã tổ bay: ");
            printResult(sys.createCrew(id));
        } else if (c == 5) {
            std::string crew = readLine("  Mã tổ bay: ");
            std::string pid = readLine("  Mã phi công: ");
            printResult(sys.addPilotToCrew(crew, pid));
        } else if (c == 6) {
            std::string crew = readLine("  Mã tổ bay: ");
            std::string gid = readLine("  Mã nhân viên: ");
            printResult(sys.addGroundToCrew(crew, gid));
        } else if (c == 7) {
            listPeople(sys);
        }
        pause();
    }
}

void menuPassengers(AirportSystem& sys) {
    while (true) {
        std::cout << "\n=== QUẢN LÝ HÀNH KHÁCH & HÀNH LÝ ===\n"
                  << "1) Thêm hành khách   2) Cập nhật hành lý\n"
                  << "3) Xem danh sách     0) Quay lại\n";
        int c = readInt("Chọn: ", 0);
        if (c == 0) return;
        if (c == 1) {
            std::string id = readLine("  Mã hành khách: ");
            std::string name = readLine("  Họ tên: ");
            int age = readInt("  Tuổi: ");
            std::string phone = readLine("  SĐT: ");
            std::string pp = readLine("  Hộ chiếu: ");
            printResult(sys.addPassenger(id, name, age, phone, pp));
        } else if (c == 2) {
            std::string id = readLine("  Mã hành khách: ");
            int pieces = readInt("  Số kiện: ");
            double kg = readDouble("  Tổng khối lượng (kg): ");
            printResult(sys.setPassengerBaggage(id, pieces, kg));
        } else if (c == 3) {
            listPassengers(sys);
        }
        pause();
    }
}

void menuFlights(AirportSystem& sys) {
    while (true) {
        std::cout << "\n=== QUẢN LÝ CHUYẾN BAY ===\n"
                  << "1) Tạo chuyến bay     2) Gán máy bay\n"
                  << "3) Gán tổ bay         4) Gán gate (rỗng=tự động)\n"
                  << "5) Đặt chỗ hành khách 6) Check-in\n"
                  << "7) Boarding           8) Chuyển trạng thái kế tiếp\n"
                  << "9) Hoãn chuyến        10) Huỷ chuyến\n"
                  << "11) Xem danh sách     0) Quay lại\n";
        int c = readInt("Chọn: ", 0);
        if (c == 0) return;
        if (c == 1) {
            std::string code = readLine("  Mã chuyến: ");
            std::string o = readLine("  Sân bay đi: ");
            std::string d = readLine("  Sân bay đến: ");
            std::string dep = readLine("  Giờ đi (YYYY-MM-DD HH:MM): ");
            std::string arr = readLine("  Giờ đến (YYYY-MM-DD HH:MM): ");
            printResult(sys.createFlight(code, o, d, dep, arr));
        } else if (c == 2) {
            std::string f = readLine("  Mã chuyến: ");
            std::string r = readLine("  Số hiệu máy bay: ");
            printResult(sys.assignAircraft(f, r));
        } else if (c == 3) {
            std::string f = readLine("  Mã chuyến: ");
            std::string cr = readLine("  Mã tổ bay: ");
            printResult(sys.assignCrew(f, cr));
        } else if (c == 4) {
            std::string f = readLine("  Mã chuyến: ");
            std::string g = readLine("  Mã gate (Enter để tự động): ");
            printResult(sys.assignGate(f, g));
        } else if (c == 5) {
            std::string f = readLine("  Mã chuyến: ");
            std::string p = readLine("  Mã hành khách: ");
            printResult(sys.bookPassenger(f, p));
        } else if (c == 6) {
            std::string f = readLine("  Mã chuyến: ");
            std::string p = readLine("  Mã hành khách: ");
            std::string seat = readLine("  Số ghế: ");
            printResult(sys.checkIn(f, p, seat));
        } else if (c == 7) {
            std::string f = readLine("  Mã chuyến: ");
            std::string p = readLine("  Mã hành khách: ");
            printResult(sys.board(f, p));
        } else if (c == 8) {
            std::string f = readLine("  Mã chuyến: ");
            printResult(sys.advanceFlight(f));
        } else if (c == 9) {
            std::string f = readLine("  Mã chuyến: ");
            int m = readInt("  Số phút hoãn: ");
            std::string reason = readLine("  Lý do: ");
            printResult(sys.delayFlight(f, m, reason));
        } else if (c == 10) {
            std::string f = readLine("  Mã chuyến: ");
            std::string reason = readLine("  Lý do: ");
            printResult(sys.cancelFlight(f, reason));
        } else if (c == 11) {
            listFlights(sys);
        }
        pause();
    }
}

void menuDispatch(AirportSystem& sys) {
    while (true) {
        std::cout << "\n=== ĐIỀU PHỐI CẤT / HẠ CÁNH & KHẨN CẤP ===\n"
                  << "1) Đưa vào hàng đợi cất cánh   2) Đưa vào hàng đợi hạ cánh\n"
                  << "3) Xử lý cất cánh kế tiếp      4) Xử lý hạ cánh kế tiếp\n"
                  << "5) Đánh dấu khẩn cấp           6) Bỏ đánh dấu khẩn cấp\n"
                  << "7) Xem hàng đợi               0) Quay lại\n";
        int c = readInt("Chọn: ", 0);
        if (c == 0) return;
        if (c == 1) {
            printResult(sys.enqueueDeparture(readLine("  Mã chuyến: ")));
        } else if (c == 2) {
            printResult(sys.enqueueArrival(readLine("  Mã chuyến: ")));
        } else if (c == 3) {
            printResult(sys.processNextDeparture());
        } else if (c == 4) {
            printResult(sys.processNextArrival());
        } else if (c == 5) {
            sys.setMarkEmergency(readLine("  Mã chuyến: "), true);
            std::cout << "  [OK] Đã đánh dấu khẩn cấp.\n";
        } else if (c == 6) {
            sys.setMarkEmergency(readLine("  Mã chuyến: "), false);
            std::cout << "  [OK] Đã bỏ đánh dấu khẩn cấp.\n";
        } else if (c == 7) {
            std::cout << "  Hàng đợi cất cánh:";
            for (auto& s : sys.departureQueue()) std::cout << " " << s;
            std::cout << "\n  Hàng đợi hạ cánh:";
            for (auto& s : sys.arrivalQueue()) std::cout << " " << s;
            std::cout << "\n";
        }
        pause();
    }
}

void menuWeather(AirportSystem& sys) {
    std::cout << "\n=== MÔ PHỎNG THỜI TIẾT XẤU ===\n";
    std::string ap = readLine("  Sân bay ảnh hưởng: ");
    std::string s = readLine("  Bắt đầu (YYYY-MM-DD HH:MM): ");
    std::string e = readLine("  Kết thúc (YYYY-MM-DD HH:MM): ");
    bool cancel = readYesNo("  Huỷ chuyến thay vì hoãn?");
    int n = sys.simulateBadWeather(ap, s, e, cancel);
    if (n < 0) {
        std::cout << "  [LỖI] Tham số thời gian không hợp lệ.\n";
    } else {
        std::cout << "  [OK] Số chuyến bị ảnh hưởng: " << n
                  << (cancel ? " (đã huỷ)." : " (đã hoãn 120 phút).") << "\n";
    }
    pause();
}

void menuPersistence(AirportSystem& sys) {
    while (true) {
        std::cout << "\n=== LƯU / ĐỌC DỮ LIỆU ===\n"
                  << "1) Lưu ra file   2) Đọc từ file   0) Quay lại\n";
        int c = readInt("Chọn: ", 0);
        if (c == 0) return;
        if (c == 1) printResult(sys.saveAll(kDataDir));
        else if (c == 2) printResult(sys.loadAll(kDataDir));
        pause();
    }
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    AirportSystem sys;

    // Tự động đọc dữ liệu nếu đã có.
    if (std::ifstream exists(kDataDir + "/skygate.db"); exists.good()) {
        exists.close();
        OpResult r = sys.loadAll(kDataDir);
        std::cout << (r.ok ? "[Đã nạp dữ liệu sẵn có] " : "") << r.message << "\n";
    } else if (std::ifstream exists(kDataDir + "/airports.txt"); exists.good()) {
        exists.close();
        // Dữ liệu .txt cũ — loadAll sẽ tự động di trú sang SQLite.
        OpResult r = sys.loadAll(kDataDir);
        std::cout << (r.ok ? "[Đã chuyển đổi dữ liệu .txt sang SQLite] " : "") << r.message << "\n";
    } else if (readYesNo("Chưa có dữ liệu. Nạp dữ liệu mẫu?")) {
        sys.seedDemoData();
        std::cout << "[OK] Đã nạp dữ liệu mẫu.\n";
    }

    while (true) {
        std::cout << "\n==================== SKYGATE AMS ====================\n";
        std::cout << sys.summary() << "\n";
        std::cout << "-----------------------------------------------------\n"
                  << "1) Sân bay/Gate/Đường băng   2) Máy bay\n"
                  << "3) Nhân sự & Tổ bay          4) Hành khách & Hành lý\n"
                  << "5) Chuyến bay                6) Điều phối cất/hạ cánh\n"
                  << "7) Mô phỏng thời tiết xấu    8) Lưu / Đọc dữ liệu\n"
                  << "9) Nạp dữ liệu mẫu           0) Thoát\n";
        int c = readInt("Chọn: ", -1);
        switch (c) {
            case 1: menuAirports(sys); break;
            case 2: menuAircraft(sys); break;
            case 3: menuPeople(sys); break;
            case 4: menuPassengers(sys); break;
            case 5: menuFlights(sys); break;
            case 6: menuDispatch(sys); break;
            case 7: menuWeather(sys); break;
            case 8: menuPersistence(sys); break;
            case 9: sys.seedDemoData(); std::cout << "[OK] Đã nạp dữ liệu mẫu.\n"; pause(); break;
            case 0:
                if (readYesNo("Lưu dữ liệu trước khi thoát?")) {
                    printResult(sys.saveAll(kDataDir));
                }
                std::cout << "Tạm biệt!\n";
                return 0;
            default:
                std::cout << "  Lựa chọn không hợp lệ.\n";
        }
    }
}
