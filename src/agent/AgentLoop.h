#ifndef AGENTLOOP_H
#define AGENTLOOP_H

#include <string>
#include <vector>
#include <functional>
#include <expected>
#include <memory>
#include <stop_token>
#include <nlohmann/json.hpp>

#include "step_data.h"
#include "task_plan.h"
#include "tool_call_parser.h"
#include "../client/llm_client.h"
#include "loop_detector.h"
#include "skill_loader.h"
#include "../tools/tool_registry.h"
#include "../tools/tool_policy.h"
#include "../environment/environment.h"

class AgentLoop {
protected:
    nlohmann::json _conversationHistory;
    LoopDetector _loopdetector;
    SkillLoader _skillLoader;
    int _maxstep{5};
    bool _enablePlanning{true}; // Cờ bật/tắt vòng đầu suy nghĩ (TaskPlan)

    // Dependency Injection
    std::shared_ptr<ToolRegistry> _registry;
    std::shared_ptr<ToolPolicy> _toolPolicy;
    std::shared_ptr<Environment> _env;

public:
    // Khai báo Callback Hook kiểu Observer
    using StepHook = std::function<void(const StepData&)>;

    explicit AgentLoop(
        int max_steps = 5, 
        std::string skills_dir = "skills",
        std::shared_ptr<ToolRegistry> registry = nullptr,
        std::shared_ptr<ToolPolicy> policy = nullptr,
        std::shared_ptr<Environment> env = nullptr
    );

    virtual ~AgentLoop() = default;

    // Chạy vòng lặp Agent ReAct với hỗ trợ stop_token (C++20 Cooperative Cancellation & Timeout)
    std::expected<std::string, std::string> run(
        const std::string& user_task, 
        const std::shared_ptr<LLMClient>& client, 
        const std::vector<std::string>& image_paths = {},
        std::stop_token stop_token = {}
    );

    // Getters & Setters
    void setStepHook(StepHook hook) {
        _stepHook = std::move(hook);
    }

    void setSkillsDir(const std::string& skills_dir) {
        _skillLoader = SkillLoader(skills_dir);
    }

    void setMaxSteps(int max_steps) {
        _maxstep = max_steps;
    }

    int getMaxSteps() const {
        return _maxstep;
    }

    void setEnablePlanning(bool enable) {
        _enablePlanning = enable;
    }

    bool isPlanningEnabled() const {
        return _enablePlanning;
    }

    void setEnvironment(std::shared_ptr<Environment> env) {
        _env = std::move(env);
    }

    std::shared_ptr<Environment> getEnvironment() const {
        return _env;
    }

    void setToolRegistry(std::shared_ptr<ToolRegistry> registry) {
        _registry = std::move(registry);
    }

    std::shared_ptr<ToolRegistry> getToolRegistry() const {
        return _registry;
    }

    void setToolPolicy(std::shared_ptr<ToolPolicy> policy) {
        _toolPolicy = std::move(policy);
    }

    SkillLoader& getSkillLoader() {
        return _skillLoader;
    }

protected:
    // Vòng đầu tiên: Lập kế hoạch suy nghĩ
    virtual std::expected<TaskPlan, std::string> plan(
        const std::string& user_task,
        const std::shared_ptr<LLMClient>& client,
        const std::vector<std::string>& image_paths,
        std::stop_token stop_token
    );

    virtual std::string prepareSystemPrompt(const ToolRegistry& registry, const std::string& user_task = "");
    virtual std::string preparePlanningPrompt(const ToolRegistry& registry, const std::string& user_task);
    virtual ToolCallRequest parseStepResponse(const std::string& raw_response);
    
    virtual std::expected<std::string, std::string> act(
        ToolRegistry& registry, 
        const std::string& tool_name, 
        const nlohmann::json& args
    );
    
    virtual void observe(
        const std::string& tool_name, 
        const std::expected<std::string, std::string>& tool_result, 
        StepData& out_step_data
    );
    
    virtual std::expected<std::string, std::string> formatFinalResponse(
        const ToolCallRequest& request
    );

private:
    StepHook _stepHook{nullptr};
};

#endif // AGENTLOOP_H
