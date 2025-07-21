
# My C++ Redis Server

## Overview

`my_redis_server` is a lightweight Redis-compatible in-memory data store implemented in C++. It provides a robust and efficient solution for handling common data structures and commands over a plain TCP socket, adhering to the Redis Serialization Protocol (RESP).

## Features

This project supports a comprehensive set of Redis features, including:

* **Common Commands**: `PING`, `ECHO`, `FLUSHALL`
* **Key/Value Operations**: `SET`, `GET`, `KEYS`, `TYPE`, `DEL`/`UNLINK`, `EXPIRE`, `RENAME`
* **List Operations**: `LGET`, `LLEN`, `LPUSH`/`RPUSH` (multi-element), `LPOP`/`RPOP`, `LREM`, `LINDEX`, `LSET`
* **Hash Operations**: `HSET`, `HGET`, `HEXISTS`, `HDEL`, `HKEYS`, `HVALS`, `HLEN`, `HGETALL`, `HMSET`

Data is automatically persisted to `dump.my_rdb` every 300 seconds and upon graceful shutdown. The server attempts to load data from this file at startup, ensuring data durability.

## Project Structure

```

.
├── build/                  \# Compiled object files and executables
├── dump.my\_rdb             \# Persistent data dump file
├── include/                \# Public header files for classes
│   ├── RedisCommandHandler.h
│   ├── RedisDatabase.h
│   └── RedisServer.h
├── Makefile                \# Build rules for the project
├── my\_redis\_server         \# Compiled server executable
├── README.md               \# This documentation
├── src/                    \# Source code implementation files
│   ├── main.cpp
│   ├── RedisCommandHandler.cpp
│   ├── RedisDatabase.cpp
│   └── RedisServer.cpp
└── usecases.md             \# Detailed command use cases and design concepts

````

## Installation

This project uses a `Makefile` for easy compilation. Ensure you have a C++17 (or later) compliant compiler installed on your system.

To build the project:

```bash
make
````

To clean compiled files:

```bash
make clean
```

Alternatively, you can compile manually:

```bash
g++ -std=c++17 -pthread -Iinclude src/*.cpp -o my_redis_server
```

## Usage

### Running the Server

After compilation, you can start the server. It listens on the default port `6379` or a specified port:

```bash
./my_redis_server            # Listens on 6379 (default)
./my_redis_server 6380       # Listens on 6380
```

Upon startup, the server will attempt to load the `dump.my_rdb` file if present:

```
Database Loaded From dump.my_rdb
# or
No dump found or load failed; starting with an empty database.
```

A background thread automatically persists the database every 5 minutes. To trigger an immediate persistence and gracefully shut down the server, press `Ctrl+C`.

### Using the Server

You can connect to `my_redis_server` using the standard `redis-cli` or any custom RESP client.

**Example with `redis-cli`:**

```bash
redis-cli -p 6379
```

**Example Session:**

```
127.0.0.1:6379> PING
PONG

127.0.0.1:6379> SET mykey "Hello Redis"
OK

127.0.0.1:6379> GET mykey
"Hello Redis"
```

## Supported Commands

### Common Commands

  * **`PING`**: `PING` $\\rightarrow$ `PONG`
  * **`ECHO`**: `ECHO <msg>` $\\rightarrow$ `<msg>`
  * **`FLUSHALL`**: `FLUSHALL` $\\rightarrow$ Clear all data

### Key/Value Operations

  * **`SET`**: `SET <key> <value>` $\\rightarrow$ Store a string value
  * **`GET`**: `GET <key>` $\\rightarrow$ Retrieve a string value or `nil`
  * **`KEYS`**: `KEYS *` $\\rightarrow$ List all keys
  * **`TYPE`**: `TYPE <key>` $\\rightarrow$ Returns `string`, `list`, `hash`, or `none`
  * **`DEL`/`UNLINK`**: `DEL <key>` $\\rightarrow$ Delete a key
  * **`EXPIRE`**: `EXPIRE <key> <seconds>` $\\rightarrow$ Set a Time-To-Live (TTL) for a key
  * **`RENAME`**: `RENAME <old_key> <new_key>` $\\rightarrow$ Rename a key

### List Operations

  * **`LGET`**: `LGET <key>` $\\rightarrow$ Returns all elements of a list
  * **`LLEN`**: `LLEN <key>` $\\rightarrow$ Returns the length of a list
  * **`LPUSH`/`RPUSH`**: `LPUSH <key> <v1> [v2 ...]`, `RPUSH <key> <v1> [v2 ...]` $\\rightarrow$ Push one or more elements to the left/right of a list
  * **`LPOP`/`RPOP`**: `LPOP <key>`, `RPOP <key>` $\\rightarrow$ Pop an element from the left/right of a list
  * **`LREM`**: `LREM <key> <count> <value>` $\\rightarrow$ Remove occurrences of a value from a list
  * **`LINDEX`**: `LINDEX <key> <index>` $\\rightarrow$ Get an element by index from a list
  * **`LSET`**: `LSET <key> <index> <value>` $\\rightarrow$ Set the value of an element in a list by its index

### Hash Operations

  * **`HSET`**: `HSET <key> <field> <value>` $\\rightarrow$ Set the string value of a hash field
  * **`HGET`**: `HGET <key> <field>` $\\rightarrow$ Get the string value of a hash field
  * **`HEXISTS`**: `HEXISTS <key> <field>` $\\rightarrow$ Determine if a hash field exists
  * **`HDEL`**: `HDEL <key> <field>` $\\rightarrow$ Delete one or more hash fields
  * **`HLEN`**: `HLEN <key>` $\\rightarrow$ Get the number of fields in a hash
  * **`HKEYS`**: `HKEYS <key>` $\\rightarrow$ Get all the fields in a hash
  * **`HVALS`**: `HVALS <key>` $\\rightarrow$ Get all the values in a hash
  * **`HGETALL`**: `HGETALL <key>` $\\rightarrow$ Get all the fields and values in a hash
  * **`HMSET`**: `HMSET <key> <f1> <v1> [f2 v2 ...]` $\\rightarrow$ Set multiple hash fields to multiple values

## Design & Architecture

The server's design incorporates several key architectural principles:

  * **Concurrency**: Each client connection is handled in its own `std::thread` to enable parallel processing of requests.
  * **Synchronization**: A single `std::mutex`, `db_mutex`, is employed to guard all in-memory data stores, ensuring thread-safe access to the database.
  * **Data Stores**:
      * `kv_store` (`unordered_map<string,string>`) for string key-value pairs.
      * `list_store` (`unordered_map<string,vector<string>>`) for list data.
      * `hash_store` (`unordered_map<string,unordered_map<string,string>>`) for hash data.
  * **Expiration**: Lazy eviction is implemented via `purgeExpired()` on each access, complemented by a `TTL` map (`expiry_map`) for managing key expirations.
  * **Persistence**: A simplified text-based RDB format is used for dumping and loading data from `dump.my_rdb`.
  * **Singleton Pattern**: The `RedisDatabase::getInstance()` method ensures that only one shared instance of the database exists, promoting centralized data management.
  * **RESP Parsing**: A custom parser within `RedisCommandHandler` efficiently handles both inline and array formats of the RESP protocol.

## Concepts & Use Cases

For a detailed understanding of the underlying concepts (TCP sockets, RESP, data structures, etc.) and real-world usage scenarios for each command, please refer to the `usecases.md` file in the project root.

## Testing

You can verify the server's functionality interactively using `redis-cli` or through automated scripts. Refer to the test examples provided in `usecases.md` for specific command tests, or utilize the `test_all.sh` script (if available) for end-to-end validation.

## Credits

This project is inspired by and built upon the foundational work of the following authors:

  * Selcuk Ata Aksoy
  * Qwasar SV -- Software Engineering School

