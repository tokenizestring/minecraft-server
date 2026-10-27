
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class generator_c;

	class flora_c
	{
	public:

		void place_trees(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z);
		void decorate(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z);

	private:

		void collect_trees(std::int32_t chunk_x, std::int32_t chunk_z);
		void build_tree(const structures::tree_plan_s& plan);
		void carve_tree(const structures::tree_plan_s& plan);
		void carve_bush(const structures::tree_plan_s& plan);
		void carve_ancient(const structures::tree_plan_s& plan);
		void carve_mega_spruce(const structures::tree_plan_s& plan);
		void carve_willow(const structures::tree_plan_s& plan);
		void carve_tall_birch(const structures::tree_plan_s& plan);
		void carve_spruce(const structures::tree_plan_s& plan);
		void carve_acacia(const structures::tree_plan_s& plan);
		void carve_broadleaf(const structures::tree_plan_s& plan);
		void anchor_tree(std::int32_t x, std::int32_t z, std::int32_t base_y, std::uint8_t log);
		void leaf_blob(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t radius, std::uint8_t leaves, std::uint64_t seed);
		void hang_vines(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t length, std::uint64_t seed);
		void branch(std::int32_t x, std::int32_t y, std::int32_t z, std::double_t dir_x, std::double_t dir_z, std::int32_t length, std::int32_t rise, std::uint8_t log, std::uint8_t leaves, std::uint64_t seed, std::int32_t blob);
		void root_flare(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t spread, std::uint8_t log, std::uint64_t seed);
		void canopy_cluster(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t spread, std::int32_t blobs, std::int32_t radius, std::uint8_t leaves, std::uint64_t seed);
		void limb(std::int32_t x, std::int32_t y, std::int32_t z, std::double_t dir_x, std::double_t dir_z, std::int32_t length, std::int32_t rise, std::uint8_t log, std::uint8_t leaves, std::uint64_t seed, std::int32_t blob, std::int32_t depth);
		void taper_trunk(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t height, std::int32_t base_width, std::uint8_t log, std::uint64_t seed);
		void plant(std::int32_t world_x, std::int32_t surface, std::int32_t world_z, std::uint32_t roll, std::uint64_t state, std::uint8_t patch_flower);
		void tall_plant(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint8_t lower, std::uint8_t upper);
		bool water_beside(std::int32_t local_x, std::int32_t y, std::int32_t local_z) const;
		bool open_below(std::int32_t x, std::int32_t y, std::int32_t z) const;

		static structures::wood_s wood_for(structures::tree_e kind);
		static std::int32_t tree_reach(structures::tree_e kind);
		static structures::tree_e kind_for(structures::biome_e biome, std::uint64_t roll);
		static std::int32_t density_for(structures::biome_e biome);
		static std::uint8_t patch_flower_for(std::uint64_t patch);

		generator_c* owner{ nullptr };
		std::vector<structures::tree_plan_s> trees;
	};
}

//=====================================================================================
