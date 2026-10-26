# Roadmap

## Done

- [x] IOCP networking (~1000 bots at ~6% of one core, ~7 MB)
- [x] Own zlib inflate
- [x] Java 26.2 and 26.3 join (handshake, status, login, configuration, registries, play)
- [x] Java 1.8 join
- [x] Bedrock 1.26.5x join over own RakNet
- [x] Cross-version player visibility (Java 26.x, Java 1.8 and Bedrock see each other)
- [x] Chat sidebar with fade, colours and `/chat on|off`
- [x] `server.txt` configuration
- [x] Large spawn streamed to Java 26.x with view distance and a shared chunk cache

## Planned

- [ ] Sky lighting
- [ ] Block place/break stored as per-position changes on top of the base world
- [ ] Bedrock and 1.8 block translation for the spawn
- [ ] Network compression for Java and Bedrock
- [ ] Entities and inventory
- [ ] Bedrock command list
- [ ] Linux port on epoll
- [ ] Low-memory world: target ~5 MB RAM and thousands of players on a $2 VPS
