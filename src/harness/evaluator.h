#ifndef EVALUATOR_H
#define EVALUATOR_H

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "../environment/environment.h"

// Strategy Interface: Bộ chấm điểm trừu tượng
class Evaluator {
public:
    virtual ~Evaluator() = default;

    /**
     * @brief Đánh giá kết quả của Agent dựa trên cấu hình task
     * @param agentOutput Kết quả chuỗi trả về từ AgentLoop
     * @param taskConfig Cấu hình JSON của task (chứa expected_keywords, eval_script...)
     * @return true nếu đạt yêu cầu, false nếu thất bại
     */
    virtual bool evaluate(const std::string& agentOutput, const nlohmann::json& taskConfig) = 0;
};

// Strategy Concretes: Chấm điểm dựa trên từ khóa (Keyword-based)
class KeywordEvaluator : public Evaluator {
private:
    bool _caseSensitive;

public:
    explicit KeywordEvaluator(bool caseSensitive = false);
    bool evaluate(const std::string& agentOutput, const nlohmann::json& taskConfig) override;
};

// Strategy Concretes: Chấm điểm dựa trên tác động thực tế trên hệ thống (Functional/Script-based)
class FunctionalEvaluator : public Evaluator {
private:
    std::shared_ptr<Environment> _env;
    int _timeoutSeconds;

public:
    explicit FunctionalEvaluator(std::shared_ptr<Environment> env = nullptr, int timeoutSeconds = 10);
    bool evaluate(const std::string& agentOutput, const nlohmann::json& taskConfig) override;
};

// Factory đơn giản để tạo Evaluator phù hợp dựa vào eval_type ("keyword" / "functional")
class EvaluatorFactory {
public:
    static std::unique_ptr<Evaluator> create(const std::string& evalType, std::shared_ptr<Environment> env = nullptr);
};

#endif // EVALUATOR_H