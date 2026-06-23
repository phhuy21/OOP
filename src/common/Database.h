#ifndef SKYGATE_DATABASE_H
#define SKYGATE_DATABASE_H

#include <functional>
#include <string>
#include <vector>

#include "OpResult.h"

struct sqlite3;
struct sqlite3_stmt;

namespace skygate {

// ---------------------------------------------------------------------------
// Database: wrapper SQLite với WAL mode, tham số hoá và giao dịch.
//
// Thiết kế để tích hợp vào AirportSystem với pattern OpResult quen thuộc:
//   - Mọi thao tác trả về OpResult{ok, message} (tiếng Việt).
//   - Stmt cho phép bind tham số → execute / query an toàn.
//   - transaction(fn) tự động rollback nếu fn trả về failure.
// ---------------------------------------------------------------------------
class Database {
public:
    // --- Prepared statement (inner class) ---
    class Stmt {
    public:
        Stmt() = default;
        Stmt(sqlite3_stmt* stmt) : stmt_(stmt) {}

        Stmt& bindInt(int index, int value);
        Stmt& bindDouble(int index, double value);
        Stmt& bindText(int index, const std::string& value);

        // Thực thi INSERT / UPDATE / DELETE — trả về OpResult.
        OpResult execute();

        // Thực thi SELECT — gọi callback cho mỗi dòng kết quả.
        // callback(int argc, char** values, char** colnames)
        OpResult query(std::function<void(int argc, char** values, char** colnames)> callback);

        void finalize();

    private:
        sqlite3_stmt* stmt_ = nullptr;
    };

    // -----------------------------------------------------------------------
    Database() = default;
    ~Database() { close(); }

    // Mở / tạo database. Kích hoạt WAL + foreign keys + busy timeout.
    OpResult open(const std::string& path);
    void close();
    bool isOpen() const { return db_ != nullptr; }

    // SQL thô — dùng cho CREATE TABLE, PRAGMA, v.v.
    OpResult execute(const std::string& sql);

    // SELECT với callback cho từng dòng.
    OpResult query(const std::string& sql,
                   std::function<void(int argc, char** values, char** colnames)> callback);

    // SELECT trả về vector các dòng (mỗi dòng là vector<string>).
    std::vector<std::vector<std::string>> queryAll(const std::string& sql);

    // Prepare một câu lệnh có tham số (?1, ?2, ...).
    Stmt prepare(const std::string& sql);

    // ID của dòng vừa INSERT (hữu ích cho AUTOINCREMENT).
    long long lastInsertRowId() const;

    // Giao dịch.
    OpResult beginTransaction();
    OpResult commitTransaction();
    OpResult rollbackTransaction();

    // Chạy fn() trong transaction. Nếu fn() trả về failure → rollback.
    OpResult transaction(std::function<OpResult()> fn);

    // Con trỏ sqlite3 thô (cho nhu cầu đặc biệt).
    sqlite3* handle() const { return db_; }

private:
    sqlite3* db_ = nullptr;
};

}  // namespace skygate

#endif  // SKYGATE_DATABASE_H
