#ifndef AGENTLOOP_H
#define AGENTLOOP_H

#include <nlohmann/json.hpp>
#include "../client/llm_client.h"
#include "loop_detector.h"
#include <string>
#include <vector>

class AgentLoop{
private:
    nlohmann::json _conversationHistory;
    LoopDetector _loopdetector;
    static constexpr int MAXSTEP = 5;
public:
    AgentLoop(): _conversationHistory(nlohmann::json::array()){}
    std::expected<std::string, std::string> run(const std::string& user_task, const std::shared_ptr<LLMClient>& client, const std::vector<std::string>& image_paths = {});
};

#endif