#pragma once

#include "hiredis.h"
#include <string>

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
    explicit RedisReply(redisReply* reply);
    ~RedisReply();

    RedisReply(const RedisReply& other) = delete;
    RedisReply(RedisReply&& other) = delete;
    RedisReply& operator=(const RedisReply& other) = delete;

    RedisType getType() const;
    bool is(RedisType type) const;
    void assertType(RedisType type) const;

    RedisReply& orThrow();

    /* AKA simple string*/
    std::string getStatus() const;
    std::string getString() const;
    long long getInt() const;
    std::string getError() const;

private:
    static const char* getTypeName(RedisType type);
    redisReply* reply;
};
