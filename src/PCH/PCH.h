#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif

using namespace std::literals;

namespace logger = SKSE::log;

namespace util
{
	using SKSE::stl::report_and_fail;
	using SKSE::stl::utf8_to_utf16;
	using SKSE::stl::utf16_to_utf8;

	template <typename Map>
	inline bool find_closest_value(
		const Map& a_map,
		typename Map::key_type a_key,
		typename Map::mapped_type& a_result)
	{
		if (a_map.empty()) {
			return false;
		}

		auto low = a_map.lower_bound(a_key);

		if (low == a_map.end()) {
			a_result = a_map.rbegin()->second;
			return true;
		}
		else if (low == a_map.begin()) {
			a_result = low->second;
			return true;
		}
		else {
			auto prev = std::prev(low);
			if ((a_key - prev->first) < (low->first - a_key)) {
				a_result = prev->second;
				return true;
			}
			else {
				a_result = low->second;
				return true;
			}
		}
	}

	consteval auto MakeOffset(
		[[maybe_unused]] std::uint64_t a_idSE,
		[[maybe_unused]] std::uint64_t a_addrVR)
	{
#ifndef SKYRIMVR
		return REL::ID(a_idSE);
#else
		return REL::Offset(a_addrVR);
#endif
	}

	inline auto MakeHook(REL::ID a_id, std::ptrdiff_t a_offset = 0)
	{
		return REL::Relocation<std::uintptr_t>(a_id, a_offset);
	}

	inline auto MakeHook(REL::Offset a_address, std::ptrdiff_t a_offset = 0)
	{
		return REL::Relocation<std::uintptr_t>(a_address.address() + a_offset);
	}
}

#ifndef SKYRIMVR
#define IF_SKYRIMSE(a_resultSE, a_resultVR) a_resultSE
#else
#define IF_SKYRIMSE(a_resultSE, a_resultVR) a_resultVR
#endif

#define DLLEXPORT __declspec(dllexport)

#include "Plugin.h"
