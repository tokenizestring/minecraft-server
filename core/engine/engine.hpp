
//=====================================================================================

#pragma once

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	constexpr auto server_address = "127.0.0.1";
	constexpr auto server_versions = "1.8.x / 26.2 / 26.3";
	constexpr auto server_brand = "mc++";
	constexpr auto log_file_name = "server.log";

	constexpr auto color_server = 0xE6E6E6u;
	constexpr auto color_net = 0x82B4F0u;
	constexpr auto color_status = 0x9A9A9Au;
	constexpr auto color_login = 0xF0DC78u;
	constexpr auto color_config = 0xC8A0F0u;
	constexpr auto color_play = 0xA0F0A0u;
	constexpr auto color_chat = 0xFFFFFFu;
	constexpr auto color_warn = 0xF0A050u;
	constexpr auto color_error = 0xF06464u;

	constexpr auto receive_size = 65536u;
	constexpr auto max_frame_size = 2097152u;
	constexpr auto max_backlog = 16777216u;
	constexpr auto max_name_length = 16u;
	constexpr auto max_chat_length = 512u;
	constexpr auto max_chat_lines = 15u;
	constexpr auto max_motd_length = 128u;
	constexpr auto config_file_name = "server.txt";
	constexpr auto chat_wrap = 28u;
	constexpr auto chat_objective = "chat";
	constexpr auto chat_title = "Chat";
	constexpr auto chat_command = "chat";
	constexpr const char* chat_arguments[] = { "on", "off", "1", "0" };
	constexpr std::uint8_t command_node_root = 0x00u;
	constexpr std::uint8_t command_node_literal = 0x01u;
	constexpr std::uint8_t command_node_executable = 0x04u;
	constexpr char chat_colors[] = { 'a', 'b', 'c', 'd', 'e', '6', '9', '3', '2', '5' };
	constexpr auto max_address_length = 255u;
	constexpr auto max_varint_size = 5u;
	constexpr auto max_frame_header_size = 3u;
	constexpr auto rx_shrink_size = 65536u;
	constexpr auto max_owned_segment = 65536u;

	constexpr auto accept_backlog = 64u;
	constexpr auto accept_address_space = 2u * (sizeof(sockaddr_in) + 16u);
	constexpr auto max_send_buffers = 64u;
	constexpr auto completion_batch = 256u;

	constexpr auto tick_ms = 50ui64;
	constexpr auto keep_alive_interval_ms = 10000ui64;
	constexpr auto keep_alive_timeout_ms = 30000ui64;
	constexpr auto login_timeout_ms = 15000ui64;
	constexpr auto linger_timeout_ms = 3000ui64;

	constexpr std::int32_t bedrock_protocol = 2193;
	constexpr auto bedrock_version = "1.26.51";
	constexpr auto bedrock_receives = 16u;
	constexpr auto max_datagram_size = 2048u;
	constexpr auto max_sessions = 1024u;
	constexpr auto session_timeout_ms = 10000ui64;
	constexpr auto resend_ms = 300ui64;
	constexpr auto max_batch_size = 16777216u;
	constexpr auto max_split_count = 4096u;

	constexpr std::uint8_t raknet_magic[] = { 0x00u, 0xFFu, 0xFFu, 0x00u, 0xFEu, 0xFEu, 0xFEu, 0xFEu, 0xFDu, 0xFDu, 0xFDu, 0xFDu, 0x12u, 0x34u, 0x56u, 0x78u };
	constexpr std::uint8_t raknet_protocol = 11u;
	constexpr auto raknet_min_mtu = 576u;
	constexpr auto raknet_max_mtu = 1492u;
	constexpr auto raknet_udp_overhead = 28u;
	constexpr auto raknet_frame_overhead = 24u;
	constexpr auto raknet_system_addresses = 20u;

	constexpr std::uint8_t raknet_connected_ping = 0x00u;
	constexpr std::uint8_t raknet_unconnected_ping = 0x01u;
	constexpr std::uint8_t raknet_unconnected_ping_open = 0x02u;
	constexpr std::uint8_t raknet_connected_pong = 0x03u;
	constexpr std::uint8_t raknet_open_request_1 = 0x05u;
	constexpr std::uint8_t raknet_open_reply_1 = 0x06u;
	constexpr std::uint8_t raknet_open_request_2 = 0x07u;
	constexpr std::uint8_t raknet_open_reply_2 = 0x08u;
	constexpr std::uint8_t raknet_connection_request = 0x09u;
	constexpr std::uint8_t raknet_connection_accepted = 0x10u;
	constexpr std::uint8_t raknet_new_incoming = 0x13u;
	constexpr std::uint8_t raknet_disconnect = 0x15u;
	constexpr std::uint8_t raknet_incompatible = 0x19u;
	constexpr std::uint8_t raknet_unconnected_pong = 0x1Cu;
	constexpr std::uint8_t raknet_game_packet = 0xFEu;

	constexpr std::uint8_t raknet_flag_valid = 0x80u;
	constexpr std::uint8_t raknet_flag_ack = 0x40u;
	constexpr std::uint8_t raknet_flag_nack = 0x20u;
	constexpr std::uint8_t raknet_flag_split = 0x10u;
	constexpr std::uint8_t raknet_datagram_flags = 0x84u;
	constexpr std::uint8_t raknet_reliability_unreliable = 0u;
	constexpr std::uint8_t raknet_reliability_reliable_ordered = 3u;
	constexpr auto raknet_datagram_header = 4u;
	constexpr auto raknet_max_ack_range = 4096u;
	constexpr auto raknet_max_order_queue = 4096u;
	constexpr std::uint8_t raknet_ack_single = 1u;
	constexpr std::uint8_t raknet_ack_range = 0u;

	constexpr std::uint8_t bedrock_compression_zlib = 0x00u;
	constexpr std::uint8_t bedrock_compression_none = 0xFFu;
	constexpr std::uint16_t bedrock_compression_threshold = 512u;

	constexpr std::uint32_t bedrock_login_request = 1u;
	constexpr std::uint32_t bedrock_play_status = 2u;
	constexpr std::uint32_t bedrock_disconnect = 5u;
	constexpr std::uint32_t bedrock_network_settings = 143u;
	constexpr std::uint32_t bedrock_request_network_settings = 193u;
	constexpr std::uint32_t bedrock_resource_packs_info = 6u;
	constexpr std::uint32_t bedrock_resource_pack_stack = 7u;
	constexpr std::uint32_t bedrock_resource_pack_response = 8u;
	constexpr std::uint32_t bedrock_start_game = 11u;
	constexpr std::uint32_t bedrock_move_player = 19u;
	constexpr std::uint32_t bedrock_text = 9u;
	constexpr std::uint32_t bedrock_text_author_message = 1u;
	constexpr std::uint8_t bedrock_text_chat = 1u;
	constexpr std::uint32_t bedrock_text_message_only = 0u;
	constexpr std::uint8_t bedrock_text_tip = 5u;
	constexpr std::uint32_t bedrock_command_request = 77u;
	constexpr std::uint32_t bedrock_remove_objective = 106u;
	constexpr std::uint32_t bedrock_set_display_objective = 107u;
	constexpr std::uint32_t bedrock_set_score = 108u;
	constexpr std::uint32_t bedrock_score_fake_player = 3u;
	constexpr auto bedrock_score_fake_player_name = "changefakeplayer";
	constexpr auto bedrock_display_sidebar = "sidebar";
	constexpr auto bedrock_criteria_dummy = "dummy";
	constexpr std::int32_t bedrock_sort_descending = 1;
	constexpr std::uint32_t bedrock_respawn = 45u;
	constexpr std::uint32_t bedrock_add_player = 12u;
	constexpr std::uint32_t bedrock_remove_actor = 14u;
	constexpr std::uint32_t bedrock_set_actor_data = 39u;
	constexpr std::uint32_t bedrock_player_list = 63u;
	constexpr std::uint32_t bedrock_list_remove = 0u;
	constexpr std::uint32_t bedrock_list_add = 1u;
	constexpr std::uint8_t bedrock_list_action_add = 0u;
	constexpr std::uint8_t bedrock_list_action_remove = 1u;
	constexpr std::uint8_t bedrock_move_normal = 0u;
	constexpr std::uint8_t bedrock_move_teleport = 2u;
	constexpr std::uint32_t bedrock_teleport_cause_unknown = 0u;
	constexpr std::int32_t bedrock_gametype_default = 5;
	constexpr std::uint8_t bedrock_permission_member_byte = 1u;
	constexpr std::uint16_t bedrock_actor_name = 4u;
	constexpr std::uint8_t bedrock_data_byte = 0u;
	constexpr std::uint8_t bedrock_data_short = 1u;
	constexpr std::uint8_t bedrock_data_int = 2u;
	constexpr std::uint8_t bedrock_data_float = 3u;
	constexpr std::uint8_t bedrock_data_string = 4u;
	constexpr std::uint8_t bedrock_data_compound = 5u;
	constexpr std::uint8_t bedrock_data_position = 6u;
	constexpr std::uint8_t bedrock_data_long = 7u;
	constexpr std::uint8_t bedrock_data_vector = 8u;
	constexpr std::uint8_t bedrock_empty_compound[] = { 0x0Au, 0x00u, 0x00u };
	constexpr std::uint8_t bedrock_empty_item[] = { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u };
	constexpr std::uint16_t bedrock_ability_layers[] = { 5u, 1u };
	constexpr std::uint32_t bedrock_ability_sets[] = { 319u, 1048575u };
	constexpr std::uint32_t bedrock_ability_values[] = { 256u, 63u };
	constexpr std::float_t bedrock_ability_speeds[] = { 0.0f, 0.0f, 0.0f, 0.05f, 1.0f, 0.1f };
	constexpr auto bedrock_skin_size = 64u;
	constexpr auto bedrock_skin_id = "Standard_Custom";
	constexpr auto bedrock_skin_patch = "{\"geometry\":{\"default\":\"geometry.humanoid.custom\"}}";
	constexpr auto bedrock_skin_geometry = "null\n";
	constexpr auto bedrock_skin_engine = "0.0.0";
	constexpr auto bedrock_skin_trusted = "true";
	constexpr std::uint8_t bedrock_arm_wide = 1u;
	constexpr std::int32_t bedrock_platform_java = 7;
	constexpr std::uint32_t bedrock_skin_colors[] = { 0xFF6A8CC0u, 0xFFC09A3Bu, 0xFF8B3A2Au };
	constexpr std::uint8_t bedrock_respawn_ready = 1u;
	constexpr std::uint32_t bedrock_level_chunk = 58u;
	constexpr std::uint32_t bedrock_request_chunk_radius = 69u;
	constexpr std::uint32_t bedrock_chunk_radius_updated = 70u;
	constexpr std::uint32_t bedrock_set_local_player_initialized = 113u;
	constexpr std::uint32_t bedrock_chunk_publisher_update = 121u;
	constexpr std::uint32_t bedrock_biome_definition_list = 122u;
	constexpr std::uint32_t bedrock_player_auth_input = 144u;
	constexpr std::uint32_t bedrock_item_registry = 162u;
	constexpr std::uint32_t bedrock_packet_violation = 156u;
	constexpr std::uint32_t bedrock_subchunk = 174u;
	constexpr std::uint32_t bedrock_subchunk_request = 175u;
	constexpr std::uint8_t bedrock_subchunk_success = 1u;
	constexpr std::uint8_t bedrock_subchunk_missing = 2u;
	constexpr std::uint8_t bedrock_subchunk_all_air = 6u;
	constexpr auto bedrock_max_subchunk_requests = 4096u;
	constexpr std::int32_t bedrock_request_no_limit = -1;
	constexpr std::uint8_t bedrock_heightmaps_none[] = { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u };
	constexpr std::uint8_t bedrock_heightmaps_air[] = { 0x03u, 0x00u, 0x03u, 0x00u, 0x00u };
	constexpr std::uint32_t bedrock_jigsaw_structure_data = 313u;
	constexpr std::uint32_t bedrock_voxel_shapes = 337u;
	constexpr std::uint8_t bedrock_empty_jigsaws[] = { 0x0Au, 0x00u, 0x09u, 0x07u, 'j', 'i', 'g', 's', 'a', 'w', 's', 0x0Au, 0x00u, 0x09u, 0x0Au, 'p', 'r', 'o', 'c', 'e', 's', 's', 'o', 'r', 's', 0x0Au, 0x00u, 0x09u, 0x0Eu, 's', 't', 'r', 'u', 'c', 't', 'u', 'r', 'e', '_', 's', 'e', 't', 's', 0x0Au, 0x00u, 0x09u, 0x0Eu, 't', 'e', 'm', 'p', 'l', 'a', 't', 'e', '_', 'p', 'o', 'o', 'l', 's', 0x0Au, 0x00u, 0x00u };

	constexpr std::int32_t bedrock_status_login_success = 0;
	constexpr std::int32_t bedrock_status_failed_client = 1;
	constexpr std::int32_t bedrock_status_failed_server = 2;
	constexpr std::int32_t bedrock_status_player_spawn = 3;
	constexpr std::int32_t bedrock_disconnect_reason_unknown = 0;
	constexpr std::uint32_t bedrock_pack_have_all = 2u;
	constexpr std::uint32_t bedrock_pack_completed = 3u;
	constexpr auto bedrock_template_version = "0.0.0";
	constexpr std::uint8_t bedrock_start_game_tail[] = { 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u };

	constexpr std::int32_t bedrock_block_air = -604749536;
	constexpr std::int32_t bedrock_block_grass = -567203660;
	constexpr std::int32_t bedrock_biome_plains = 1;
	constexpr auto bedrock_subchunk_version = 9u;
	constexpr auto bedrock_min_section = -4;
	constexpr auto bedrock_section_count = 24u;
	constexpr auto bedrock_storage_bits = 1u;
	constexpr auto bedrock_view_radius = 4;
	constexpr auto bedrock_eye_height = 1.62f;
	constexpr std::int32_t bedrock_generator_flat = 2;
	constexpr std::int32_t bedrock_difficulty_easy = 1;
	constexpr std::int32_t bedrock_permission_member = 1;
	constexpr auto bedrock_chunk_tick_range = 4;
	constexpr auto bedrock_any_version = "*";
	constexpr auto bedrock_level_name = "world";
	constexpr auto bedrock_zero_uuid = "00000000-0000-0000-0000-000000000000";
	constexpr auto bedrock_correlation_id = "<raknet>0000-0000-0000-0000";
	constexpr auto bedrock_server_version = "1.26.52";
	constexpr auto bedrock_debug_send_world = true;
	constexpr auto bedrock_debug_reference = true;
	constexpr auto bedrock_debug_reference_data = true;
	constexpr auto bedrock_debug_inject = true;
	constexpr auto bedrock_inject_interval_ms = 4000ui64;

	constexpr std::uint16_t deflate_length_base[] = { 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 13u, 15u, 17u, 19u, 23u, 27u, 31u, 35u, 43u, 51u, 59u, 67u, 83u, 99u, 115u, 131u, 163u, 195u, 227u, 258u };
	constexpr std::uint8_t deflate_length_extra[] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u, 2u, 2u, 2u, 2u, 3u, 3u, 3u, 3u, 4u, 4u, 4u, 4u, 5u, 5u, 5u, 5u, 0u };
	constexpr std::uint16_t deflate_distance_base[] = { 1u, 2u, 3u, 4u, 5u, 7u, 9u, 13u, 17u, 25u, 33u, 49u, 65u, 97u, 129u, 193u, 257u, 385u, 513u, 769u, 1025u, 1537u, 2049u, 3073u, 4097u, 6145u, 8193u, 12289u, 16385u, 24577u };
	constexpr std::uint8_t deflate_distance_extra[] = { 0u, 0u, 0u, 0u, 1u, 1u, 2u, 2u, 3u, 3u, 4u, 4u, 5u, 5u, 6u, 6u, 7u, 7u, 8u, 8u, 9u, 9u, 10u, 10u, 11u, 11u, 12u, 12u, 13u, 13u };
	constexpr std::uint8_t deflate_code_order[] = { 16u, 17u, 18u, 0u, 8u, 7u, 9u, 6u, 10u, 5u, 11u, 4u, 12u, 3u, 13u, 2u, 14u, 1u, 15u };
	constexpr auto deflate_max_bits = 15u;

	constexpr std::uint8_t legacy_ping_byte = 0xFEu;
	constexpr std::uint8_t legacy_kick_byte = 0xFFu;
	constexpr auto legacy_ping_protocol = 127u;

	constexpr std::int32_t protocol_1_7_6 = 5;
	constexpr std::int32_t protocol_1_8 = 47;
	constexpr std::int32_t protocol_1_16 = 735;
	constexpr std::int32_t protocol_1_19 = 759;
	constexpr std::int32_t protocol_1_20_2 = 764;
	constexpr std::int32_t protocol_1_20_5 = 766;
	constexpr std::int32_t protocol_1_21_2 = 768;
	constexpr std::int32_t protocol_26_2 = 776;
	constexpr std::int32_t protocol_26_3 = 777;

	constexpr std::int32_t known_protocols[] = { 4, 5, 47, 107, 108, 109, 110, 210, 315, 316, 335, 338, 340, 393, 401, 404, 477, 480, 485, 490, 498, 573, 575, 578, 735, 736, 751, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777 };
	constexpr const char* known_names[] = { "1.7.2", "1.7.10", "1.8.x", "1.9", "1.9.1", "1.9.2", "1.9.4", "1.10.x", "1.11", "1.11.2", "1.12", "1.12.1", "1.12.2", "1.13", "1.13.1", "1.13.2", "1.14", "1.14.1", "1.14.2", "1.14.3", "1.14.4", "1.15", "1.15.1", "1.15.2", "1.16", "1.16.1", "1.16.2", "1.16.3", "1.16.5", "1.17", "1.17.1", "1.18.1", "1.18.2", "1.19", "1.19.2", "1.19.3", "1.19.4", "1.20.1", "1.20.2", "1.20.4", "1.20.6", "1.21.1", "1.21.3", "1.21.4", "1.21.5", "1.21.6", "1.21.8", "1.21.10", "1.21.11", "26.1", "26.2", "26.3" };

	constexpr std::int32_t handshake_next_status = 1;
	constexpr std::int32_t handshake_next_login = 2;
	constexpr std::int32_t handshake_next_transfer = 3;

	constexpr std::int32_t handshake_in_intention = 0x00;
	constexpr std::int32_t status_in_request = 0x00;
	constexpr std::int32_t status_in_ping = 0x01;
	constexpr std::int32_t status_out_response = 0x00;
	constexpr std::int32_t status_out_pong = 0x01;
	constexpr std::int32_t login_in_start = 0x00;
	constexpr std::int32_t login_in_acknowledged = 0x03;
	constexpr std::int32_t login_out_disconnect = 0x00;
	constexpr std::int32_t login_out_success = 0x02;

	constexpr std::int32_t config_out_custom_payload = 0x01;
	constexpr std::int32_t config_out_finish = 0x03;
	constexpr std::int32_t config_out_registry_data = 0x07;

	constexpr std::int32_t v1_8_play_out_keep_alive = 0x00;
	constexpr std::int32_t v1_8_play_out_join_game = 0x01;
	constexpr std::int32_t v1_8_play_out_spawn_position = 0x05;
	constexpr std::int32_t v1_8_play_out_position = 0x08;
	constexpr std::int32_t v1_8_play_out_chunk = 0x21;
	constexpr std::int32_t v1_8_play_out_disconnect = 0x40;
	constexpr std::int32_t v1_8_play_in_keep_alive = 0x00;
	constexpr std::int32_t v1_8_play_in_position = 0x04;
	constexpr std::int32_t v1_8_play_in_position_look = 0x06;
	constexpr std::int32_t v1_8_play_in_look = 0x05;
	constexpr std::int32_t v1_8_play_in_chat = 0x01;
	constexpr std::int32_t v1_8_play_out_chat = 0x02;
	constexpr std::uint8_t v1_8_chat_action_bar = 2u;
	constexpr std::int32_t v1_8_play_out_objective = 0x3B;
	constexpr std::int32_t v1_8_play_out_score = 0x3C;
	constexpr std::int32_t v1_8_play_out_display_objective = 0x3D;
	constexpr auto v1_8_objective_type = "integer";
	constexpr auto v1_8_score_limit = 38u;
	constexpr std::uint8_t objective_create = 0u;
	constexpr std::uint8_t objective_remove = 1u;
	constexpr std::int32_t display_slot_sidebar = 1;
	constexpr std::int32_t number_format_blank = 0;
	constexpr std::int32_t v1_8_play_out_spawn_player = 0x0C;
	constexpr std::int32_t v1_8_play_out_destroy_entities = 0x13;
	constexpr std::int32_t v1_8_play_out_entity_teleport = 0x18;
	constexpr std::int32_t v1_8_play_out_head_look = 0x19;
	constexpr std::int32_t v1_8_play_out_player_list = 0x38;
	constexpr std::int32_t v1_8_player_list_add = 0;
	constexpr std::int32_t v1_8_player_list_remove = 4;
	constexpr auto v1_8_fixed_scale = 32.0;
	constexpr std::uint8_t v1_8_metadata_health = 0x66u;
	constexpr auto v1_8_health = 20.0f;
	constexpr std::uint8_t v1_8_metadata_end = 0x7Fu;
	constexpr std::uint8_t java_info_actions = 0xFFu;

	constexpr std::uint8_t modern_game_event_wait_for_chunks = 13u;
	constexpr auto modern_brand_channel = "minecraft:brand";
	constexpr auto modern_feature_vanilla = "minecraft:vanilla";
	constexpr auto modern_pack_namespace = "minecraft";
	constexpr auto modern_pack_id = "core";
	constexpr auto modern_overworld = "minecraft:overworld";
	constexpr auto modern_plains = "minecraft:plains";
	constexpr auto modern_biome_registry = "minecraft:worldgen/biome";
	constexpr auto modern_dimension_registry = "minecraft:dimension_type";
	constexpr auto data_registry_header = "registry ";
	constexpr auto data_tags_header = "tags ";
	constexpr auto missing_index = 0xFFFFFFFFu;

	constexpr auto spawn_block_x = 12;
	constexpr auto spawn_block_y = 59;
	constexpr auto spawn_block_z = 7;
	constexpr auto spawn_x = 12.5;
	constexpr auto spawn_y = 60.0;
	constexpr auto spawn_z = 7.5;
	constexpr auto spawn_platform_radius = 2;
	constexpr auto spawn_platform_blocks = (spawn_platform_radius * 2 + 1) * (spawn_platform_radius * 2 + 1);
	constexpr auto rescue_y = -32.0;
	constexpr auto terrain_file_name = "data/spawn/world.bin";
	constexpr std::uint32_t terrain_magic = 0x4257434Du;
	constexpr std::uint32_t terrain_version = 1u;
	constexpr std::uint8_t terrain_flag_air = 1u;
	constexpr std::int32_t modern_min_section = -4;
	constexpr auto view_radius = 2;
	constexpr std::uint8_t gamemode_survival = 0u;

	constexpr auto section_volume = 4096u;
	constexpr auto light_array_size = 2048u;
	constexpr auto biome_column_count = 256u;

	constexpr auto modern_min_y = -64;
	constexpr auto modern_section_count = 24u;
	constexpr auto modern_light_section_count = 26u;
	constexpr auto modern_palette_bits = 4u;
	constexpr auto modern_heightmap_bits = 9u;
	constexpr auto modern_heightmap_per_long = 64u / modern_heightmap_bits;
	constexpr auto modern_heightmap_longs = (256u + modern_heightmap_per_long - 1u) / modern_heightmap_per_long;
	constexpr auto modern_light_mask = (1ui64 << modern_light_section_count) - 1ui64;
	constexpr std::int32_t modern_heightmap_types[] = { 1, 4, 5 };
	constexpr auto modern_sea_level = 63;

	constexpr std::uint16_t v1_8_grass_block = 2u << 4u;
	constexpr std::uint8_t v1_8_plains = 1u;
	constexpr auto v1_8_level_type = "flat";

	constexpr auto get_section_index(std::int32_t x, std::int32_t y, std::int32_t z) { return static_cast<std::uint32_t>(((y & 15) << 8) | ((z & 15) << 4) | (x & 15)); }
	constexpr auto get_column_index(std::int32_t x, std::int32_t z) { return static_cast<std::uint32_t>(((z & 15) << 4) | (x & 15)); }
	constexpr auto is_reliable(std::uint8_t reliability) { return (reliability == 2u || reliability == 3u || reliability == 4u || reliability == 6u || reliability == 7u); }
	constexpr auto is_sequenced(std::uint8_t reliability) { return (reliability == 1u || reliability == 4u); }
	constexpr auto is_ordered(std::uint8_t reliability) { return (reliability == 1u || reliability == 3u || reliability == 4u || reliability == 7u); }
	constexpr auto get_bedrock_index(std::int32_t x, std::int32_t y, std::int32_t z) { return static_cast<std::uint32_t>(((x & 15) << 8) | ((z & 15) << 4) | (y & 15)); }
	constexpr auto is_platform_column(std::int32_t x, std::int32_t z) { return (x >= spawn_block_x - spawn_platform_radius && x <= spawn_block_x + spawn_platform_radius && z >= spawn_block_z - spawn_platform_radius && z <= spawn_block_z + spawn_platform_radius); }
	constexpr auto is_platform_index(std::uint32_t index) { return ((index >> 8u) == static_cast<std::uint32_t>(spawn_block_y & 15) && is_platform_column(static_cast<std::int32_t>(index & 15u), static_cast<std::int32_t>((index >> 4u) & 15u))); }
	constexpr auto is_platform_bedrock_index(std::uint32_t index) { return ((index & 15u) == static_cast<std::uint32_t>(spawn_block_y & 15) && is_platform_column(static_cast<std::int32_t>(index >> 8u), static_cast<std::int32_t>((index >> 4u) & 15u))); }

	namespace structures
	{
		enum state_e : std::uint32_t
		{
			state_handshake,
			state_status,
			state_login,
			state_configuration,
			state_play
		};
		/*
		//=====================================================================================
		*/
		enum family_e : std::uint32_t
		{
			family_none,
			family_1_8,
			family_26_2,
			family_26_3,
			family_count
		};
		/*
		//=====================================================================================
		*/
		struct layout_s
		{
			const char* pack_version;
			const char* data_path;

			std::int32_t config_out_disconnect;
			std::int32_t config_out_features;
			std::int32_t config_out_known_packs;
			std::int32_t config_out_tags;
			std::int32_t config_in_finish;

			std::int32_t play_out_disconnect;
			std::int32_t play_out_game_event;
			std::int32_t play_out_keep_alive;
			std::int32_t play_out_chunk;
			std::int32_t play_out_login;
			std::int32_t play_out_position;
			std::int32_t play_out_center_chunk;
			std::int32_t play_in_keep_alive;
			std::int32_t play_in_move_position;
			std::int32_t play_in_move_position_rotation;
			std::int32_t play_in_move_rotation;
			std::int32_t play_out_add_entity;
			std::int32_t play_out_player_info_update;
			std::int32_t play_out_player_info_remove;
			std::int32_t play_out_teleport_entity;
			std::int32_t play_out_rotate_head;
			std::int32_t play_out_remove_entities;
			std::int32_t play_in_chat;
			std::int32_t play_out_system_chat;
			std::int32_t play_out_set_objective;
			std::int32_t play_out_set_display_objective;
			std::int32_t play_out_set_score;
			std::int32_t play_out_reset_score;
			std::int32_t play_in_chat_command;
			std::int32_t play_out_commands;
			std::int32_t player_entity_type;

			std::uint32_t grass_block;
			bool byte_bitsets;
			bool optional_previous_gamemode;
			std::uint32_t direct_bits;
			std::int32_t play_out_forget_chunk;
		};
		/*
		//=====================================================================================
		*/
		struct registry_s
		{
			std::vector<std::uint8_t> config;
			std::uint32_t plains;
			std::uint32_t overworld;
			bool loaded;
		};
		/*
		//=====================================================================================
		*/
		enum login_format_e : std::uint32_t
		{
			login_format_undashed,
			login_format_dashed,
			login_format_binary,
			login_format_properties,
			login_format_strict,
			login_format_modern,
			login_format_session
		};
		/*
		//=====================================================================================
		*/
		struct huffman_s
		{
			std::array<std::int16_t, deflate_max_bits + 1u> counts;
			std::array<std::int16_t, 288u> symbols;
		};
		/*
		//=====================================================================================
		*/
		struct reader_s
		{
			const std::uint8_t* data;
			std::size_t size;
			std::size_t cursor;
			bool valid;

			bool good() const { return valid; }
			std::size_t remaining() const { return (size - cursor); }
		};
		/*
		//=====================================================================================
		*/
		enum operation_e : std::uint32_t
		{
			operation_accept,
			operation_receive,
			operation_send,
			operation_datagram
		};
		/*
		//=====================================================================================
		*/
		struct datagram_s
		{
			OVERLAPPED overlapped;
			operation_e type;
			sockaddr_in from;
			std::int32_t from_length;
			DWORD flags;
			std::array<std::uint8_t, max_datagram_size> data;
		};
		/*
		//=====================================================================================
		*/
		struct split_s
		{
			std::uint32_t count;
			std::uint32_t received;
			std::uint8_t reliability;
			std::uint32_t order_index;
			std::vector<std::vector<std::uint8_t>> parts;
		};
		/*
		//=====================================================================================
		*/
		struct resend_s
		{
			std::vector<std::uint8_t> datagram;
			std::uint64_t sent_ms;
		};
		/*
		//=====================================================================================
		*/
		enum edition_e : std::uint32_t
		{
			edition_java,
			edition_bedrock
		};
		/*
		//=====================================================================================
		*/
		struct skin_s
		{
			std::string id;
			std::string patch;
			std::string geometry;
			std::string engine;
			std::vector<std::uint8_t> image;
			std::uint32_t width;
			std::uint32_t height;
			std::uint8_t arm;
			bool premium;
		};
		/*
		//=====================================================================================
		*/
		struct player_s
		{
			std::int32_t entity_id;
			std::uint64_t uuid_high;
			std::uint64_t uuid_low;
			std::string name;
			std::double_t x;
			std::double_t y;
			std::double_t z;
			std::float_t yaw;
			std::float_t pitch;
			bool on_ground;
			bool moved;
			bool spawned;
			bool sidebar;
			bool chat_visible;
			char color;
			edition_e edition;
			std::int32_t platform;
			void* link;
			std::shared_ptr<const skin_s> skin;
		};
		/*
		//=====================================================================================
		*/
		struct actor_data_s
		{
			std::uint16_t id;
			std::uint8_t kind;
			std::int64_t integer;
			std::int64_t self_integer;
			std::float_t x;
			std::float_t y;
			std::float_t z;
			const char* text;
			bool self;
		};
		/*
		//=====================================================================================
		*/
		struct session_s
		{
			sockaddr_in address;
			std::uint64_t guid;
			std::uint16_t mtu;
			bool connected;
			bool alive;
			bool queued;
			std::uint64_t seen_ms;

			std::uint32_t send_sequence;
			std::uint32_t reliable_index;
			std::uint32_t order_index;
			std::uint16_t split_id;
			std::vector<std::uint8_t> outgoing;
			std::map<std::uint32_t, resend_s> unacked;

			std::vector<std::uint32_t> acks;
			std::uint32_t highest_sequence;
			bool any_sequence;
			std::uint32_t reliable_floor;
			std::set<std::uint32_t> reliable_seen;
			std::uint32_t expected_order;
			std::map<std::uint32_t, std::vector<std::uint8_t>> order_queue;
			std::unordered_map<std::uint16_t, split_s> splits;

			bool compression;
			bool playing;
			std::int32_t protocol;
			std::int64_t entity_id;
			std::string name;
			std::string xuid;
			player_s player;
			std::uint32_t inject_step;
			std::uint64_t inject_ms;
		};
		/*
		//=====================================================================================
		*/
		struct client_s;
		/*
		//=====================================================================================
		*/
		struct operation_s
		{
			OVERLAPPED overlapped;
			operation_e type;
			client_s* client;
			SOCKET handle;
			std::array<std::uint8_t, accept_address_space> addresses;
		};
		/*
		//=====================================================================================
		*/
		struct terrain_state_s
		{
			std::uint32_t java_26_2;
			std::uint32_t java_26_3;
			std::uint8_t flags;
		};
		/*
		//=====================================================================================
		*/
		struct terrain_section_s
		{
			std::uint32_t palette;
			std::uint32_t longs;
			std::uint16_t palette_size;
			std::uint16_t non_air;
			std::uint16_t fluid;
			std::uint8_t bits;
		};
		/*
		//=====================================================================================
		*/
		struct terrain_column_s
		{
			std::uint32_t mask;
			std::uint32_t refs;
		};
		/*
		//=====================================================================================
		*/
		struct chunk_cache_s
		{
			std::shared_ptr<const std::vector<std::uint8_t>> packet;
			std::uint64_t last_use;
		};
		/*
		//=====================================================================================
		*/
		struct segment_s
		{
			const std::uint8_t* shared;
			std::size_t shared_size;
			std::vector<std::uint8_t> owned;
			std::shared_ptr<const std::vector<std::uint8_t>> hold;

			const std::uint8_t* data() const { return shared ? shared : owned.data(); }
			std::size_t size() const { return shared ? shared_size : owned.size(); }
		};
		/*
		//=====================================================================================
		*/
		struct client_s
		{
			SOCKET handle;
			std::string address;
			state_e state;
			family_e family;
			std::int32_t protocol;

			operation_s receive_operation;
			operation_s send_operation;
			std::uint32_t operations;

			std::vector<std::uint8_t> rx;
			std::size_t rx_cursor;
			std::deque<segment_s> tx;
			std::size_t tx_cursor;
			std::size_t tx_bytes;
			std::uint32_t in_flight;
			bool sending;
			bool queued;

			layout_s layout;
			player_s player;
			std::string name;
			std::uint64_t uuid_high;
			std::uint64_t uuid_low;
			std::int32_t entity_id;

			std::int64_t keep_alive_id;
			std::int32_t view_x;
			std::int32_t view_z;
			std::uint64_t connected_ms;
			std::uint64_t keep_alive_sent_ms;
			std::uint64_t keep_alive_seen_ms;
			std::uint64_t closing_ms;

			bool alive;
			bool closing;

			bool active() const { return (alive && !closing); }
			bool named() const { return (name.size() > 0u); }
			bool pending() const { return (tx_bytes > 0u); }
			bool playing() const { return (state == state_play); }
			bool finished() const { return (!alive && operations == 0u); }
		};
	}

	namespace functions
	{
		static std::uint64_t get_time_ms()
		{
			return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
		}
		/*
		//=====================================================================================
		*/
		static std::string get_local_path(const char* file_name)
		{
			std::array<char, MAX_PATH> path{};
			std::string_view folder{ path.data(), GetModuleFileNameA(nullptr, path.data(), static_cast<DWORD>(path.size())) };
			return std::string{ folder.substr(0u, folder.find_last_of('\\') + 1u) }.append(file_name);
		}
		/*
		//=====================================================================================
		*/
		static void write_bytes(std::vector<std::uint8_t>& out, const void* data, std::size_t size)
		{
			out.insert(out.end(), static_cast<const std::uint8_t*>(data), static_cast<const std::uint8_t*>(data) + size);
		}
		/*
		//=====================================================================================
		*/
		static void write_u8(std::vector<std::uint8_t>& out, std::uint8_t value)
		{
			out.push_back(value);
		}
		/*
		//=====================================================================================
		*/
		static void write_bool(std::vector<std::uint8_t>& out, bool value)
		{
			out.push_back(static_cast<std::uint8_t>(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_u16(std::vector<std::uint8_t>& out, std::uint16_t value)
		{
			auto swapped{ _byteswap_ushort(value) };
			write_bytes(out, &swapped, sizeof(swapped));
		}
		/*
		//=====================================================================================
		*/
		static void write_u32(std::vector<std::uint8_t>& out, std::uint32_t value)
		{
			auto swapped{ _byteswap_ulong(value) };
			write_bytes(out, &swapped, sizeof(swapped));
		}
		/*
		//=====================================================================================
		*/
		static void write_u64(std::vector<std::uint8_t>& out, std::uint64_t value)
		{
			auto swapped{ _byteswap_uint64(value) };
			write_bytes(out, &swapped, sizeof(swapped));
		}
		/*
		//=====================================================================================
		*/
		static void write_i32(std::vector<std::uint8_t>& out, std::int32_t value)
		{
			write_u32(out, static_cast<std::uint32_t>(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_i64(std::vector<std::uint8_t>& out, std::int64_t value)
		{
			write_u64(out, static_cast<std::uint64_t>(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_f32(std::vector<std::uint8_t>& out, std::float_t value)
		{
			write_u32(out, std::bit_cast<std::uint32_t>(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_f64(std::vector<std::uint8_t>& out, std::double_t value)
		{
			write_u64(out, std::bit_cast<std::uint64_t>(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_varint(std::vector<std::uint8_t>& out, std::int32_t value)
		{
			auto bits{ static_cast<std::uint32_t>(value) };

			while (bits >= 0x80u)
			{
				out.push_back(static_cast<std::uint8_t>(bits | 0x80u));
				bits >>= 7u;
			}

			out.push_back(static_cast<std::uint8_t>(bits));
		}
		/*
		//=====================================================================================
		*/
		static void write_string(std::vector<std::uint8_t>& out, std::string_view text)
		{
			write_varint(out, static_cast<std::int32_t>(text.size()));
			write_bytes(out, text.data(), text.size());
		}
		/*
		//=====================================================================================
		*/
		static void write_json_text(std::vector<std::uint8_t>& out, std::string_view text)
		{
			std::string json{ "{\"text\":\"" };

			for (auto character : text)
			{
				if (character == '"' || character == '\\')
				{
					json.push_back('\\');
					json.push_back(character);
				}

				else if (static_cast<std::uint8_t>(character) < 0x20u)
				{
					std::array<char, 8u> escaped{};
					std::snprintf(escaped.data(), escaped.size(), "\\u%04x", static_cast<std::uint32_t>(static_cast<std::uint8_t>(character)));
					json.append(escaped.data());
				}

				else
				{
					json.push_back(character);
				}
			}

			write_string(out, json.append("\"}"));
		}
		/*
		//=====================================================================================
		*/
		static void write_modified_utf8(std::vector<std::uint8_t>& out, std::uint32_t point)
		{
			if (point && point < 0x80u)
			{
				out.push_back(static_cast<std::uint8_t>(point));
			}

			else if (point < 0x800u)
			{
				out.push_back(static_cast<std::uint8_t>(0xC0u | (point >> 6u)));
				out.push_back(static_cast<std::uint8_t>(0x80u | (point & 0x3Fu)));
			}

			else
			{
				out.push_back(static_cast<std::uint8_t>(0xE0u | (point >> 12u)));
				out.push_back(static_cast<std::uint8_t>(0x80u | ((point >> 6u) & 0x3Fu)));
				out.push_back(static_cast<std::uint8_t>(0x80u | (point & 0x3Fu)));
			}
		}
		/*
		//=====================================================================================
		*/
		static void write_nbt_text(std::vector<std::uint8_t>& out, std::string_view text)
		{
			write_u8(out, 0x08u);
			auto start{ out.size() };
			write_u16(out, 0u);

			for (auto index{ 0u }; index < text.size() && out.size() - start < 0xFFF0u;)
			{
				auto lead{ static_cast<std::uint8_t>(text[index]) };
				auto length{ lead < 0x80u ? 1u : (lead >> 5u) == 0x06u ? 2u : (lead >> 4u) == 0x0Eu ? 3u : (lead >> 3u) == 0x1Eu ? 4u : 0u };
				auto point{ length == 1u ? lead : length == 2u ? lead & 0x1Fu : length == 3u ? lead & 0x0Fu : lead & 0x07u };

				for (auto next{ 1u }; next < length; ++next)
				{
					if (index + next < text.size() && (static_cast<std::uint8_t>(text[index + next]) >> 6u) == 0x02u)
					{
						point = (point << 6u) | (static_cast<std::uint8_t>(text[index + next]) & 0x3Fu);
					}

					else
					{
						length = 0u;
					}
				}

				if (length && point && point < 0x10000u && (point < 0xD800u || point > 0xDFFFu))
				{
					write_modified_utf8(out, point);
				}

				else if (length == 4u && point >= 0x10000u && point < 0x110000u)
				{
					write_modified_utf8(out, 0xD800u + ((point - 0x10000u) >> 10u));
					write_modified_utf8(out, 0xDC00u + ((point - 0x10000u) & 0x3FFu));
				}

				else
				{
					write_modified_utf8(out, '?');
				}

				index += length ? length : 1u;
			}

			auto size{ static_cast<std::uint16_t>(out.size() - start - 2u) };
			out[start] = static_cast<std::uint8_t>(size >> 8u);
			out[start + 1u] = static_cast<std::uint8_t>(size);
		}
		/*
		//=====================================================================================
		*/
		static void write_bitset(std::vector<std::uint8_t>& out, std::uint64_t mask, bool byte_form)
		{
			if (byte_form)
			{
				auto length{ (64u - static_cast<std::uint32_t>(std::countl_zero(mask)) + 7u) / 8u };
				write_varint(out, static_cast<std::int32_t>(length));

				for (auto index{ 0u }; index < length; ++index)
				{
					write_u8(out, static_cast<std::uint8_t>(mask >> (index * 8u)));
				}
			}

			else if (mask)
			{
				write_varint(out, 1);
				write_u64(out, mask);
			}

			else
			{
				write_varint(out, 0);
			}
		}
		/*
		//=====================================================================================
		*/
		static void write_u16_le(std::vector<std::uint8_t>& out, std::uint16_t value)
		{
			write_bytes(out, &value, sizeof(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_u24_le(std::vector<std::uint8_t>& out, std::uint32_t value)
		{
			write_bytes(out, &value, 3u);
		}
		/*
		//=====================================================================================
		*/
		static void write_u32_le(std::vector<std::uint8_t>& out, std::uint32_t value)
		{
			write_bytes(out, &value, sizeof(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_f32_le(std::vector<std::uint8_t>& out, std::float_t value)
		{
			write_bytes(out, &value, sizeof(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_varint64(std::vector<std::uint8_t>& out, std::uint64_t value)
		{
			while (value >= 0x80ui64)
			{
				out.push_back(static_cast<std::uint8_t>(value | 0x80ui64));
				value >>= 7u;
			}

			out.push_back(static_cast<std::uint8_t>(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_zigzag64(std::vector<std::uint8_t>& out, std::int64_t value)
		{
			write_varint64(out, (static_cast<std::uint64_t>(value) << 1u) ^ static_cast<std::uint64_t>(value >> 63));
		}
		/*
		//=====================================================================================
		*/
		static void write_u64_le(std::vector<std::uint8_t>& out, std::uint64_t value)
		{
			write_bytes(out, &value, sizeof(value));
		}
		/*
		//=====================================================================================
		*/
		static void write_zigzag32(std::vector<std::uint8_t>& out, std::int32_t value)
		{
			write_varint(out, static_cast<std::int32_t>((static_cast<std::uint32_t>(value) << 1u) ^ static_cast<std::uint32_t>(value >> 31)));
		}
		/*
		//=====================================================================================
		*/
		static void write_address(std::vector<std::uint8_t>& out, const sockaddr_in& address)
		{
			write_u8(out, 4u);

			for (auto index{ 0u }; index < 4u; ++index)
			{
				write_u8(out, static_cast<std::uint8_t>(~reinterpret_cast<const std::uint8_t*>(&address.sin_addr)[index]));
			}

			write_bytes(out, &address.sin_port, sizeof(address.sin_port));
		}
		/*
		//=====================================================================================
		*/
		static void write_frame(std::vector<std::uint8_t>& out, const std::vector<std::uint8_t>& body)
		{
			write_varint(out, static_cast<std::int32_t>(body.size()));
			write_bytes(out, body.data(), body.size());
		}
		/*
		//=====================================================================================
		*/
		static bool read_raw(structures::reader_s& in, void* out, std::size_t size)
		{
			if (in.good() && in.remaining() >= size)
			{
				std::memcpy(out, in.data + in.cursor, size);
				in.cursor += size;
				return true;
			}

			in.valid = false;
			return false;
		}
		/*
		//=====================================================================================
		*/
		static std::uint8_t read_u8(structures::reader_s& in)
		{
			std::uint8_t value{ 0u };
			read_raw(in, &value, sizeof(value));
			return value;
		}
		/*
		//=====================================================================================
		*/
		static std::uint16_t read_u16(structures::reader_s& in)
		{
			std::uint16_t value{ 0u };
			read_raw(in, &value, sizeof(value));
			return _byteswap_ushort(value);
		}
		/*
		//=====================================================================================
		*/
		static std::uint32_t read_u24_le(structures::reader_s& in)
		{
			std::uint32_t value{ 0u };
			read_raw(in, &value, 3u);
			return value;
		}
		/*
		//=====================================================================================
		*/
		static std::uint32_t read_u32(structures::reader_s& in)
		{
			std::uint32_t value{ 0u };
			read_raw(in, &value, sizeof(value));
			return _byteswap_ulong(value);
		}
		/*
		//=====================================================================================
		*/
		static std::uint32_t read_u32_le(structures::reader_s& in)
		{
			std::uint32_t value{ 0u };
			read_raw(in, &value, sizeof(value));
			return value;
		}
		/*
		//=====================================================================================
		*/
		static std::uint64_t read_u64(structures::reader_s& in)
		{
			std::uint64_t value{ 0ui64 };
			read_raw(in, &value, sizeof(value));
			return _byteswap_uint64(value);
		}
		/*
		//=====================================================================================
		*/
		static std::int64_t read_i64(structures::reader_s& in)
		{
			return static_cast<std::int64_t>(read_u64(in));
		}
		/*
		//=====================================================================================
		*/
		static std::double_t read_f64(structures::reader_s& in)
		{
			return std::bit_cast<std::double_t>(read_u64(in));
		}
		/*
		//=====================================================================================
		*/
		static std::int32_t read_varint(structures::reader_s& in)
		{
			auto value{ 0u };
			auto shift{ 0u };
			auto more{ true };

			while (more && in.good())
			{
				if (shift < 35u)
				{
					auto byte{ read_u8(in) };
					value |= static_cast<std::uint32_t>(byte & 0x7Fu) << shift;
					more = ((byte & 0x80u) != 0u);
					shift += 7u;
				}

				else
				{
					in.valid = false;
				}
			}

			return static_cast<std::int32_t>(value);
		}
		/*
		//=====================================================================================
		*/
		static std::int32_t read_zigzag32(structures::reader_s& in)
		{
			auto value{ static_cast<std::uint32_t>(read_varint(in)) };
			return static_cast<std::int32_t>((value >> 1u) ^ (0u - (value & 1u)));
		}
		/*
		//=====================================================================================
		*/
		static std::string_view read_string(structures::reader_s& in, std::size_t max_characters)
		{
			std::string_view text{};

			if (auto length{ read_varint(in) }; in.good() && length >= 0 && static_cast<std::size_t>(length) <= max_characters * 4u && in.remaining() >= static_cast<std::size_t>(length))
			{
				text = std::string_view{ reinterpret_cast<const char*>(in.data + in.cursor), static_cast<std::size_t>(length) };
				in.cursor += static_cast<std::size_t>(length);
			}

			else
			{
				in.valid = false;
			}

			return text;
		}
		/*
		//=====================================================================================
		*/
		static const char* get_version_name(std::int32_t protocol)
		{
			auto name{ "unknown" };

			for (auto index{ 0u }; index < std::size(known_protocols); ++index)
			{
				if (known_protocols[index] == protocol)
				{
					name = known_names[index];
				}
			}

			return name;
		}
		/*
		//=====================================================================================
		*/
		static structures::family_e get_family(std::int32_t protocol)
		{
			auto family{ structures::family_none };

			if (protocol == protocol_26_3)
			{
				family = structures::family_26_3;
			}

			else if (protocol == protocol_26_2)
			{
				family = structures::family_26_2;
			}

			else if (protocol == protocol_1_8)
			{
				family = structures::family_1_8;
			}

			return family;
		}
		/*
		//=====================================================================================
		*/
		static bool is_modern(structures::family_e family)
		{
			return (family >= structures::family_26_2 && family < structures::family_count);
		}
		/*
		//=====================================================================================
		*/
		static structures::layout_s get_layout(structures::family_e family)
		{
			structures::layout_s layout{};

			switch (family)
			{
			case structures::family_26_2:
				layout = { .pack_version = "26.2", .data_path = "data/java/26_2.txt", .config_out_disconnect = 0x02, .config_out_features = 0x0C, .config_out_known_packs = 0x0E, .config_out_tags = 0x0D, .config_in_finish = 0x03, .play_out_disconnect = 0x20, .play_out_game_event = 0x26, .play_out_keep_alive = 0x2C, .play_out_chunk = 0x2D, .play_out_login = 0x31, .play_out_position = 0x48, .play_out_center_chunk = 0x5E, .play_in_keep_alive = 0x1C, .play_in_move_position = 0x1E, .play_in_move_position_rotation = 0x1F, .play_in_move_rotation = 0x20, .play_out_add_entity = 0x01, .play_out_player_info_update = 0x46, .play_out_player_info_remove = 0x45, .play_out_teleport_entity = 0x7D, .play_out_rotate_head = 0x53, .play_out_remove_entities = 0x4D, .play_in_chat = 0x09, .play_out_system_chat = 0x79, .play_out_set_objective = 0x6A, .play_out_set_display_objective = 0x62, .play_out_set_score = 0x6E, .play_out_reset_score = 0x4F, .play_in_chat_command = 0x07, .play_out_commands = 0x10, .player_entity_type = 156, .grass_block = 9u, .byte_bitsets = false, .optional_previous_gamemode = false, .direct_bits = 15u, .play_out_forget_chunk = 0x25 };
				break;

			case structures::family_26_3:
				layout = { .pack_version = "26.3", .data_path = "data/java/26_3.txt", .config_out_disconnect = 0x02, .config_out_features = 0x0D, .config_out_known_packs = 0x0F, .config_out_tags = 0x0E, .config_in_finish = 0x03, .play_out_disconnect = 0x20, .play_out_game_event = 0x27, .play_out_keep_alive = 0x2D, .play_out_chunk = 0x2E, .play_out_login = 0x32, .play_out_position = 0x49, .play_out_center_chunk = 0x60, .play_in_keep_alive = 0x1C, .play_in_move_position = 0x1E, .play_in_move_position_rotation = 0x1F, .play_in_move_rotation = 0x20, .play_out_add_entity = 0x01, .play_out_player_info_update = 0x47, .play_out_player_info_remove = 0x46, .play_out_teleport_entity = 0x80, .play_out_rotate_head = 0x55, .play_out_remove_entities = 0x4E, .play_in_chat = 0x09, .play_out_system_chat = 0x7C, .play_out_set_objective = 0x6C, .play_out_set_display_objective = 0x64, .play_out_set_score = 0x70, .play_out_reset_score = 0x50, .play_in_chat_command = 0x07, .play_out_commands = 0x10, .player_entity_type = 159, .grass_block = 9u, .byte_bitsets = true, .optional_previous_gamemode = true, .direct_bits = 16u, .play_out_forget_chunk = 0x26 };
				break;
			}

			return layout;
		}
		/*
		//=====================================================================================
		*/
		static structures::login_format_e get_login_format(std::int32_t protocol)
		{
			auto format{ structures::login_format_undashed };

			if (protocol >= protocol_26_2)
			{
				format = structures::login_format_session;
			}

			else if (protocol >= protocol_1_21_2)
			{
				format = structures::login_format_modern;
			}

			else if (protocol >= protocol_1_20_5)
			{
				format = structures::login_format_strict;
			}

			else if (protocol >= protocol_1_19)
			{
				format = structures::login_format_properties;
			}

			else if (protocol >= protocol_1_16)
			{
				format = structures::login_format_binary;
			}

			else if (protocol >= protocol_1_7_6)
			{
				format = structures::login_format_dashed;
			}

			return format;
		}
		/*
		//=====================================================================================
		*/
		static bool skips_configuration(std::int32_t protocol)
		{
			return (protocol < protocol_1_20_2);
		}
		/*
		//=====================================================================================
		*/
		static bool is_name_character(char c)
		{
			return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
		}
		/*
		//=====================================================================================
		*/
		static bool is_valid_name(std::string_view name)
		{
			return (name.size() >= 1u && name.size() <= max_name_length && std::all_of(name.begin(), name.end(), is_name_character));
		}
		/*
		//=====================================================================================
		*/
		static std::uint64_t mix_bits(std::uint64_t value)
		{
			value += 0x9E3779B97F4A7C15ui64;
			value = (value ^ (value >> 30u)) * 0xBF58476D1CE4E5B9ui64;
			value = (value ^ (value >> 27u)) * 0x94D049BB133111EBui64;
			return value ^ (value >> 31u);
		}
		/*
		//=====================================================================================
		*/
		static void make_offline_uuid(std::string_view name, std::uint64_t& high, std::uint64_t& low)
		{
			high = 0xCBF29CE484222325ui64;
			low = 0x9E3779B97F4A7C15ui64;

			for (auto c : name)
			{
				high = mix_bits(high ^ static_cast<std::uint8_t>(c));
				low = mix_bits(low + static_cast<std::uint8_t>(c) + high);
			}

			high = (high & 0xFFFFFFFFFFFF0FFFui64) | 0x0000000000003000ui64;
			low = (low & 0x3FFFFFFFFFFFFFFFui64) | 0x8000000000000000ui64;
		}
		/*
		//=====================================================================================
		*/
		static std::string format_uuid(std::uint64_t high, std::uint64_t low, bool dashed)
		{
			std::array<char, 40u> text{};
			std::snprintf(text.data(), text.size(), dashed ? "%08llx-%04llx-%04llx-%04llx-%012llx" : "%08llx%04llx%04llx%04llx%012llx", high >> 32u, (high >> 16u) & 0xFFFFui64, high & 0xFFFFui64, low >> 48u, low & 0xFFFFFFFFFFFFui64);
			return std::string{ text.data() };
		}
		/*
		//=====================================================================================
		*/
		static std::uint64_t get_address_key(const sockaddr_in& address)
		{
			return (static_cast<std::uint64_t>(address.sin_addr.s_addr) << 16u) | address.sin_port;
		}
		/*
		//=====================================================================================
		*/
		static std::string format_address(const sockaddr_in& address)
		{
			std::array<char, INET_ADDRSTRLEN> text{};
			inet_ntop(AF_INET, &address.sin_addr, text.data(), text.size());
			return std::string{ text.data() }.append(":").append(std::to_string(ntohs(address.sin_port)));
		}
		/*
		//=====================================================================================
		*/
		static bool has_magic(structures::reader_s& in)
		{
			std::array<std::uint8_t, sizeof(raknet_magic)> magic{};
			return (read_raw(in, magic.data(), magic.size()) && std::memcmp(magic.data(), raknet_magic, magic.size()) == 0);
		}
		/*
		//=====================================================================================
		*/
		static std::uint32_t get_base64_value(char c)
		{
			auto value{ 64u };

			if (c >= 'A' && c <= 'Z')
			{
				value = static_cast<std::uint32_t>(c - 'A');
			}

			else if (c >= 'a' && c <= 'z')
			{
				value = static_cast<std::uint32_t>(c - 'a') + 26u;
			}

			else if (c >= '0' && c <= '9')
			{
				value = static_cast<std::uint32_t>(c - '0') + 52u;
			}

			else if (c == '-' || c == '+')
			{
				value = 62u;
			}

			else if (c == '_' || c == '/')
			{
				value = 63u;
			}

			return value;
		}
		/*
		//=====================================================================================
		*/
		static std::string decode_base64(std::string_view text)
		{
			std::string out{};
			auto bits{ 0u };
			auto count{ 0u };

			for (auto c : text)
			{
				if (auto value{ get_base64_value(c) }; value < 64u)
				{
					bits = (bits << 6u) | value;
					count += 6u;

					if (count >= 8u)
					{
						count -= 8u;
						out.push_back(static_cast<char>((bits >> count) & 0xFFu));
					}
				}
			}

			return out;
		}
		/*
		//=====================================================================================
		*/
		static std::string_view find_json_string(std::string_view text, std::string_view key)
		{
			std::string_view value{};

			if (auto at{ text.find(std::string{ "\"" }.append(key).append("\"")) }; at != std::string_view::npos)
			{
				if (auto open{ text.find('"', text.find(':', at + key.size() + 2u) + 1u) }; open != std::string_view::npos && text.find('"', open + 1u) != std::string_view::npos)
				{
					value = text.substr(open + 1u, text.find('"', open + 1u) - open - 1u);
				}
			}

			return value;
		}
		/*
		//=====================================================================================
		*/
		static std::span<const structures::actor_data_s> get_actor_baseline()
		{
			static constexpr structures::actor_data_s table[] = { { 0u, 7u, 844424930656256i64, 844424930656256i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 1u, 2u, 1i64, 1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 2u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 3u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 4u, 4u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 5u, 7u, -1i64, -1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 6u, 7u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 7u, 1u, 300i64, 300i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 10u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, false }, { 22u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 26u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 27u, 2u, 2i64, 1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 28u, 6u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 37u, 7u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 38u, 3u, 0i64, 0i64, 1.0f, 0.0f, 0.0f, nullptr, false }, { 39u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 42u, 1u, 300i64, 300i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 43u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 44u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 45u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 46u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 53u, 3u, 0i64, 0i64, 0.6000000238418579f, 0.0f, 0.0f, nullptr, false }, { 54u, 3u, 0i64, 0i64, 1.7999999523162842f, 0.0f, 0.0f, nullptr, false }, { 55u, 2u, -1i64, -1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 56u, 8u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, false }, { 57u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 58u, 3u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 59u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 60u, 3u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 70u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 71u, 4u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, "", true }, { 72u, 4u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, "", true }, { 73u, 0u, 1i64, 1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 74u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, false }, { 75u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 76u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 77u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 78u, 2u, -1i64, -1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 81u, 0u, 1i64, 1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 82u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 84u, 4u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, "", true }, { 88u, 7u, -1i64, -1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 91u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 92u, 7u, 35184405643264i64, 35184405643264i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 100u, 4u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, "", true }, { 101u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 102u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 103u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 104u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 106u, 2u, 3i64, 3i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 107u, 0u, 1i64, 1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 108u, 3u, 0i64, 0i64, 8.0f, 0.0f, 0.0f, nullptr, true }, { 109u, 3u, 0i64, 0i64, 16.0f, 0.0f, 0.0f, nullptr, true }, { 110u, 4u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, "ambient", true }, { 111u, 3u, 0i64, 0i64, 1.0f, 0.0f, 0.0f, nullptr, true }, { 114u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 115u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 116u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 117u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 118u, 5u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 119u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 120u, 3u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 124u, 3u, 0i64, 0i64, 1.0f, 0.0f, 0.0f, nullptr, true }, { 125u, 2u, 20i64, 20i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 126u, 2u, 614i64, 614i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 127u, 6u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 128u, 2u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 129u, 0u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 130u, 8u, 0i64, 0i64, 0.6000000238418579f, 1.7999999523162842f, 0.0f, nullptr, true }, { 131u, 7u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 132u, 4u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, "", true }, { 133u, 8u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 134u, 3u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 135u, 3u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 136u, 2u, -1i64, -1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 137u, 2u, -1i64, -1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 138u, 2u, -1i64, -1i64, 0.0f, 0.0f, 0.0f, nullptr, true }, { 139u, 7u, 0i64, 0i64, 0.0f, 0.0f, 0.0f, nullptr, false }, { 140u, 3u, 0i64, 0i64, 64.0f, 0.0f, 0.0f, nullptr, true } };
			return table;
		}
		/*
		//=====================================================================================
		*/
		static std::uint8_t get_angle_byte(std::float_t degrees)
		{
			return static_cast<std::uint8_t>(static_cast<std::int32_t>(std::floor(degrees * 256.0f / 360.0f)) & 0xFF);
		}
		/*
		//=====================================================================================
		*/
		static std::string get_java_name(std::string_view name)
		{
			std::string out{ name.substr(0u, max_name_length) };
			std::replace_if(out.begin(), out.end(), [](char c) { return !is_name_character(c); }, '_');
			return out;
		}
		/*
		//=====================================================================================
		*/
		static std::string_view find_json_literal(std::string_view text, std::string_view key)
		{
			std::string_view value{};

			if (auto at{ text.find(std::string{ "\"" }.append(key).append("\":")) }; at != std::string_view::npos)
			{
				auto start{ text.find_first_not_of(' ', at + key.size() + 3u) };
				auto end{ text.find_first_of(",}", start) };
				value = text.substr(start, end - start);
			}

			return value;
		}
		/*
		//=====================================================================================
		*/
		static std::uint32_t find_json_number(std::string_view text, std::string_view key)
		{
			auto literal{ find_json_literal(text, key) };
			std::uint32_t value{ 0u };
			std::from_chars(literal.data(), literal.data() + literal.size(), value);
			return value;
		}
		/*
		//=====================================================================================
		*/
		static std::uint64_t encode_position_1_8(std::int32_t x, std::int32_t y, std::int32_t z)
		{
			return ((static_cast<std::uint64_t>(x) & 0x3FFFFFFui64) << 38u) | ((static_cast<std::uint64_t>(y) & 0xFFFui64) << 26u) | (static_cast<std::uint64_t>(z) & 0x3FFFFFFui64);
		}
	}

	static_assert(std::size(known_protocols) == std::size(known_names));
	static_assert(sizeof(std::float_t) == 4u);
	static_assert(sizeof(std::double_t) == 8u);
	static_assert(get_section_index(spawn_block_x, spawn_block_y, spawn_block_z) == 2940u);
	static_assert(spawn_platform_blocks == 25);
	static_assert(is_platform_index(get_section_index(spawn_block_x + 2, spawn_block_y, spawn_block_z - 2)) && is_platform_bedrock_index(get_bedrock_index(spawn_block_x - 2, spawn_block_y, spawn_block_z + 2)));
}

//=====================================================================================
