#ifndef TASK_PLAN_H
#define TASK_PLAN_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

/**
 * @brief Đại diện cho một bước con trong kế hoạch tổng thể
 */
struct PlanStep {
    int stepIndex{1};
    std::string description;
    std::string suggestedTool{"auto"};
    bool isCompleted{false};

    nlohmann::json toJson() const {
        return {
            {"step", stepIndex},
            {"description", description},
            {"suggested_tool", suggestedTool},
            {"completed", isCompleted}
        };
    }
};

/**
 * @brief Class quản lý kế hoạch tư duy chiến lược của Agent (Plan-and-Solve / ReAct Initial Thinking)
 */
class TaskPlan {
private:
    std::string _goal;
    std::string _reasoning;
    std::vector<PlanStep> _steps;
    bool _isValid{false};

public:
    TaskPlan() = default;
    TaskPlan(std::string goal, std::string reasoning, std::vector<PlanStep> steps)
        : _goal(std::move(goal)), _reasoning(std::move(reasoning)), 
          _steps(std::move(steps)), _isValid(true) {}

    // Getters
    const std::string& getGoal() const { return _goal; }
    const std::string& getReasoning() const { return _reasoning; }
    const std::vector<PlanStep>& getSteps() const { return _steps; }
    bool isValid() const { return _isValid; }

    // Setters / Modifiers
    void setGoal(const std::string& goal) { _goal = goal; }
    void setReasoning(const std::string& reasoning) { _reasoning = reasoning; }
    void addStep(const PlanStep& step) { _steps.push_back(step); }
    void setValid(bool valid) { _isValid = valid; }

    // Parse phản hồi dạng JSON từ LLM thành TaskPlan
    static TaskPlan parse(const std::string& raw_response);

    // Chuyển TaskPlan thành định dạng Text chèn vào Context cho các vòng ReAct tiếp theo
    std::string toPromptContext() const;

    // Chuyển thành JSON (dùng cho Log, Trajectory, Debug)
    nlohmann::json toJson() const;

    // Định dạng in ra Console
    std::string toString() const;
};

#endif // TASK_PLAN_H
