#pragma once

#include <format>

#include "hiredis.h"
#include "RedisReply.h"
#include <span>

class RedisClient {
public:
    static RedisClient connect(const char* host, int port);

    explicit RedisClient(const std::shared_ptr<redisContext>& context);

    RedisClient(const RedisClient& other) = delete;
    RedisClient(RedisClient&& other) = delete;
    RedisClient& operator=(const RedisClient& other) = delete;

    template<typename... Args>
    RedisReply run(const char* format, Args...) const;
    RedisReply run(const std::span<std::reference_wrapper<const std::string>>& args) const;

    template<typename... Args>
    void appendRun(const char* format, Args...) const;
    void appendRun(const std::span<std::reference_wrapper<const std::string>>& args) const;

    RedisReply getReply() const;

private:
    RedisReply makeReply(redisReply* reply) const;
    static std::tuple<std::unique_ptr<const char*[]>, std::unique_ptr<unsigned long[]>> makeArgsArrays(const std::span<std::reference_wrapper<const std::string>>& args);
    static void handleRedisErrorCode(int code);;

    std::shared_ptr<redisContext> context;
};

template <typename ... Args>
RedisReply RedisClient::run(const char* format, Args... args) const {
    const auto reply = static_cast<redisReply*>(redisCommand(context.get(), format, args...));
    return makeReply(reply);
}

template <typename ... Args>
void RedisClient::appendRun(const char* format, Args... args) const {
    const auto res = redisAppendCommand(context.get(), format, args...);
    handleRedisErrorCode(res);
}
