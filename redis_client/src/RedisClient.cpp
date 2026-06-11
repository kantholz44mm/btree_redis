#include "RedisClient.h"


#include <stdexcept>
#include <format>
#include <memory>

RedisClient RedisClient::connect(const char* host, const int port) {
    const auto context = redisConnect(host, port);
    if (context == nullptr || context->err) {
        if (context) {
            throw std::runtime_error(std::format("Error connecting to redis: {}", context->errstr));
        }
        throw std::runtime_error("Error connecting to redis");
    }
    auto contextPtr = std::shared_ptr<redisContext>(context, [](redisContext* context) {
        redisFree(context);
    });
    return RedisClient(contextPtr);
}

RedisClient::RedisClient(const std::shared_ptr<redisContext>& context): context(context) {}

RedisReply RedisClient::makeReply(redisReply* reply) const {
    if (reply == nullptr) {
        throw std::runtime_error(std::format("Error sending command (Code {}): {}", context->err, context->errstr));
    }
    const auto rootReply = std::shared_ptr<redisReply>(reply, [](redisReply* reply) {
        freeReplyObject(reply);
    });
    return RedisReply(rootReply, reply);
}

