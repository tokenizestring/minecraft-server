# minecraft-server

A from-scratch, heavily optimised Minecraft server written in C++.

No vanilla code, no third-party libraries: networking, compression, the Java and Bedrock protocols and the world are all written here. The goal is a server that can hold thousands of players on a tiny machine.

## Supported clients

All of these join the same server natively, with no proxy in between, and can see each other:

- Java Edition 26.2
- Java Edition 26.3
- Java Edition 1.8
- Bedrock Edition 1.26.5x (own RakNet implementation)

## Highlights

- Windows IOCP networking: around 1000 bots use roughly 6% of one core and about 7 MB of memory
- Own zlib inflate
- Chat sidebar with fading lines, colours and `/chat on|off`
- Large spawn world streamed to Java 26.x clients with configurable view distance and a shared chunk cache
- Plain text `server.txt` configuration

## Building

Requires Visual Studio (C++20 toolset). From the project folder run:

```
build.bat
```

The binary and its data are written to `build/bin/`.

## Configuration

On first start the server writes `server.txt` with defaults. Keys:

| key | meaning |
| --- | --- |
| `java-port` | Java Edition TCP port |
| `bedrock-port` | Bedrock Edition UDP port |
| `max-players` | player limit |
| `motd` | server list message |
| `view-distance` | chunk radius sent to players |
| `chunk-cache-mb` | size of the shared encoded chunk cache |
| `chat-lines` | lines shown in the chat sidebar |
| `chat-fade-seconds` | how long a chat line stays visible |

The spawn world (`data/spawn/world.bin`) is not part of the repository. `tools/convert/schem_to_world.py` converts a Sponge `.schem` file into it.

## Layout

```
core/      engine constants, IOCP networking, zlib, config
game/      shared world, terrain, chat and commands
java/      Java Edition protocol: login, registries, chunks, play, players
bedrock/   Bedrock Edition: RakNet, login, packets, chunks, play, players
utils/     logging
tools/     Python probes, load tests, capture and decode helpers, converters
data/      registry and tag data for Java 26.x
```

## Roadmap

See [ROADMAP.md](ROADMAP.md).
