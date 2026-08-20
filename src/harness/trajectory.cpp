#include "trajectory.h"
#include <fstream>
#include <utility>

// ==========================================
// Implementation for StepData
// ==========================================

nlohmann::json StepData::toJson() const {
    nlohmann::json j;
    j["step_number"] = stepNumber;
    j["thought"] = thought;
    j["action"] = {
        {"tool", actionName},
        {"args", actionArgs}
    };
    j["observation"] = observation;
    j["latency_ms"] = latencyMs;
    j["tokens_used"] = tokensUsed;
    return j;
}

// ==========================================
// Implementation for Trajectory
// ==========================================

Trajectory::Trajectory(std::string taskId, std::string modelName, std::string instruction)
    : _taskId(std::move(taskId)),
      _modelName(std::move(modelName)),
      _instruction(std::move(instruction)) {}

void Trajectory::addStep(const StepData& step) {
    _steps.push_back(step);
    _totalTokens += step.tokensUsed;
}

void Trajectory::addStep(StepData&& step) {
    _totalTokens += step.tokensUsed;
    _steps.push_back(std::move(step));
}

// Setters
void Trajectory::setTaskId(const std::string& taskId) { _taskId = taskId; }
void Trajectory::setModelName(const std::string& modelName) { _modelName = modelName; }
void Trajectory::setInstruction(const std::string& instruction) { _instruction = instruction; }
void Trajectory::setFinalOutput(const std::string& finalOutput) { _finalOutput = finalOutput; }
void Trajectory::setSuccess(bool success) { _isSuccess = success; }
void Trajectory::setTotalTimeMs(int64_t totalMs) { _totalTimeMs = totalMs; }
void Trajectory::setTotalTokens(int totalTokens) { _totalTokens = totalTokens; }

// Getters
const std::string& Trajectory::getTaskId() const { return _taskId; }
const std::string& Trajectory::getModelName() const { return _modelName; }
const std::string& Trajectory::getInstruction() const { return _instruction; }
const std::vector<StepData>& Trajectory::getSteps() const { return _steps; }
size_t Trajectory::getStepCount() const { return _steps.size(); }
const std::string& Trajectory::getFinalOutput() const { return _finalOutput; }
bool Trajectory::isSuccess() const { return _isSuccess; }
int64_t Trajectory::getTotalTimeMs() const { return _totalTimeMs; }
int Trajectory::getTotalTokens() const { return _totalTokens; }

// Serialization
nlohmann::json Trajectory::toJson() const {
    nlohmann::json root;
    root["task_id"] = _taskId;
    root["model"] = _modelName;
    root["instruction"] = _instruction;
    root["success"] = _isSuccess;
    root["total_time_ms"] = _totalTimeMs;
    root["total_tokens"] = _totalTokens;
    root["step_count"] = _steps.size();

    nlohmann::json stepsArray = nlohmann::json::array();
    for (const auto& step : _steps) {
        stepsArray.push_back(step.toJson());
    }
    root["steps"] = stepsArray;
    root["final_output"] = _finalOutput;

    return root;
}

std::expected<bool, std::string> Trajectory::exportToJson(const std::string& filepath) const {
    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        return std::unexpected("Could not open file for writing: " + filepath);
    }

    try {
        outFile << toJson().dump(2);
        if (!outFile.good()) {
            return std::unexpected("Stream error occurred while writing to: " + filepath);
        }
    } catch (const std::exception& e) {
        return std::unexpected(std::string("JSON serialize/write error: ") + e.what());
    }

    return true;
}