#include "Database.h"

#include <stdexcept>

#include "../../third_party/sqlite3.h"

namespace skygate {

// ===========================================================================
//  Stmt — prepared statement
// ===========================================================================

Database::Stmt& Database::Stmt::bindInt(int index, int value) {
    sqlite3_bind_int(stmt_, index, value);
    return *this;
}

Database::Stmt& Database::Stmt::bindDouble(int index, double value) {
    sqlite3_bind_double(stmt_, index, value);
    return *this;
}

Database::Stmt& Database::Stmt::bindText(int index, const std::string& value) {
    sqlite3_bind_text(stmt_, index, value.c_str(),
                      static_cast<int>(value.size()), SQLITE_TRANSIENT);
    return *this;
}

OpResult Database::Stmt::execute() {
    int rc = sqlite3_step(stmt_);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        return OpResult::failure("Lỗi thực thi câu lệnh: " +
                                 std::string(sqlite3_errmsg(sqlite3_db_handle(stmt_))));
    }
    sqlite3_reset(stmt_);
    return OpResult::success();
}

OpResult Database::Stmt::query(
    std::function<void(int argc, char** values, char** colnames)> callback) {
    int rc;
    while ((rc = sqlite3_step(stmt_)) == SQLITE_ROW) {
        int argc = sqlite3_column_count(stmt_);
        // sqlite3_column_name và sqlite3_column_text cần con trỏ char*
        // Nhưng callback signature dùng char** cho values và colnames.
        // Ta dùng vector tạm.
        std::vector<char*> vals(argc);
        std::vector<char*> cols(argc);
        for (int i = 0; i < argc; ++i) {
            vals[i] = const_cast<char*>(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt_, i)));
            cols[i] = const_cast<char*>(sqlite3_column_name(stmt_, i));
        }
        callback(argc, vals.data(), cols.data());
    }
    if (rc != SQLITE_DONE) {
        return OpResult::failure("Lỗi truy vấn: " +
                                 std::string(sqlite3_errmsg(sqlite3_db_handle(stmt_))));
    }
    sqlite3_reset(stmt_);
    return OpResult::success();
}

void Database::Stmt::finalize() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

// ===========================================================================
//  Database
// ===========================================================================

OpResult Database::open(const std::string& path) {
    if (db_) close();

    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "không rõ lỗi";
        close();
        return OpResult::failure("Không thể mở cơ sở dữ liệu: " + err);
    }

    // WAL mode — cho phép đọc/ghi đồng thời giữa nhiều process.
    execute("PRAGMA journal_mode=WAL;");
    // Khoá ngoại.
    execute("PRAGMA foreign_keys=ON;");
    // Cân bằng an toàn và hiệu năng trong WAL.
    execute("PRAGMA synchronous=NORMAL;");
    // Bảng tạm trong bộ nhớ.
    execute("PRAGMA temp_store=MEMORY;");
    // Cache 64 MB.
    execute("PRAGMA cache_size=-65536;");
    // Chờ tối đa 5 giây nếu database bị khoá bởi process khác.
    execute("PRAGMA busy_timeout=5000;");

    return OpResult::success("Đã mở cơ sở dữ liệu.");
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

OpResult Database::execute(const std::string& sql) {
    if (!db_) return OpResult::failure("Cơ sở dữ liệu chưa được mở.");

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "lỗi không xác định";
        sqlite3_free(errMsg);
        return OpResult::failure("Lỗi SQL: " + msg);
    }
    return OpResult::success();
}

OpResult Database::query(const std::string& sql,
                         std::function<void(int, char**, char**)> callback) {
    if (!db_) return OpResult::failure("Cơ sở dữ liệu chưa được mở.");

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        return OpResult::failure("Lỗi chuẩn bị truy vấn: " + err);
    }

    Stmt wrapper(stmt);
    OpResult r = wrapper.query(callback);
    wrapper.finalize();
    return r;
}

std::vector<std::vector<std::string>> Database::queryAll(const std::string& sql) {
    std::vector<std::vector<std::string>> rows;
    query(sql, [&](int argc, char** values, char** /*colnames*/) {
        std::vector<std::string> row;
        row.reserve(argc);
        for (int i = 0; i < argc; ++i) row.push_back(values[i] ? values[i] : "");
        rows.push_back(std::move(row));
    });
    return rows;
}

Database::Stmt Database::prepare(const std::string& sql) {
    if (!db_) {
        // Trả về Stmt rỗng — caller phải kiểm tra.
        return Stmt();
    }
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Stmt();
    }
    return Stmt(stmt);
}

long long Database::lastInsertRowId() const {
    return db_ ? sqlite3_last_insert_rowid(db_) : 0;
}

OpResult Database::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}

OpResult Database::commitTransaction() {
    return execute("COMMIT;");
}

OpResult Database::rollbackTransaction() {
    return execute("ROLLBACK;");
}

OpResult Database::transaction(std::function<OpResult()> fn) {
    OpResult r = beginTransaction();
    if (!r.ok) return r;
    r = fn();
    if (!r.ok) {
        rollbackTransaction();
        return r;
    }
    return commitTransaction();
}

}  // namespace skygate
