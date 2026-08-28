#include "memory_search_tool.h"

#include <sstream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>

MemorySearchTool::MemorySearchTool() : _db(nullptr) {
    const char* dbPath = "memory.db";
    if (sqlite3_open(dbPath, &_db) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(_db);
        sqlite3_close(_db);
        _db = nullptr;
        throw std::runtime_error("Không thể mở cơ sở dữ liệu: " + error);
    }
    initDatabase();
}

MemorySearchTool::~MemorySearchTool() {
    if (_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

bool MemorySearchTool::initDatabase() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS memories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            content TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    char* errorMessage = nullptr;
    int result = sqlite3_exec(_db, sql, nullptr, nullptr, &errorMessage);
    if (result != SQLITE_OK) {
        if (errorMessage) sqlite3_free(errorMessage);
        return false;
    }
    return true;
}

std::string MemorySearchTool::getName() const {
    return "memory_search";
}

std::string MemorySearchTool::getDescription() const {
    return "Search long-term memory for previously saved facts, notes, or user-specific information.";
}

// Tách chuỗi truy vấn thành danh sách các từ khóa
std::vector<std::string> MemorySearchTool::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    while (iss >> token) {
        // Loại bỏ các dấu câu cơ bản
        token.erase(std::remove_if(token.begin(), token.end(), [](char c) {
            return c == ',' || c == '.' || c == '?' || c == '!' || c == ':' || c == ';' || c == '\'' || c == '\"';
        }), token.end());

        if (!token.empty() && token.length() >= 2) { // Bỏ qua ký tự đơn lẻ
            tokens.push_back(token);
        }
    }
    return tokens;
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
        if (limit <= 0) limit = 5;

        // =====================================================================
        // TẦNG 1: TÌM KIẾM CHÍNH XÁC CỤM TỪ (EXACT PHRASE MATCH)
        // =====================================================================
        const char* exactSql = R"(
            SELECT id, content, created_at
            FROM memories
            WHERE content LIKE ?
            ORDER BY id DESC
            LIMIT ?
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(_db, exactSql, -1, &stmt, nullptr) == SQLITE_OK) {
            std::string pattern = "%" + query + "%";
            sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, limit);

            std::ostringstream output;
            bool found = false;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                found = true;
                int id = sqlite3_column_int(stmt, 0);
                const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

                output << "[Memory #" << id << "]\n";
                output << "Content: " << (content ? content : "") << "\n";
                output << "Created: " << (createdAt ? createdAt : "") << "\n\n";
            }
            sqlite3_finalize(stmt);

            if (found) {
                return output.str();
            }
        }

        // =====================================================================
        // TẦNG 2: TÌM KIẾM TÁCH TỪ & CHẤM ĐIỂM XẾP HẠNG (RELEVANCE SCORING)
        // =====================================================================
        auto tokens = tokenize(query);
        if (tokens.empty()) {
            return "Không tìm thấy thông tin phù hợp với truy vấn: " + query;
        }

        // Xây dựng công thức tính điểm trùng khớp: ((content LIKE ?) + (content LIKE ?) + ...) AS match_score
        std::ostringstream scoreExpr;
        std::ostringstream whereClause;

        scoreExpr << "(";
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) {
                scoreExpr << " + ";
                whereClause << " OR ";
            }
            scoreExpr << "(content LIKE ?)";
            whereClause << "content LIKE ?";
        }
        scoreExpr << ") AS match_score";

        std::string sqlStr = "SELECT id, content, created_at, " + scoreExpr.str() + 
                             " FROM memories WHERE " + whereClause.str() + 
                             " ORDER BY match_score DESC, id DESC LIMIT ?;";

        if (sqlite3_prepare_v2(_db, sqlStr.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return "SQLite prepare error: " + std::string(sqlite3_errmsg(_db));
        }

        int paramIndex = 1;
        // Bind các tham số cho biểu thức match_score
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::string tokenPattern = "%" + tokens[i] + "%";
            sqlite3_bind_text(stmt, paramIndex++, tokenPattern.c_str(), -1, SQLITE_TRANSIENT);
        }
        // Bind các tham số cho mệnh đề WHERE
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::string tokenPattern = "%" + tokens[i] + "%";
            sqlite3_bind_text(stmt, paramIndex++, tokenPattern.c_str(), -1, SQLITE_TRANSIENT);
        }
        // Bind limit
        sqlite3_bind_int(stmt, paramIndex, limit);

        std::ostringstream tokenOutput;
        bool tokenFound = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            tokenFound = true;
            int id = sqlite3_column_int(stmt, 0);
            const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

            tokenOutput << "[Memory #" << id << "]\n";
            tokenOutput << "Content: " << (content ? content : "") << "\n";
            tokenOutput << "Created: " << (createdAt ? createdAt : "") << "\n\n";
        }
        sqlite3_finalize(stmt);

        if (!tokenFound) {
            return "Không tìm thấy thông tin nào trong bộ nhớ liên quan đến: " + query;
        }

        return tokenOutput.str();

    } catch (const std::exception& e) {
        return "MemorySearch Exception: " + std::string(e.what());
    }
}

nlohmann::json MemorySearchTool::get_schema() const {
    return {
        {"type", "tool_call"},
        {"tool_call", {
            {"name", "memory_search"},
            {"description", "Search long-term memory for previously saved notes, facts or personal information."},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"query", {
                        {"type", "string"},
                        {"description", "Keywords or phrase to search for (e.g., 'John', 'sinh nhật', 'C++23')."}
                    }},
                    {"limit", {
                        {"type", "integer"},
                        {"description", "Maximum number of memories to return (default: 5)."},
                        {"default", 5}
                    }}
                }},
                {"required", {"query"}}
            }}
        }}
    };
}
