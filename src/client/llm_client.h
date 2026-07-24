#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <string>

class LLMClient {
public:
    virtual std::string chat(std::string prompt) = 0;
};

#endif