#include "Data/ItemTraits.h"
#include "Hooks/Alchemy.h"

namespace
{
	void InitializeLog()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log"sv, Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
		const auto level = spdlog::level::trace;
#else
		const auto level = spdlog::level::info;
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version =
[]() {
	SKSE::PluginVersionData v{};

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("Parapets"sv);

	v.UsesAddressLibrary(true);
	v.HasNoStructUse(true);
	v.UsesStructsPost629(false);

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(99);

	Hooks::Alchemy::Install();

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener([](auto msg)
	{
		switch (msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
		{
			const auto itemTraits = Data::ItemTraits::GetSingleton();
			const auto dataHandler = RE::TESDataHandler::GetSingleton();
			if (dataHandler) {
				itemTraits->AddItemDefinitions(dataHandler->LookupForm(0xE3E9A, "Skyrim.esm"sv));
				itemTraits->AddItemDefinitions(dataHandler->LookupForm(0x74A37, "Skyrim.esm"sv));
				itemTraits->AddItemDefinitions(dataHandler->LookupForm(0x74A29, "Skyrim.esm"sv));
				itemTraits->AddItemDefinitions(dataHandler->LookupForm(0x74A23, "Skyrim.esm"sv));
				itemTraits->AddItemDefinitions(dataHandler->LookupForm(0x74A2D, "Skyrim.esm"sv));
				itemTraits->AddItemDefinitions(dataHandler->LookupForm(0x74A27, "Skyrim.esm"sv));
				itemTraits->AddItemDefinitions(dataHandler->LookupForm(0x74A21, "Skyrim.esm"sv));
			}
		} break;
		}
	});

	return true;
}
