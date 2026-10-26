
//=====================================================================================

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>

#include <cstdint>
#include <cstddef>
#include <bit>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <csignal>
#include <ctime>
#include <chrono>
#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <ranges>
#include <span>
#include <charconv>

#include "utils/log/log.hpp"

#include "core/engine/engine.hpp"
#include "core/config/config.hpp"
#include "core/zlib/zlib.hpp"
#include "core/network/network.hpp"

#include "game/world/world.hpp"
#include "game/terrain/terrain.hpp"
#include "game/light/light.hpp"
#include "game/chat/chat.hpp"
#include "game/command/command.hpp"

#include "java/registry/java_registry.hpp"
#include "java/protocol/java_protocol.hpp"
#include "java/chunk/java_chunk.hpp"
#include "java/play/java_play.hpp"
#include "java/player/java_player.hpp"
#include "java/chat/java_chat.hpp"
#include "java/command/java_command.hpp"

#include "bedrock/raknet/raknet.hpp"
#include "bedrock/packet/bedrock_packet.hpp"
#include "bedrock/login/bedrock_login.hpp"
#include "bedrock/chunk/bedrock_chunk.hpp"
#include "bedrock/play/bedrock_play.hpp"
#include "bedrock/player/bedrock_player.hpp"
#include "bedrock/chat/bedrock_chat.hpp"
#include "bedrock/debug/bedrock_debug.hpp"

//=====================================================================================
