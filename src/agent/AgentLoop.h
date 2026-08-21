#ifndef AGENTLOOP_H
#define AGENTLOOP_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "tool_call_parser.h"
#include "../client/llm_client.h"
#include "loop_detector.h"
#include "../harness/trajectory.h"
#include "../tools/tool_registry.h"

class AgentLoop{
protected:
    nlohmann::json _conversationHistory;
    LoopDetector _loopdetector;
    int _maxstep{5};
public:
    // Khai báo Callback Hook kiểu Observer
    using StepHook = std::function<void(const StepData&)>;
    explicit AgentLoop(int max_steps = 5): _maxstep(max_steps), _conversationHistory(nlohmann::json::array()) {}
    std::expected<std::string, std::string> run(const std::string& user_task, const std::shared_ptr<LLMClient>& client, const std::vector<std::string>& image_paths = {});
    void setStepHook(StepHook hook) {
        _stepHook = std::move(hook);
    }
    virtual ~AgentLoop() = default;
protected:
    virtual std::string prepareSystemPrompt(const ToolRegistry& registry);
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

#endif