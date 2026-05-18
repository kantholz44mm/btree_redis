#pragma once

#include <format>

#include "hiredis.h"
#include "RedisReply.h"
#include <stdexcept>

class RedisClient {
public:
    static RedisClient connect(const char* host, int port);

    explicit RedisClient(redisContext* context);
    ~RedisClient();

    RedisClient(const RedisClient& other) = delete;
    RedisClient(RedisClient&& other) = delete;
    RedisClient& operator=(const RedisClient& other) = delete;

    template<typename... Args>
    RedisReply run(const char* format, Args...);

private:
    redisContext* context;
};

template <typename ... Args>
RedisReply RedisClient::run(const char* format, Args... args) {
    const auto reply = static_cast<redisReply*>(redisCommand(context, format, args...));
    if (reply == nullptr) {
        throw std::runtime_error(std::format("Error sending command \"{}\" (Code {}): {}", format, context->err, context->errstr));
    }
    return RedisReply(reply);
}
