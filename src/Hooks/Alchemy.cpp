#include "Alchemy.h"

#include "Data/ItemTraits.h"
#include "RE/Offset.h"

#define NOGDI
#include <xbyak/xbyak.h>

namespace Hooks
{
	void Alchemy::Install()
	{
		CreateItemPatch();
	}

	void Alchemy::CreateItemPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::BGSCreatedObjectManager::CreateAlchemyItem,
			0x174);

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				mov(r14, rax);

				mov(rcx, r14);
				mov(rax, reinterpret_cast<std::uintptr_t>(&Alchemy::ModifyAlchemyItem));
				call(rax);

				mov(edx, ptr[rax + 0x14]);
				jmp(ptr[rip]);
				dq(hook.address() + 0x6);
			}
		};

		Patch patch{};

		auto& trampoline = SKSE::GetTrampoline();
		trampoline.write_branch<6>(hook.address(), trampoline.allocate(patch));
	}

	RE::AlchemyItem* Alchemy::ModifyAlchemyItem(RE::AlchemyItem* a_alchemyItem)
	{
		Data::ItemTraits::GetSingleton()->ModifyAlchemyItem(a_alchemyItem);
		return a_alchemyItem;
	}
}
