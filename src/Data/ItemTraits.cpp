#include "ItemTraits.h"

namespace Data
{
	ItemTraits* ItemTraits::GetSingleton()
	{
		static ItemTraits singleton{};
		return &singleton;
	}

	void ItemTraits::AddItemDefinitions(RE::TESForm* a_form)
	{
		if (!a_form)
			return;

		if (const auto levItem = a_form->As<RE::TESLevItem>()) {
			for (const auto& entry : levItem->entries) {
				AddItemDefinitions(entry.form);
			}
		}
		else if (const auto formList = a_form->As<RE::BGSListForm>()) {
			for (const auto& form : formList->forms) {
				AddItemDefinitions(form);
			}
		}
		else if (const auto alchemyItem = a_form->As<RE::AlchemyItem>()) {
			auto costliestEffect = alchemyItem->GetCostliestEffectItem();
			auto& baseEffect = costliestEffect->baseEffect;
			float potency = GetPotency(costliestEffect);

			auto& defs = alchemyEffects[baseEffect];
			defs.try_emplace(potency, ItemDefinition{ alchemyItem->fullName, alchemyItem->model });
		}
	}

	void ItemTraits::ModifyAlchemyItem(RE::AlchemyItem* a_alchemyItem)
	{
		auto costliestEffect = a_alchemyItem->GetCostliestEffectItem();
		auto baseEffect = costliestEffect->baseEffect;
		float potency = GetPotency(costliestEffect);

		if (auto i = alchemyEffects.find(baseEffect); i != alchemyEffects.end()) {
			ItemDefinition def;
			if (util::find_closest_value(i->second, potency, def)) {
				a_alchemyItem->fullName = def.name;
				a_alchemyItem->model = def.model;
			}
		}
	}

	float ItemTraits::GetPotency(RE::Effect* a_effect)
	{
		auto& baseEffect = a_effect->baseEffect;
		auto flags = baseEffect->data.flags;
		if (flags.all(EffectFlag::kPowerAffectsMagnitude)) {
			return a_effect->effectItem.magnitude;
		}
		else if (flags.all(EffectFlag::kPowerAffectsDuration)) {
			return static_cast<float>(a_effect->effectItem.duration);
		}
		else {
			return 0.0f;
		}
	}
}
