#include <iostream>

#include "RedisClient.h"

int main() {
    auto client = RedisClient::connect("127.0.0.1", 3000);

    const auto key = "example";

    const auto setReply = client.run("SET %s %d", key, 5);
    std::cout << setReply.getStatus() << std::endl;

    auto getReply = client.run("GET %s", key);
    std::cout << getReply.getString() << std::endl;

    std::cout << client.run("INCR %s", key).getInt() << std::endl;
    std::cout << client.run("DECRBY %s %d", key, 3).getInt() << std::endl;
    std::cout << client.run("DEL %s", key).getInt() << std::endl;
    std::cout << client.run("EXISTS %s", key).getInt() << std::endl;

    const auto finalGetReply = client.run("GET %s", key);
    std::cout << (finalGetReply.is(RedisType::NIL) ? "nil" : finalGetReply.getStatus()) << std::endl;

    client.run({"MSET", "key1", "val1", "key2", "val2", "key3", "val3" }).orThrow();
    const auto mgetVals = client.run({"MGET", "key1", "key2", "key3"});
    const auto vals = mgetVals.getArray();
    std::cout << "MGET:" << std::endl;
    for (const auto& val : vals) {
        std::cout << val.getString() << std::endl;
    }
}
