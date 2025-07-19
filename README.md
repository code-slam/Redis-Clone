# MyRedisProject

A minimal Redis clone in C++, featuring:

- **TCP/IPv4** socket setup (`socket`, `bind`, `listen`, `accept`)
- **Background persistence** thread (dump every 300 s)
- **Graceful shutdown** support

## Project Structure
MyRedisProject/
├── include/
│ └── RedisServer.h
├── src/
│ ├── main.cpp
│ └── RedisServer.cpp
└── README.md
