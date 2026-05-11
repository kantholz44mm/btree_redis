#include "RedisReply.h"

#include <format>
#include <stdexcept>

RedisReply::RedisReply(redisReply* reply): reply(reply) {}

RedisReply::~RedisReply() {
    freeReplyObject(reply);
}

RedisType RedisReply::getType() const {
    return static_cast<RedisType>(reply->type);
}

bool RedisReply::is(const RedisType type) const {
    return getType() == type;
}

void RedisReply::assertType(const RedisType type) const {
    if (!is(type)) {
        throw std::runtime_error(std::format("Expected {}, got {}", getTypeName(type), getTypeName(getType())));
    }
}

std::string RedisReply::getStatus() const {
    assertType(RedisType::STATUS);
    return {reply->str};
}

std::string RedisReply::getString() const {
    assertType(RedisType::STRING);
    return {reply->str};
}

long long RedisReply::getInt() const {
    assertType(RedisType::INTEGER);
    return reply->integer;
}

std::string RedisReply::getError() const {
    assertType(RedisType::ERROR);
    return {reply->str};
}

const char* RedisReply::getTypeName(const RedisType type) {
    switch (type) {
        case RedisType::STRING: return "STRING";
        case RedisType::ARRAY: return "ARRAY";
        case RedisType::INTEGER: return "INTEGER";
        case RedisType::NIL: return "NIL";
        case RedisType::STATUS: return "STATUS";
        case RedisType::ERROR: return "ERROR";
        case RedisType::DOUBLE: return "DOUBLE";
        case RedisType::BOOL: return "BOOL";
        case RedisType::MAP: return "MAP";
        case RedisType::SET: return "SET";
        case RedisType::ATTR: return "ATTR";
        case RedisType::PUSH: return "PUSH";
        case RedisType::BIGNUM: return "BIGNUM";
        case RedisType::VERB: return "VERB";
    }
    return "UNKNOWN";
}
