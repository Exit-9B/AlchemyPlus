#include "Alchemy.h"

#include "Data/ItemTraits.h"
#include "RE/Offset.h"

namespace Hooks
{
	void Alchemy::Install()
	{
		CreateItemPatch();
	}

	void Alchemy::CreateItemPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::AlchemyItem::CreateFromEffects,
			0x16F);

		if (!REL::make_pattern<"E8">().match(hook.address())) {
			util::report_and_fail("Alchemy::CreateItemPatch failed to install"sv);
		}

		auto& trampoline = SKSE::GetTrampoline();
		_AddForm = trampoline.write_call<5>(hook.address(), &Alchemy::ModifyAlchemyItem);
	}

	void Alchemy::ModifyAlchemyItem(
		RE::TESDataHandler* a_dataHandler,
		RE::AlchemyItem* a_alchemyItem)
	{
		Data::ItemTraits::GetSingleton()->ModifyAlchemyItem(a_alchemyItem);
		return _AddForm(a_dataHandler, a_alchemyItem);
	}
}
