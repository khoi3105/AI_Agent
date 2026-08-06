#include "loop_detector.h"
#include <format>

LoopDetector::LoopDetector(int repeat_threshold, int ping_pong_threshold) :_repeat_threshold(repeat_threshold), _ping_pong_threshold(ping_pong_threshold) {}

std::string LoopDetector::make_signature(const std::string& tool_name, const nlohmann::json& args) const {
    // Chuẩn hóa thành dạng: tool_name::{"arg1":"val1"}
    return tool_name + "::" + args.dump();
}

void LoopDetector::reset() {
    _call_history.clear();
}

LoopCheckResult LoopDetector::checkLoop(const std::string& tool_name, const nlohmann::json& args) {
    std::string current_sig = make_signature(tool_name, args);
    _call_history.push_back(current_sig);

    size_t n = _call_history.size();

    // 1. Kiểm tra Generic Repeat (Ví dụ: A -> A -> A)
    if (n >= static_cast<size_t>(_repeat_threshold)) {
        bool is_generic_repeat = true;
        for (size_t i = n - _repeat_threshold; i < n - 1; ++i) {
            if (_call_history[i] != current_sig) {
                is_generic_repeat = false;
                break;
            }
        }

        if (is_generic_repeat) {
            return {
                LoopStatus::CRITICAL,
                std::format("[LOOP DETECTED] Generic Repeat: Tool '{}' được gọi liên tục {} lần với cùng tham số!", 
                            tool_name, _repeat_threshold)
            };
        }
    }

    int required_ping_pong_steps = _ping_pong_threshold * 2;
    if (n >= static_cast<size_t>(required_ping_pong_steps)) {
        std::string sig_B = _call_history[n - 1]; // Cuộc gọi vừa thực hiện (B)
        std::string sig_A = _call_history[n - 2]; // Cuộc gọi trước đó (A)

        if (sig_A != sig_B) { // Chỉ tính là Ping-Pong nếu 2 tool khác nhau hoặc tham số khác nhau
            bool is_ping_pong = true;
            for (int i = 0; i < _ping_pong_threshold; ++i) {
                size_t idx_b = n - 1 - (i * 2);
                size_t idx_a = n - 2 - (i * 2);
                if (_call_history[idx_b] != sig_B || _call_history[idx_a] != sig_A) {
                    is_ping_pong = false;
                    break;
                }
            }

            if (is_ping_pong) {
                return {
                    LoopStatus::CRITICAL,
                    std::format("[LOOP DETECTED] Ping-Pong Loop: Phát hiện chu kỳ lặp luân phiên giữa 2 cuộc gọi ({}) trong {} chu kỳ!", 
                                tool_name, _ping_pong_threshold)
                };
            }
        }
    }

    // 3. Cảnh báo sớm (Warning threshold - ví dụ lặp 2 lần liên tiếp)
    if (n >= 2 && _call_history[n - 1] == _call_history[n - 2]) {
        return {
            LoopStatus::WARNING,
            std::format("[LOOP WARNING] Tool '{}' vừa được gọi lặp lại 2 lần liên tiếp.", tool_name)
        };
    }

    return { LoopStatus::NONE, "" };
}

void LoopDetector::setRepeatThreshold(int threshold) { 
    _repeat_threshold = threshold; 
}

void LoopDetector::setPingPongthreshold(int threshold) {
     _ping_pong_threshold = threshold; 
}