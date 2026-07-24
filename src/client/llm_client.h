#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <string>

class LLMClient {
public:
    virtual std::string chat(const std::string& user_prompt) = 0;
    virtual ~LLMClient() = default;
};

#endif