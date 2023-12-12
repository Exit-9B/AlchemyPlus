#include "Alchemy.h"

#include "RE/Offset.h"
#include "Settings/UserSettings.h"

#include <cmath>

namespace Hooks
{
	void Alchemy::Install()
	{
		CreateItemPatch();
		RoundEffectivenessPatch();
	}

	void Alchemy::CreateItemPatch()
	{
		static const auto hook =
			REL::Relocation<std::uintptr_t>(RE::Offset::AlchemyItem::CreateFromEffects, 0x16F);

		if (!REL::make_pattern<"E8">().match(hook.address())) {
			util::report_and_fail("Alchemy::CreateItemPatch failed to install"sv);
		}

		auto& trampoline = SKSE::GetTrampoline();
		_AddForm = trampoline.write_call<5>(hook.address(), &Alchemy::ModifyAlchemyItem);
	}

	void Alchemy::RoundEffectivenessPatch()
	{
		auto vtbl = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::AlchemyMenu::ModEffectivenessFunctor::Vtbl);

		_ModEffectiveness = vtbl.write_vfunc(1, &Alchemy::ModEffectiveness);
	}

	void Alchemy::ModifyAlchemyItem(
		RE::TESDataHandler* a_dataHandler,
		RE::AlchemyItem* a_alchemyItem)
	{
		using EffectFlag = RE::EffectSetting::EffectSettingData::Flag;

		const auto userSettings = Settings::UserSettings::GetSingleton();

		if (userSettings->copyExemplars.enabled) {
			if (auto exemplar = userSettings->copyExemplars.exemplars.FindClosest(a_alchemyItem)) {
				a_alchemyItem->fullName = exemplar->name;
				a_alchemyItem->model = exemplar->model;
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

			if (userSettings->mixtureNames.enabled) {
				std::string newName;

				if (SKSE::Translation::Translate(
						fmt::format("$AlchemyPlus_Impure{{{}}}", a_alchemyItem->GetFullName()),
						newName)) {

					a_alchemyItem->fullName = newName;
				}
			}

			if (userSettings->impureCostFix.enabled) {
				a_alchemyItem->data.costOverride = static_cast<std::int32_t>(cost);
				a_alchemyItem->data.flags.set(RE::AlchemyItem::AlchemyFlag::kCostOverride);
			}
		}
		else if (a_alchemyItem->effects.size() > 1) {

			if (userSettings->mixtureNames.enabled) {
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

		return _AddForm(a_dataHandler, a_alchemyItem);
	}

	std::int32_t Alchemy::ModEffectiveness(void* a_functor, RE::Effect* a_effect)
	{
		using EffectFlag = RE::EffectSetting::EffectSettingData::Flag;

		auto result = _ModEffectiveness(a_functor, a_effect);

		const auto userSettings = Settings::UserSettings::GetSingleton();
		const auto baseEffect = a_effect->baseEffect;

		if (baseEffect && baseEffect->data.flags & EffectFlag::kPowerAffectsMagnitude) {
			const auto roundMagnitude = userSettings->roundedPotency.GetMagnitudeSetting(
				baseEffect);

			const float magnitude = roundMagnitude.Apply(a_effect->effectItem.magnitude);

			a_effect->SetMagnitude(magnitude);
		}

		if (baseEffect && baseEffect->data.flags & EffectFlag::kPowerAffectsDuration) {
			const auto roundDuration = userSettings->roundedPotency.GetDurationSetting(baseEffect);

			const float duration = roundDuration.Apply(
				static_cast<float>(a_effect->effectItem.duration));

			a_effect->SetDuration(static_cast<std::int32_t>(duration));
		}

		return result;
	}
}
