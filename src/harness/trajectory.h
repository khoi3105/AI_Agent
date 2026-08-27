#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <expected>

#include "../agent/step_data.h"

// Lớp quản lý toàn bộ quá trình thực thi và xuất log
class Trajectory {
private:
    std::string _taskId;
    std::string _modelName;
    std::string _instruction;
    std::vector<StepData> _steps;
    std::string _finalOutput;
    bool _isSuccess{false};
    int64_t _totalTimeMs{0};
    int _totalTokens{0};

public:
    Trajectory() = default;
    explicit Trajectory(std::string taskId, std::string modelName = "", std::string instruction = "");

    // Thêm một bước thực thi
    void addStep(const StepData& step);
    void addStep(StepData&& step);

    // Setters
    void setTaskId(const std::string& taskId);
    void setModelName(const std::string& modelName);
    void setInstruction(const std::string& instruction);
    void setFinalOutput(const std::string& finalOutput);
    void setSuccess(bool success);
    void setTotalTimeMs(int64_t totalMs);
    void setTotalTokens(int totalTokens);

    // Getters
    const std::string& getTaskId() const;
    const std::string& getModelName() const;
    const std::string& getInstruction() const;
    const std::vector<StepData>& getSteps() const;
    size_t getStepCount() const;
    const std::string& getFinalOutput() const;
    bool isSuccess() const;
    int64_t getTotalTimeMs() const;
    int getTotalTokens() const;

    // Chuyển toàn bộ dữ liệu nhật ký thành JSON
    nlohmann::json toJson() const;

    // Xuất nhật ký ra file JSON (ví dụ: trajectory_{task_id}.json)
    std::expected<bool, std::string> exportToJson(const std::string& filepath) const;
};

#endif // TRAJECTORY_H
