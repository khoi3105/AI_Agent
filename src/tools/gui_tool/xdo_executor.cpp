#include "xdo_executor.h"
#include <format>

XdoExecutor::XdoExecutor(const char* display) {
    _xdo = xdo_new(display);
}

XdoExecutor::~XdoExecutor() {
    if (_xdo) {
        xdo_free(_xdo);
        _xdo = nullptr;
    }
}

XdoExecutor::XdoExecutor(XdoExecutor&& other) noexcept : _xdo(other._xdo) {
    other._xdo = nullptr;
}

XdoExecutor& XdoExecutor::operator=(XdoExecutor&& other) noexcept {
    if (this != &other) {
        if (_xdo) {
            xdo_free(_xdo);
        }
        _xdo = other._xdo;
        other._xdo = nullptr;
    }
    return *this;
}

std::expected<void, std::string> XdoExecutor::click(int x, int y, int button) {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao hoac khong the ket noi den X11 Display!");
    }
    if (xdo_move_mouse(_xdo, x, y, 0) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi di chuyen chuot den ({}, {})", x, y));
    }
    if (xdo_click_window(_xdo, CURRENTWINDOW, button) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi click chuot tai ({}, {}) voi button {}", x, y, button));
    }
    return {};
}

std::expected<void, std::string> XdoExecutor::doubleClick(int x, int y, int button) {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao hoac khong the ket noi den X11 Display!");
    }
    if (xdo_move_mouse(_xdo, x, y, 0) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi di chuyen chuot den ({}, {})", x, y));
    }
    if (xdo_click_window_multiple(_xdo, CURRENTWINDOW, button, 2, 50000) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi double-click tai ({}, {})", x, y));
    }
    return {};
}

std::expected<void, std::string> XdoExecutor::mouseMove(int x, int y) {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao!");
    }
    if (xdo_move_mouse(_xdo, x, y, 0) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi di chuyen chuot den ({}, {})", x, y));
    }
    return {};
}

std::expected<void, std::string> XdoExecutor::mouseDown(int button) {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao!");
    }
    if (xdo_mouse_down(_xdo, CURRENTWINDOW, button) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi nhan giu chuot button {}", button));
    }
    return {};
}

std::expected<void, std::string> XdoExecutor::mouseUp(int button) {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao!");
    }
    if (xdo_mouse_up(_xdo, CURRENTWINDOW, button) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi tha chuot button {}", button));
    }
    return {};
}

std::expected<void, std::string> XdoExecutor::typeText(const std::string& text, unsigned int delay_microsec) {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao!");
    }
    if (xdo_enter_text_window(_xdo, CURRENTWINDOW, text.c_str(), delay_microsec) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi go chuoi van ban: \"{}\"", text));
    }
    return {};
}

std::expected<void, std::string> XdoExecutor::keyPress(const std::string& key, unsigned int delay_microsec) {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao!");
    }
    if (xdo_send_keysequence_window(_xdo, CURRENTWINDOW, key.c_str(), delay_microsec) != XDO_SUCCESS) {
        return std::unexpected(std::format("Loi khi gui to hop / phim: \"{}\"", key));
    }
    return {};
}

std::expected<std::pair<int, int>, std::string> XdoExecutor::getMouseLocation() const {
    if (!_xdo) {
        return std::unexpected("libxdo chua duoc khoi tao!");
    }
    int x = 0;
    int y = 0;
    int screen = 0;
    if (xdo_get_mouse_location(_xdo, &x, &y, &screen) != XDO_SUCCESS) {
        return std::unexpected("Khong the lay toa do chuot hien tai!");
    }
    return std::make_pair(x, y);
}

std::pair<int, int> XdoExecutor::getScreenSize() const {
    if (!_xdo) {
        return {1920, 1080}; // Default fallback
    }
    unsigned int width = 0;
    unsigned int height = 0;
    if (xdo_get_viewport_dimensions(_xdo, &width, &height, 0) == XDO_SUCCESS && width > 0 && height > 0) {
        return {static_cast<int>(width), static_cast<int>(height)};
    }
    return {1920, 1080};
}

