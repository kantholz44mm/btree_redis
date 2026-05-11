#include "RedisClient.h"


#include <stdexcept>
#include <format>

RedisClient RedisClient::connect(const char* host, const int port) {
    const auto context = redisConnect(host, port);
    if (context == nullptr || context->err) {
        if (context) {
            throw std::runtime_error(std::format("Error connecting to redis: {}", context->errstr));
        }
        throw std::runtime_error("Error connecting to redis");
    }
    return RedisClient(context);
}

RedisClient::RedisClient(redisContext* context): context(context) {}

RedisClient::~RedisClient() {
    redisFree(context);
}

