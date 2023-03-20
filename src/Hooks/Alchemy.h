#pragma once

namespace Hooks
{
	class Alchemy
	{
	public:
		Alchemy() = delete;

		static void Install();

	private:
		// Replace item model and name
		static void CreateItemPatch();
		// Round magnitude when creating effects
		static void RoundMagnitudePatch();
		// Round duration when creating effects
		static void RoundDurationPatch();

		static void ModifyAlchemyItem(
			RE::TESDataHandler* a_dataHandler,
			RE::AlchemyItem* a_alchemyItem);

		static float CalculateMagnitude(float a_magnitude);

		static float CalculateDuration(float a_duration);

		inline static REL::Relocation<decltype(&ModifyAlchemyItem)> _AddForm;
	};
}
