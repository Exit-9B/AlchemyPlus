#include "Alchemy.h"

#include "Data/ItemTraits.h"
#include "RE/Offset.h"
#include "Settings/INISettings.h"

#include <xbyak/xbyak.h>

#include <cmath>

namespace Hooks
{
	void Alchemy::Install()
	{
		const auto iniSettings = Settings::INISettings::GetSingleton();

		CreateItemPatch();

		if (iniSettings->bEnableRoundedPotency) {
			RoundMagnitudePatch();
			RoundDurationPatch();
		}
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

	void Alchemy::RoundMagnitudePatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::AlchemyMenu::ModEffectivenessFunctor::Invoke,
			0xA8);

		if (!REL::make_pattern<"F3 0F 11 44 24 38">().match(hook.address())) {
			util::report_and_fail("Failed to install Alchemy::RoundMagnitudePatch");
		}

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				mov(rax, reinterpret_cast<std::uintptr_t>(&Alchemy::CalculateMagnitude));
				call(rax);
				mov(rdx, ptr[rbx + 0x10]);
				movss(ptr[rsp + 0x38], xmm0);
				jmp(ptr[rip]);
				dq(hook.address() + 0x6);
			}
		};

		auto patch = new Patch();
		patch->ready();

		auto& trampoline = SKSE::GetTrampoline();
		trampoline.write_branch<6>(hook.address(), patch->getCode());
	}

	void Alchemy::RoundDurationPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::AlchemyMenu::ModEffectivenessFunctor::Invoke,
			0x120);

		if (!REL::make_pattern<"0F 5B C0 F3 0F 2C F8">().match(hook.address())) {
			util::report_and_fail("Failed to install Alchemy::RoundDurationPatch");
		}

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				cvtdq2ps(xmm0, xmm0);
				mov(rax, reinterpret_cast<std::uintptr_t>(&Alchemy::CalculateDuration));
				call(rax);
				cvttss2si(edi, xmm0);
				jmp(ptr[rip]);
				dq(hook.address() + 0x7);
			}
		};

		auto patch = new Patch();
		patch->ready();

		auto& trampoline = SKSE::GetTrampoline();
		trampoline.write_branch<6>(hook.address(), patch->getCode());
	}

	void Alchemy::ModifyAlchemyItem(
		RE::TESDataHandler* a_dataHandler,
		RE::AlchemyItem* a_alchemyItem)
	{
		auto itemTraits = Data::ItemTraits::GetSingleton();
		itemTraits->ModifyAlchemyItem(a_alchemyItem);
		return _AddForm(a_dataHandler, a_alchemyItem);
	}

	float Alchemy::CalculateMagnitude(float a_magnitude)
	{
		const auto iniSettings = Settings::INISettings::GetSingleton();
		float roundThreshold = iniSettings->fMagnitudeThreshold;
		float roundMult = iniSettings->fMagnitudeMult;

		float result = a_magnitude;
		if (a_magnitude > roundThreshold) {
			result += roundMult * 0.5f;
			result -= std::remainderf(result, roundMult);
		}

		return result;
	}

	float Alchemy::CalculateDuration(float a_duration)
	{
		const auto iniSettings = Settings::INISettings::GetSingleton();
		float roundThreshold = iniSettings->fDurationThreshold;
		float roundMult = iniSettings->fDurationMult;

		float result = a_duration;
		if (a_duration > roundThreshold) {
			result += roundMult * 0.5f;
			result -= std::remainderf(result, roundMult);
		}

		return result;
	}
}
