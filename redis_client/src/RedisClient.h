#pragma once

#include <format>

#include "hiredis.h"
#include "RedisReply.h"
#include <vector>

class RedisClient {
public:
    static RedisClient connect(const char* host, int port);

    explicit RedisClient(const std::shared_ptr<redisContext>& context);

    RedisClient(const RedisClient& other) = delete;
    RedisClient(RedisClient&& other) = delete;
    RedisClient& operator=(const RedisClient& other) = delete;

    template<typename... Args>
    RedisReply run(const char* format, Args...) const;
    RedisReply run(const std::vector<std::string>& args) const;

private:
    RedisReply makeReply(redisReply* reply) const;

    std::shared_ptr<redisContext> context;
};

template <typename ... Args>
RedisReply RedisClient::run(const char* format, Args... args) const {
    const auto reply = static_cast<redisReply*>(redisCommand(context.get(), format, args...));
    return makeReply(reply);
}
