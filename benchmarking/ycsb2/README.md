Copied from https://github.com/m-mueller678/btree-cpp/blob/master/ycsb2.cpp and adjusted to use the RESP based redis client.

Example environment variables for testing

```shell
REDIS_HOST=127.0.0.1;REDIS_PORT=3000;DATA=rng4;DENSITY=0.5;KEY_COUNT=1000;OP_COUNT=1e5;PAYLOAD_SIZE=10;RUN_ID=123e4567-e89b-12d3-a456-426614174000;SCAN_LENGTH=100;YCSB_VARIANT=3;ZIPF=-1;
```