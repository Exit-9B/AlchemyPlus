#include "Alchemy.h"

#include "Data/ItemTraits.h"
#include "RE/Offset.h"

#include <xbyak/xbyak.h>

#include <cmath>

namespace Hooks
{
	void Alchemy::Install()
	{
		CreateItemPatch();
		ModEffectivenessPatch();
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

	void Alchemy::ModEffectivenessPatch()
	{
		static const auto hook1 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::AlchemyMenu::ModEffectivenessFunctor::Invoke,
			0xA8);

		static const auto hook2 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::AlchemyMenu::ModEffectivenessFunctor::Invoke,
			0x120);

		struct Patch1 : Xbyak::CodeGenerator
		{
			Patch1()
			{
				mov(rax, reinterpret_cast<std::uintptr_t>(&Alchemy::CalculateMagnitude));
				call(rax);
				mov(rdx, ptr[rbx + 0x10]);
				movss(ptr[rsp + 0x38], xmm0);
				jmp(ptr[rip]);
				dq(hook1.address() + 0x6);
			}
		};

		Patch1 patch1{};

		struct Patch2 : Xbyak::CodeGenerator
		{
			Patch2()
			{
				cvtdq2ps(xmm0, xmm0);
				mov(rax, reinterpret_cast<std::uintptr_t>(&Alchemy::CalculateDuration));
				call(rax);
				cvttss2si(edi, xmm0);
				jmp(ptr[rip]);
				dq(hook2.address() + 0x7);
			}
		};

		Patch2 patch2{};

		auto& trampoline = SKSE::GetTrampoline();
		trampoline.write_branch<6>(hook1.address(), trampoline.allocate(patch1));
		trampoline.write_branch<6>(hook2.address(), trampoline.allocate(patch2));
	}

	void Alchemy::ModifyAlchemyItem(
		RE::TESDataHandler* a_dataHandler,
		RE::AlchemyItem* a_alchemyItem)
	{
		Data::ItemTraits::GetSingleton()->ModifyAlchemyItem(a_alchemyItem);
		return _AddForm(a_dataHandler, a_alchemyItem);
	}

	float Alchemy::CalculateMagnitude(float a_magnitude)
	{
		constexpr float roundThreshold = 25.0f;
		constexpr float roundMult = 5.0f;

		float result = a_magnitude;
		if (a_magnitude > roundThreshold) {
			result += roundMult * 0.5f;
			result -= std::remainderf(result, roundMult);
		}

		return result;
	}

	float Alchemy::CalculateDuration(float a_duration)
	{
		constexpr float roundThreshold = 15.0f;
		constexpr float roundMult = 5.0f;

		float result = a_duration;
		if (a_duration > roundThreshold) {
			result += roundMult * 0.5f;
			result -= std::remainderf(result, roundMult);
		}

		return result;
	}
}
