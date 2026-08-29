#ifndef XDO_EXECUTOR_H
#define XDO_EXECUTOR_H

#include <string>
#include <vector>
#include <utility>
#include <expected>

extern "C" {
#include <xdo.h>
}

/**
 * @brief Lớp bao bọc (Wrapper) thư viện C libxdo theo chuẩn RAII trong C++.
 * Quản lý vòng đời của con trỏ xdo_t* và cung cấp các hàm điều khiển chuột/phím an toàn.
 */
class XdoExecutor {
private:
    xdo_t* _xdo{nullptr};

public:
    explicit XdoExecutor(const char* display = nullptr);
    ~XdoExecutor();

    // Vô hiệu hóa copy constructor / assignment để đảm bảo quản lý tài nguyên duy nhất
    XdoExecutor(const XdoExecutor&) = delete;
    XdoExecutor& operator=(const XdoExecutor&) = delete;

    // Cho phép move semantics
    XdoExecutor(XdoExecutor&& other) noexcept;
    XdoExecutor& operator=(XdoExecutor&& other) noexcept;

    bool isValid() const { return _xdo != nullptr; }

    std::expected<void, std::string> click(int x, int y, int button = 1);
    std::expected<void, std::string> doubleClick(int x, int y, int button = 1);
    std::expected<void, std::string> mouseMove(int x, int y);
    std::expected<void, std::string> mouseDown(int button = 1);
    std::expected<void, std::string> mouseUp(int button = 1);
    std::expected<void, std::string> typeText(const std::string& text, unsigned int delay_microsec = 12000);
    std::expected<void, std::string> keyPress(const std::string& key, unsigned int delay_microsec = 12000);
    std::expected<std::pair<int, int>, std::string> getMouseLocation() const;
    std::pair<int, int> getScreenSize() const;
};

#endif // XDO_EXECUTOR_H
