
//=====================================================================================

#include "../../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	growth_c growth;

	void growth_c::track(std::uint64_t key, const structures::edit_s& edit)
	{
		auto clock{ functions::get_clock() };
		cursor = cursor ? cursor : clock;

		if (crops.is_growing(edit.kind))
		{
			schedule(structures::growth_s{ key, edit.index, edit.stamp }, crops.get_ripen(edit, clock), clock);		}
	}
	/*
	//=====================================================================================
	*/
	void growth_c::tick()
	{
		auto clock{ functions::get_clock() };
		cursor = cursor && cursor <= clock + 1u ? std::max(cursor, clock + 1u - std::min(clock + 1u, growth_wheel_size)) : clock;

		while (cursor <= clock)
		{
			firing.clear();
			firing.swap(wheel[cursor % growth_wheel_size]);
			tracked -= firing.size();
			cursor += 1u;

			for (const auto& entry : firing)
			{
				ripen(entry, clock);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::size_t growth_c::get_tracked() const
	{
		return tracked;
	}
	/*
	//=====================================================================================
	*/
	void growth_c::schedule(const structures::growth_s& entry, std::uint32_t due, std::uint32_t clock)
	{
		if (due != growth_never)
		{
			wheel[std::clamp(due, clock + 1u, clock + growth_wheel_size - 1u) % growth_wheel_size].push_back(entry);
			tracked += 1u;
		}
	}
	/*
	//=====================================================================================
	*/
	void growth_c::ripen(const structures::growth_s& entry, std::uint32_t clock)
	{
		auto x{ static_cast<std::int32_t>(entry.key >> 32u) * 16 + static_cast<std::int32_t>(entry.index & 15u) };
		auto z{ static_cast<std::int32_t>(entry.key & 0xFFFFFFFFui64) * 16 + static_cast<std::int32_t>((entry.index >> 4u) & 15u) };
		auto y{ world_min_y + static_cast<std::int32_t>(entry.index >> 8u) };
		auto edit{ edits.find_edit(structures::world_smp, x, y, z) };
		if (crops.is_growing(edit.kind) && edit.stamp == entry.stamp)
		{
			auto shown{ crops.get_shown(edit, clock) };

			if (shown == crops.get_ripe(edit.kind))
			{
				edits.set(structures::world_smp, x, y, z, shown, 0u);
			}

			else
			{
				schedule(entry, crops.get_ripen(edit, clock), clock);
			}

			java_block.show(structures::world_smp, x, y, z, shown);
		}
	}
}

//=====================================================================================
