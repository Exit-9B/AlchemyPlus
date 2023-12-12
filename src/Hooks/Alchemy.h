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
		// Round magnitude/duration when creating effects
		static void RoundEffectivenessPatch();

		static void ModifyAlchemyItem(
			RE::TESDataHandler* a_dataHandler,
			RE::AlchemyItem* a_alchemyItem);

		static std::int32_t ModEffectiveness(void* a_functor, RE::Effect* a_effect);

		inline static REL::Relocation<decltype(&ModifyAlchemyItem)> _AddForm;
		inline static REL::Relocation<decltype(&ModEffectiveness)> _ModEffectiveness;
	};
}
