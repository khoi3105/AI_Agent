#ifndef AGENTLOOP_H
#define AGENTLOOP_H

#include <nlohmann/json.hpp>
#include "../client/llm_client.h"
#include <string>

class AgentLoop{
private:
    nlohmann::json _conversation_history;
public:
    static constexpr int MAXSTEP = 5;
    AgentLoop(): _conversation_history(nlohmann::json::array()){}
    std::string run(const std::string& user_task, const std::shared_ptr<LLMClient>& client);
};

#endif