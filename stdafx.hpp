
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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
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
#include "core/deflate/deflate.hpp"
#include "core/network/network.hpp"

#include "game/world/world.hpp"
#include "game/terrain/terrain.hpp"
#include "game/blocks/blocks.hpp"
#include "game/light/light.hpp"
#include "game/caverns/caverns.hpp"
#include "game/features/features.hpp"
#include "game/flora/flora.hpp"
#include "game/generator/generator.hpp"
#include "game/edits/edits.hpp"
#include "game/lookup/lookup.hpp"
#include "game/items/items.hpp"
#include "game/inventory/inventory.hpp"
#include "game/profile/profile.hpp"
#include "game/market/market.hpp"
#include "game/chat/chat.hpp"
#include "game/npc/npc.hpp"
#include "game/command/command.hpp"

#include "java/registry/java_registry.hpp"
#include "java/protocol/java_protocol.hpp"
#include "java/builder/java_builder.hpp"
#include "java/chunk/java_chunk.hpp"
#include "java/play/java_play.hpp"
#include "java/player/java_player.hpp"
#include "java/chat/java_chat.hpp"
#include "java/command/java_command.hpp"
#include "java/block/java_block.hpp"
#include "java/menu/java_menu.hpp"
#include "java/inventory/java_inventory.hpp"
#include "java/market/java_market.hpp"

#include "bedrock/raknet/raknet.hpp"
#include "bedrock/packet/bedrock_packet.hpp"
#include "bedrock/login/bedrock_login.hpp"
#include "bedrock/chunk/bedrock_chunk.hpp"
#include "bedrock/play/bedrock_play.hpp"
#include "bedrock/player/bedrock_player.hpp"
#include "bedrock/chat/bedrock_chat.hpp"
#include "bedrock/menu/bedrock_menu.hpp"
#include "bedrock/market/bedrock_market.hpp"
#include "bedrock/debug/bedrock_debug.hpp"

//=====================================================================================
