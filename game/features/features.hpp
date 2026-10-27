
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class generator_c;

	class features_c
	{
	public:

		void collect(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z);
		void place(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z);
		bool site_clear(std::int32_t world_x, std::int32_t world_z) const;

	private:

		void collect_lake(std::int32_t chunk_x, std::int32_t chunk_z);
		void collect_field(std::int32_t chunk_x, std::int32_t chunk_z);
		void collect_pyramid(std::int32_t chunk_x, std::int32_t chunk_z);
		void carve_lake(const structures::lake_plan_s& plan);
		void build_field(const structures::field_plan_s& plan);
		void build_pyramid(const structures::pyramid_plan_s& plan);

		static bool lake_biome(structures::biome_e biome);
		static bool field_covers(const structures::field_plan_s& plan, std::int32_t offset_x, std::int32_t offset_z, std::int32_t shape);
		static bool is_near(std::int32_t from_x, std::int32_t from_z, std::int32_t to_x, std::int32_t to_z);
		static std::double_t ellipse(std::int32_t offset_x, std::int32_t offset_z, std::int32_t radius_x, std::int32_t radius_z);

		generator_c* owner{ nullptr };
		std::vector<structures::lake_plan_s> lakes;
		std::vector<structures::field_plan_s> fields;
		std::vector<structures::pyramid_plan_s> pyramids;
	};
}

//=====================================================================================
