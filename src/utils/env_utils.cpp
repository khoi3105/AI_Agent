#include "env_utils.h"

std::string getEnvVar(const char* name, const std::string& defaultValue) {
    const char* val = std::getenv(name);
    return (val != nullptr) ? std::string(val) : defaultValue;
}