#include "gui_action_tool.h"
#include <format>
#include <iostream>

GuiActionTool::GuiActionTool(const char* display) : _executor(display) {}

std::string GuiActionTool::getName() const {
    return "gui_action";
}

std::string GuiActionTool::getDescription() const {
    return "Thực thi các hành động điều khiển giao diện Desktop (chuột & bàn phím) qua libxdo: click, double_click, move, type_text, key_press, mouse_down, mouse_up. Tọa độ (x, y) sử dụng hệ chuẩn hóa [0, 1000].";
}

std::pair<int, int> GuiActionTool::resolveCoordinates(int raw_x, int raw_y) const {
    auto [screenWidth, screenHeight] = _executor.getScreenSize();

    // Nếu tọa độ nằm trong khoảng chuẩn hóa [0, 1000], tự động quy đổi sang pixel thực tế
    if (raw_x >= 0 && raw_x <= 1000 && raw_y >= 0 && raw_y <= 1000) {
        int actual_x = static_cast<int>((raw_x / 1000.0) * screenWidth);
        int actual_y = static_cast<int>((raw_y / 1000.0) * screenHeight);
        return {actual_x, actual_y};
    }

    // Nếu tọa độ đã vượt quá 1000 -> xem như tọa độ pixel tuyệt đối
    return {raw_x, raw_y};
}

std::string GuiActionTool::execute(const nlohmann::json& args) {
    if (!args.is_object() || !args.contains("action") || !args["action"].is_string()) {
        return "[Lỗi gui_action]: Thiếu tham số bắt buộc 'action' (click, double_click, move, type_text, key_press).";
    }

    std::string action = args["action"].get<std::string>();

    if (action == "click") {
        if (!args.contains("x") || !args.contains("y")) {
            return "[Lỗi gui_action]: Action 'click' yêu cầu cung cấp tọa độ 'x' và 'y'.";
        }
        int raw_x = args["x"].get<int>();
        int raw_y = args["y"].get<int>();
        auto [actual_x, actual_y] = resolveCoordinates(raw_x, raw_y);
        int button = args.value("button", 1);

        auto res = _executor.click(actual_x, actual_y, button);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã click chuột tại pixel ({}, {}) [Tọa độ gốc: x={}, y={}] với button {}.", 
                           actual_x, actual_y, raw_x, raw_y, button);
    }
    else if (action == "double_click") {
        if (!args.contains("x") || !args.contains("y")) {
            return "[Lỗi gui_action]: Action 'double_click' yêu cầu cung cấp tọa độ 'x' và 'y'.";
        }
        int raw_x = args["x"].get<int>();
        int raw_y = args["y"].get<int>();
        auto [actual_x, actual_y] = resolveCoordinates(raw_x, raw_y);
        int button = args.value("button", 1);

        auto res = _executor.doubleClick(actual_x, actual_y, button);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã double-click chuột tại pixel ({}, {}) [Tọa độ gốc: x={}, y={}].", 
                           actual_x, actual_y, raw_x, raw_y);
    }
    else if (action == "move") {
        if (!args.contains("x") || !args.contains("y")) {
            return "[Lỗi gui_action]: Action 'move' yêu cầu cung cấp tọa độ 'x' và 'y'.";
        }
        int raw_x = args["x"].get<int>();
        int raw_y = args["y"].get<int>();
        auto [actual_x, actual_y] = resolveCoordinates(raw_x, raw_y);

        auto res = _executor.mouseMove(actual_x, actual_y);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã di chuyển con trỏ chuột đến pixel ({}, {}) [Tọa độ gốc: x={}, y={}].", 
                           actual_x, actual_y, raw_x, raw_y);
    }
    else if (action == "type_text") {
        if (!args.contains("text") || !args["text"].is_string()) {
            return "[Lỗi gui_action]: Action 'type_text' yêu cầu tham số chuỗi 'text'.";
        }
        std::string text = args["text"].get<std::string>();
        int delay = args.value("delay_microsec", 12000);

        auto res = _executor.typeText(text, delay);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã gõ văn bản: \"{}\"", text);
    }
    else if (action == "key_press") {
        if (!args.contains("key") || !args["key"].is_string()) {
            return "[Lỗi gui_action]: Action 'key_press' yêu cầu tham số chuỗi 'key' (ví dụ: 'Return', 'Control_L+l', 'Control_L+t', 'Control_L+w', 'Escape', 'Tab', 'BackSpace').";
        }
        std::string key = args["key"].get<std::string>();
        int delay = args.value("delay_microsec", 12000);

        auto res = _executor.keyPress(key, delay);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã nhấn phím/tổ hợp phím: \"{}\"", key);
    }
    else if (action == "mouse_down") {
        int button = args.value("button", 1);
        auto res = _executor.mouseDown(button);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã nhấn giữ chuột button {}.", button);
    }
    else if (action == "mouse_up") {
        int button = args.value("button", 1);
        auto res = _executor.mouseUp(button);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã nhả chuột button {}.", button);
    }
    else if (action == "get_mouse_location") {
        auto res = _executor.getMouseLocation();
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        auto [screenWidth, screenHeight] = _executor.getScreenSize();
        int norm_x = static_cast<int>((res.value().first * 1000.0) / screenWidth);
        int norm_y = static_cast<int>((res.value().second * 1000.0) / screenHeight);
        return std::format("Vị trí chuột hiện tại: pixel ({}, {}) ~ chuẩn hóa [{}/1000, {}/1000] trên màn hình {}x{}", 
                           res.value().first, res.value().second, norm_x, norm_y, screenWidth, screenHeight);
    }

    return std::format("[Lỗi gui_action]: Không hỗ trợ action '{}'. Hỗ trợ: click, double_click, move, type_text, key_press, mouse_down, mouse_up, get_mouse_location.", action);
}

nlohmann::json GuiActionTool::get_schema() const {
    return {
        {"type", "tool_call"},
        {"tool_call", {
            {"name", getName()},
            {"description", getDescription()},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"action", {
                        {"type", "string"},
                        {"enum", {"click", "double_click", "move", "type_text", "key_press", "mouse_down", "mouse_up", "get_mouse_location"}},
                        {"description", "Loại hành động GUI cần thực thi."}
                    }},
                    {"x", {
                        {"type", "integer"},
                        {"description", "Tọa độ X chuẩn hóa từ 0 đến 1000 (0 = mép trái, 1000 = mép phải màn hình)."}
                    }},
                    {"y", {
                        {"type", "integer"},
                        {"description", "Tọa độ Y chuẩn hóa từ 0 đến 1000 (0 = mép trên, 1000 = mép dưới màn hình)."}
                    }},
                    {"button", {
                        {"type", "integer"},
                        {"description", "Nút chuột: 1 = chuột trái, 2 = chuột giữa, 3 = chuột phải (mặc định: 1)."}
                    }},
                    {"text", {
                        {"type", "string"},
                        {"description", "Chuỗi văn bản cần gõ vào ứng dụng đang focus (bắt buộc cho type_text)."}
                    }},
                    {"key", {
                        {"type", "string"},
                        {"description", "Tên phím hoặc tổ hợp phím cần bấm, ví dụ: 'Return', 'Control_L+l', 'Control_L+t', 'Control_L+w', 'Escape', 'Tab', 'BackSpace' (bắt buộc cho key_press)."}
                    }}
                }},
                {"required", {"action"}}
            }}
        }}
    };
}
