#pragma once

#include <memory>

#include "hiredis.h"
#include <string>
#include <vector>

enum class RedisType {
    STRING = REDIS_REPLY_STRING,
    ARRAY = REDIS_REPLY_ARRAY,
    INTEGER = REDIS_REPLY_INTEGER,
    NIL = REDIS_REPLY_NIL,
    STATUS = REDIS_REPLY_STATUS,
    ERROR = REDIS_REPLY_ERROR,
    DOUBLE = REDIS_REPLY_DOUBLE,
    BOOL = REDIS_REPLY_BOOL,
    MAP = REDIS_REPLY_MAP,
    SET = REDIS_REPLY_SET,
    ATTR = REDIS_REPLY_ATTR,
    PUSH = REDIS_REPLY_PUSH,
    BIGNUM = REDIS_REPLY_BIGNUM,
    VERB = REDIS_REPLY_VERB,
};

class RedisReply {
public:
    RedisReply(const std::shared_ptr<redisReply>& rootReply, redisReply* reply);

    RedisType getType() const;
    bool is(RedisType type) const;
    void assertType(RedisType type) const;

    RedisReply& orThrow();

    /* AKA simple string*/
    std::string getStatus() const;
    std::string getString() const;
    long long getInt() const;
    std::string getError() const;
    std::vector<RedisReply> getArray() const;

private:
    static const char* getTypeName(RedisType type);
    // if this is an element of an array reply, the rootReply is the array.
    // It should not be deleted until all children no longer use it, since deletion is recursive.
    std::shared_ptr<redisReply> rootReply;
    redisReply* reply;
};
