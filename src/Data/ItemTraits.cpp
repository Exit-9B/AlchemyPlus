#include "ItemTraits.h"

#include "Settings/INISettings.h"

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
			if (alchemyItem->effects.size() != 1)
				return;

			auto effect = alchemyItem->effects[0];
			auto& baseEffect = effect->baseEffect;
			float potency = GetPotency(effect);

			auto& defs = alchemyEffects[baseEffect];
			defs.try_emplace(potency, ItemDefinition{ alchemyItem->fullName, alchemyItem->model });
		}
	}

	void ItemTraits::ModifyAlchemyItem(RE::AlchemyItem* a_alchemyItem)
	{
		const auto iniSettings = Settings::INISettings::GetSingleton();

		if (iniSettings->bEnableBetterModels || iniSettings->bEnableBetterNames) {
			auto costliestEffect = a_alchemyItem->GetCostliestEffectItem();
			auto baseEffect = costliestEffect->baseEffect;
			float potency = GetPotency(costliestEffect);

			if (auto i = alchemyEffects.find(baseEffect); i != alchemyEffects.end()) {
				ItemDefinition def;
				if (util::find_closest_value(i->second, potency, def)) {
					if (iniSettings->bEnableBetterNames) {
						a_alchemyItem->fullName = def.name;
					}
					if (iniSettings->bEnableBetterModels) {
						a_alchemyItem->model = def.model;
					}
				}
			}
		}

		bool isPoison = a_alchemyItem->IsPoison();
		bool impure = false;
		float cost = 0.0f;
		for (auto& effect : a_alchemyItem->effects) {
			bool isHostile = effect->baseEffect->data.flags.all(EffectFlag::kHostile);
			if (isPoison == isHostile) {
				cost += effect->cost;
			}
			else {
				cost -= effect->cost;
				impure = true;
			}
		}

		if (cost < 0.0f) {
			cost = 0.0f;
		}

		if (impure) {

			if (iniSettings->bEnableMixtureNames) {
				std::string newName;

				if (SKSE::Translation::Translate(
						fmt::format("$AlchemyPlus_Impure{{{}}}", a_alchemyItem->GetFullName()),
						newName)) {

					a_alchemyItem->fullName = newName;
				}
			}

			if (iniSettings->bEnableImpureCostFix) {
				a_alchemyItem->data.costOverride = static_cast<int32_t>(cost);
				a_alchemyItem->data.flags.set(RE::AlchemyItem::AlchemyFlag::kCostOverride);
			}
		}
		else if (a_alchemyItem->effects.size() > 1) {

			if (iniSettings->bEnableMixtureNames) {
				std::string newName;

				if (SKSE::Translation::Translate(
						fmt::format(
							"$AlchemyPlus_Mixed{{{}}}{{{}}}",
							a_alchemyItem->GetFullName(),
							a_alchemyItem->effects.size() - 1),
						newName)) {

					a_alchemyItem->fullName = newName;
				}
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
