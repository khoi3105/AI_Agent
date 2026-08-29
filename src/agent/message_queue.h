#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

/**
 * @brief Cấu trúc thông điệp trao đổi giữa các Agent (Inter-Agent Message)
 */
struct AgentMessage {
    std::string senderId;    // "Worker_A", "Worker_B", "Coordinator"
    std::string receiverId;  // "Coordinator", "all"
    std::string content;     // Nội dung trả về / chỉ thị
    nlohmann::json data{};   // Dữ liệu payload có cấu trúc kèm theo (nếu có)
    int64_t timestampMs{0};  // Dấu thời gian (Unix timestamp ms)
};

/**
 * @brief Hàng đợi thông điệp an toàn luồng (Thread-Safe Message Queue)
 * Sử dụng std::queue + std::mutex + std::condition_variable theo mô hình Producer-Consumer
 */
template <typename T>
class MessageQueue {
private:
    std::queue<T> _queue;
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    bool _isClosed{false};

public:
    MessageQueue() = default;
    ~MessageQueue() {
        close();
    }

    // Đẩy một thông điệp vào hàng đợi (Thread-safe)
    void push(T message) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_isClosed) return;
            _queue.push(std::move(message));
        }
        _cv.notify_one();
    }

    // Lấy thông điệp ra khỏi hàng đợi có Timeout (Thread-safe)
    std::optional<T> pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(200)) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_cv.wait_for(lock, timeout, [this]() { return !_queue.empty() || _isClosed; })) {
            if (_queue.empty()) return std::nullopt;
            T msg = std::move(_queue.front());
            _queue.pop();
            return msg;
        }
        return std::nullopt;
    }

    // Đóng hàng đợi và đánh thức toàn bộ thread đang chờ
    void close() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _isClosed = true;
        }
        _cv.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }
};

using AgentMessageQueue = MessageQueue<AgentMessage>;

#endif // MESSAGE_QUEUE_H
