#include "memory_save_tool.h"

#include <stdexcept>

MemorySaveTool::MemorySaveTool() : _db(nullptr) {
    const char* dbPath = "memory.db";
    if (sqlite3_open(dbPath, &_db) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(_db);
        sqlite3_close(_db);
        _db = nullptr;

        throw std::runtime_error(
            "Không thể mở SQLite database: " + error
        );
    }

    if (!initDatabase()) {
        sqlite3_close(_db);
        _db = nullptr;

        throw std::runtime_error(
            "Không thể khởi tạo cở sở dữ liệu"
        );
    }
}

MemorySaveTool::~MemorySaveTool()
{
    if (_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

bool MemorySaveTool::initDatabase() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS memories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            content TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    char* errorMessage = nullptr;
    int result = sqlite3_exec(
        _db,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        if (errorMessage) {
            sqlite3_free(errorMessage);
        }

        return false;
    }

    return true;
}

std::string MemorySaveTool::getName() const {
    return "memory_save";
}

std::string MemorySaveTool::getDescription() const {
    return "Save an important piece of information into long-term memory.";
}

std::string MemorySaveTool::execute(const nlohmann::json& args)
{
    try {
        if (!args.contains("content") || !args["content"].is_string()) {
            return "Error: 'content' is required and must be a string.";
        }

        std::string content = args["content"].get<std::string>();

        if (content.empty()) {
            return "Error: Nội dung không được để trống.";
        }

        const char* sql =
            "INSERT INTO memories (content) VALUES (?);";

        sqlite3_stmt* stmt = nullptr;

        int result = sqlite3_prepare_v2(
            _db,
            sql,
            -1,
            &stmt,
            nullptr
        );

        if (result != SQLITE_OK) {
            return "SQLite prepare error: " +
                   std::string(sqlite3_errmsg(_db));
        }

        sqlite3_bind_text(
            stmt,
            1,
            content.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        result = sqlite3_step(stmt);

        if (result != SQLITE_DONE) {
            std::string error = sqlite3_errmsg(_db);
            sqlite3_finalize(stmt);

            return "SQLite insert error: " + error;
        }

        sqlite3_finalize(stmt);

        return "Lưu thành công.";

    } catch (const std::exception& e) {
        return "Invalid JSON: " + std::string(e.what());
    }
}

nlohmann::json MemorySaveTool::get_schema() const
{
    return {
        {"type", "tool_call"},
        {"tool_call", {
            {"name", "memory_save"},
            {"description",
                "Save an important piece of information into long-term memory."},

            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"content", {
                        {"type", "string"},
                        {"description",
                            "The information that should be remembered."}
                    }}
                }},
                {"required", {"content"}}
            }}
        }}
    };
}
