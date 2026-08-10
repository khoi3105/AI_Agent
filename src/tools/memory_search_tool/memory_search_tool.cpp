#include "memory_search_tool.h"

#include <sstream>
#include <stdexcept>

MemorySearchTool::MemorySearchTool() : _db(nullptr) {
    const char* dbPath = "memory.db";
    if (sqlite3_open(dbPath, &_db) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(_db);
        sqlite3_close(_db);
        _db = nullptr;
        throw std::runtime_error(
            "Không thể mở cơ sở dữ liệu: " + error
        );
    }
}

MemorySearchTool::~MemorySearchTool() {
    if (_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

std::string MemorySearchTool::getName() const {
    return "memory_search";
}

std::string MemorySearchTool::getDescription() const {
    return "Search long-term memory for information relevant to a query.";
}

std::string MemorySearchTool::execute(const nlohmann::json& args) {
    try {
        if (!args.contains("query") || !args["query"].is_string()) {
            return "Error: 'query' is required and must be a string.";
        }

        std::string query = args["query"].get<std::string>();

        int limit = 5;

        if (args.contains("limit") && args["limit"].is_number_integer()) {
            limit = args["limit"].get<int>();
        }

        if (limit <= 0) {
            limit = 5;
        }

        const char* sql = R"(
            SELECT id, content, created_at
            FROM memories
            WHERE content LIKE ?
            ORDER BY id DESC
            LIMIT ?
        )";

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

        std::string pattern = "%" + query + "%";
        sqlite3_bind_text(
            stmt,
            1,
            pattern.c_str(),
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_int(
            stmt,
            2,
            limit
        );

        std::ostringstream output;
        bool found = false;
        while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
            found = true;
            int id = sqlite3_column_int(stmt, 0);
            const char* content =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 1)
                );
            const char* createdAt =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, 2)
                );

            output << "[Memory #" << id << "]\n";
            output << "Content: "
                   << (content ? content : "")
                   << "\n";

            output << "Created: "
                   << (createdAt ? createdAt : "")
                   << "\n\n";
        }

        if (result != SQLITE_DONE) {
            std::string error = sqlite3_errmsg(_db);
            sqlite3_finalize(stmt);

            return "SQLite search error: " + error;
        }
        sqlite3_finalize(stmt);
        if (!found) {
            return "Không tìm thấy truy vấn: " + query;
        }

        return output.str();

    } catch (const std::exception& e) {
        return "Invalid JSON: " + std::string(e.what());
    }
}

nlohmann::json MemorySearchTool::get_schema() const
{
    return {
        {"type", "function"},
        {"function", {
            {"name", "memory_search"},
            {"description",
                "Search long-term memory for information relevant to a query."},

            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"query", {
                        {"type", "string"},
                        {"description",
                            "The keyword or phrase to search for."}
                    }},
                    {"limit", {
                        {"type", "integer"},
                        {"description",
                            "Maximum number of memories to return."},
                        {"default", 5}
                    }}
                }},
                {"required", {"query"}}
            }}
        }}
    };
}