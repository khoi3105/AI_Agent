#ifndef LOOP_DETECTOR_H
#define LOOP_DETECTOR_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

enum class LoopStatus {
    NONE,
    WARNING,
    CRITICAL
};

struct LoopCheckResult {
    LoopStatus status = LoopStatus::NONE;
    std::string message;
};

class LoopDetector {
private:
    std::vector<std::string> _call_history;
    int _repeat_threshold;
    int _ping_pong_threshold;  
    std::string make_signature(const std::string& tool_name, const nlohmann::json& args) const;
public:
    explicit LoopDetector(int repeat_threshold = 3, int ping_pong_threshold = 2);
    LoopCheckResult checkLoop(const std::string& tool_name, const nlohmann::json& args);
    void reset();
    void setRepeatThreshold(int threshold);
    void setPingPongthreshold(int threshold);
};

#endif 