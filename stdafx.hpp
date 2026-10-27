
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
#include <numbers>

#include "utils/log/log.hpp"

#include "core/engine/engine.hpp"
#include "core/config/config.hpp"
#include "core/zlib/zlib.hpp"
#include "core/deflate/deflate.hpp"
#include "core/network/network.hpp"

#include "game/world/world/world.hpp"
#include "game/world/terrain/terrain.hpp"
#include "game/world/blocks/blocks/blocks.hpp"
#include "game/world/blocks/light/light.hpp"
#include "game/world/generation/caverns/caverns.hpp"
#include "game/world/generation/features/features.hpp"
#include "game/world/generation/flora/flora.hpp"
#include "game/world/generation/generator/generator.hpp"
#include "game/world/storage/edits/edits.hpp"
#include "game/world/storage/lookup/lookup.hpp"
#include "game/world/storage/surface/surface.hpp"
#include "game/world/entities/drops/drops.hpp"
#include "game/world/entities/mobs/mobs.hpp"
#include "game/world/farming/crops/crops.hpp"
#include "game/world/farming/growth/growth.hpp"

#include "game/player/items/items.hpp"
#include "game/player/inventory/inventory.hpp"
#include "game/player/profile/profile.hpp"
#include "game/player/vitals/vitals.hpp"

#include "game/economy/market/market.hpp"

#include "game/social/chat/chat.hpp"
#include "game/social/friends/friends.hpp"
#include "game/social/hub/hub.hpp"
#include "game/social/command/command.hpp"

#include "java/network/registry/java_registry.hpp"
#include "java/network/protocol/java_protocol.hpp"
#include "java/network/play/java_play.hpp"
#include "java/network/chat/java_chat.hpp"
#include "java/network/command/java_command.hpp"

#include "java/world/builder/java_builder.hpp"
#include "java/world/chunk/java_chunk.hpp"
#include "java/world/block/java_block.hpp"
#include "java/world/glow/java_glow.hpp"

#include "java/entities/player/java_player.hpp"
#include "java/entities/drops/java_drops.hpp"
#include "java/entities/mobs/java_mobs.hpp"

#include "java/interface/menu/java_menu.hpp"
#include "java/interface/inventory/java_inventory.hpp"
#include "java/interface/market/java_market.hpp"
#include "java/interface/social/java_social.hpp"

#include "bedrock/network/raknet/raknet.hpp"
#include "bedrock/network/packet/bedrock_packet.hpp"
#include "bedrock/network/login/bedrock_login.hpp"
#include "bedrock/network/play/bedrock_play.hpp"
#include "bedrock/network/debug/bedrock_debug.hpp"

#include "bedrock/world/chunk/bedrock_chunk.hpp"

#include "bedrock/entities/player/bedrock_player.hpp"

#include "bedrock/interface/chat/bedrock_chat.hpp"
#include "bedrock/interface/menu/bedrock_menu.hpp"
#include "bedrock/interface/market/bedrock_market.hpp"

//=====================================================================================
