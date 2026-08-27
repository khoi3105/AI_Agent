#ifndef STEP_DATA_H
#define STEP_DATA_H

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

// Cấu trúc dữ liệu ghi nhận thông tin của từng bước (step) trong vòng lặp ReAct
struct StepData {
    int stepNumber{0};
    std::string thought;
    std::string actionName;
    nlohmann::json actionArgs;
    std::string observation;
    int64_t latencyMs{0};
    int tokensUsed{0};

    // Chuyển dữ liệu của 1 bước thành JSON
    nlohmann::json toJson() const;
};

#endif // STEP_DATA_H
