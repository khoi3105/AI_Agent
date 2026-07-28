#include "env_utils.h"

std::string getEnvVar(const char* name) {
    const char* val = std::getenv(name);
    return (val != nullptr) ? std::string(val) : "";
}