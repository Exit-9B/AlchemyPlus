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

		static void ModifyAlchemyItem(
			RE::TESDataHandler* a_dataHandler,
			RE::AlchemyItem* a_alchemyItem);

		inline static REL::Relocation<decltype(&Alchemy::ModifyAlchemyItem)> _AddForm;
	};
}
