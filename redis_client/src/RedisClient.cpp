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

RedisReply RedisClient::run(const std::span<std::reference_wrapper<const std::string>>& args) const {
    const auto argv = std::make_unique<const char*[]>(args.size());
    const auto argc = std::make_unique<size_t[]>(args.size());
    auto argvIter = argv.get();
    auto argcIter = argc.get();
    for (const auto& arg : args) {
        *argvIter++ = arg.get().c_str();
        *argcIter++ = arg.get().size();
    }
    const auto reply = static_cast<redisReply*>(redisCommandArgv(context.get(), args.size(), argv.get(), argc.get()));
    return makeReply(reply);
}

RedisReply RedisClient::makeReply(redisReply* reply) const {
    if (reply == nullptr) {
        throw std::runtime_error(std::format("Error sending command (Code {}): {}", context->err, context->errstr));
    }
    const auto rootReply = std::shared_ptr<redisReply>(reply, [](redisReply* reply) {
        freeReplyObject(reply);
    });
    return RedisReply(rootReply, reply);
}

