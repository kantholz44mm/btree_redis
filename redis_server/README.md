# redis-server

Implementiert Teile des [RESP-Protokolls](https://redis.io/docs/latest/develop/reference/protocol-spec).

## Resources

* Redis-Protokol (RESP): https://redis.io/docs/latest/develop/reference/protocol-spec
* Redis-Command reference: https://redis.io/docs/latest/commands/

## Running

Die CMake-Executable `redis_server` startet den RESP-Server auf Port 3000.

## Development Tools

Zur Entwicklung wird eine [docker-compose](docker-compose.yml)-Datei bereitgestellt.
Diese stellt bereit:
* Ein Redis-Container, von welchem die Redis-CLI verwendet werden kann (siehe [Redis-Cli](#redis-cli))
* Ein [Redis-Insight](https://redis.io/insight/) Container, als GUI zum verbinden mit Redis (siehe [Redis-Insight](#redis-insight))


### Redis-CLI

Den `redis` Docker-Container verwenden, um sich mit der Redis CLI mit dem redis-server zu verbinden:

```shell
docker exec -it redis_server-redis-1 redis-cli -h host.docker.internal -p 3000
```

### Redis-Insight

Der Redis-Insight container ist unter [localhost:5540](http://localhost:5540) erreichbar.
Dieser ist vorkonfiguriert für die Verbindung mit dem offiziellen Redis-Container
und dem redis-server auf Port 3000.
Die Verbindung zu dem Redis-Container ist für Vergleichszwecke möglich.
Im aktuellen Zustand ist der Server (noch) nicht mit Redis-Insight kompatibel.
