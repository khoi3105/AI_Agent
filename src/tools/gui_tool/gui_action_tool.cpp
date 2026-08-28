#include "gui_action_tool.h"
#include <format>

GuiActionTool::GuiActionTool(const char* display) : _executor(display) {}

std::string GuiActionTool::getName() const {
    return "gui_action";
}

std::string GuiActionTool::getDescription() const {
    return "Thực thi các hành động điều khiển giao diện Desktop (chuột & bàn phím) qua libxdo: click, double_click, move, type_text, key_press, mouse_down, mouse_up.";
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
        int x = args["x"].get<int>();
        int y = args["y"].get<int>();
        int button = args.value("button", 1);

        auto res = _executor.click(x, y, button);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã click chuột thành công tại tọa độ ({}, {}) với button {}.", x, y, button);
    }
    else if (action == "double_click") {
        if (!args.contains("x") || !args.contains("y")) {
            return "[Lỗi gui_action]: Action 'double_click' yêu cầu cung cấp tọa độ 'x' và 'y'.";
        }
        int x = args["x"].get<int>();
        int y = args["y"].get<int>();
        int button = args.value("button", 1);

        auto res = _executor.doubleClick(x, y, button);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã double-click chuột thành công tại tọa độ ({}, {}).", x, y);
    }
    else if (action == "move") {
        if (!args.contains("x") || !args.contains("y")) {
            return "[Lỗi gui_action]: Action 'move' yêu cầu cung cấp tọa độ 'x' và 'y'.";
        }
        int x = args["x"].get<int>();
        int y = args["y"].get<int>();

        auto res = _executor.mouseMove(x, y);
        if (!res.has_value()) {
            return std::format("[Lỗi gui_action]: {}", res.error());
        }
        return std::format("Đã di chuyển con trỏ chuột đến tọa độ ({}, {}).", x, y);
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
            return "[Lỗi gui_action]: Action 'key_press' yêu cầu tham số chuỗi 'key' (ví dụ: 'Return', 'ctrl+c', 'ctrl+v', 'Escape', 'Tab', 'BackSpace').";
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
        return std::format("Vị trí chuột hiện tại: ({}, {})", res.value().first, res.value().second);
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
                        {"description", "Tọa độ X trên màn hình Desktop (bắt buộc cho click, double_click, move)."}
                    }},
                    {"y", {
                        {"type", "integer"},
                        {"description", "Tọa độ Y trên màn hình Desktop (bắt buộc cho click, double_click, move)."}
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
                        {"description", "Tên phím hoặc tổ hợp phím cần bấm, ví dụ: 'Return', 'ctrl+c', 'ctrl+v', 'ctrl+t', 'ctrl+l', 'Escape', 'Tab', 'BackSpace', 'Alt_L+F4' (bắt buộc cho key_press)."}
                    }}
                }},
                {"required", {"action"}}
            }}
        }}
    };
}
