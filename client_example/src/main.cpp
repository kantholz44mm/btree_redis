#include <cstdlib>

#include "hiredis.h"
#include <iostream>

int main() {

    // https://redis.io/docs/latest/develop/clients/hiredis/

    redisContext *c = redisConnect("127.0.0.1", 3000);
    if (c == nullptr || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
        std::exit(1);
    }

    const auto key = "example";

    auto reply = static_cast<redisReply*>(redisCommand(c, "SET %s %d", key, 5));
    std::cout << reply->str << std::endl;
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(c, "GET %s", key));
    std::cout << reply->str << std::endl;
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(c, "INCR %s", key));
    std::cout << reply->integer << std::endl;
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(c, "DECRBY %s %d", key, 3));
    std::cout << reply->integer << std::endl;
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(c, "DEL %s", key));
    std::cout << reply->integer << std::endl;
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(c, "EXIST %s", key));
    std::cout << reply->integer << std::endl;
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(c, "GET %s", key));
    std::cout << (reply->type == REDIS_REPLY_NIL ? "nil" : reply->str) << std::endl;
    freeReplyObject(reply);

    redisFree(c);
}
