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

		static RE::AlchemyItem* ModifyAlchemyItem(RE::AlchemyItem* a_alchemyItem);
	};
}
