#pragma once

namespace Data
{
	class ItemTraits final
	{
	public:
		struct ItemDefinition
		{
			RE::BSFixedString name;
			RE::BSFixedString model;
		};

		using SortedDefs = std::map<float, ItemDefinition>;
		using EffectTable = std::map<const RE::EffectSetting*, SortedDefs>;
		using EffectFlag = RE::EffectSetting::EffectSettingData::Flag;

		ItemTraits(const ItemTraits&) = delete;
		ItemTraits(ItemTraits&&) = delete;
		ItemTraits& operator=(const ItemTraits&) = delete;
		ItemTraits& operator=(ItemTraits&&) = delete;

		static ItemTraits* GetSingleton();

		void AddItemDefinitions(RE::TESForm* a_form);

		void ModifyAlchemyItem(RE::AlchemyItem* a_alchemyItem);

	private:
		ItemTraits() = default;

		float GetPotency(RE::Effect* a_effect);

		ItemDefinition defaultItemDefinition;
		EffectTable alchemyEffects;
	};
}
