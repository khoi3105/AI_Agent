#ifndef LLM_CLIENT_FACTORY_H
#define LLM_CLIENT_FACTORY_H

#include "llm_client.h"
#include <memory>
#include <string>

/**
 * @brief Factory class to create appropriate LLMClient instance (Ollama, Gemini, OpenAI, NIM)
 * Implements Factory Pattern for clean OOP design and extensibility.
 */
class LLMClientFactory {
public:
    static std::shared_ptr<LLMClient> createClient(
        const std::string& model,
        const std::string& baseURL,
        const std::string& apiKey = ""
    );
};

#endif // LLM_CLIENT_FACTORY_H
