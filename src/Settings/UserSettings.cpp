#include "UserSettings.h"

#include <json/json.h>

namespace Settings
{
	float RoundingSetting::Apply(float a_value) const
	{
		float result = a_value;
		if (a_value > threshold) {
			result += mult * 0.5f;
			result -= std::remainderf(result, mult);
		}
		return result;
	}

	UserSettings* UserSettings::GetSingleton()
	{
		static UserSettings singleton{};
		return &singleton;
	}

	static RE::BSResourceNiBinaryStream& operator>>(
		RE::BSResourceNiBinaryStream& a_sin,
		Json::Value& a_root)
	{
		Json::CharReaderBuilder fact;
		std::unique_ptr<Json::CharReader> const reader{ fact.newCharReader() };

		const auto size = a_sin.stream->totalSize;
		const auto buffer = std::make_unique<char[]>(size);
		a_sin.read(buffer.get(), size);

		const auto begin = buffer.get();
		const auto end = begin + size;

		std::string errs;
		const bool ok = reader->parse(begin, end, &a_root, &errs);

		if (!ok) {
			throw std::runtime_error(errs);
		}

		return a_sin;
	}

	static RE::TESForm* GetFormFromIdentifier(const std::string& a_identifier)
	{
		std::istringstream ss{ a_identifier };
		std::string plugin, id;

		std::getline(ss, plugin, '|');
		std::getline(ss, id);
		RE::FormID rawFormID;
		std::istringstream(id) >> std::hex >> rawFormID;

		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		return dataHandler->LookupForm(rawFormID, plugin);
	}

	static bool ReadSetting(UserSettings::Setting& a_setting, const Json::Value& a_value)
	{
		if (!a_value.isObject()) {
			return false;
		}

		return a_setting.enabled = a_value["enabled"].asBool();
	}

	void UserSettings::LoadSettings()
	{
		RE::BSResourceNiBinaryStream fileStream{
			fmt::format("SKSE/Plugins/{}.json", Plugin::NAME)
		};

		if (!fileStream.good()) {
			return;
		}

		Json::Value root;
		try {
			fileStream >> root;
		}
		catch (...) {
			logger::error("Parse errors in config");
		}

		if (!root.isObject()) {
			return;
		}

		ReadSetting(knownFailureFix, root["knownFailureFix"]);
		ReadSetting(mixtureNames, root["mixtureNames"]);
		ReadSetting(impureCostFix, root["impureCostFix"]);

		if (auto config = root["copyExemplars"]; ReadSetting(copyExemplars, config)) {
			const auto& exemplars = config["exemplars"];
			if (exemplars.isArray()) {
				for (const auto& exemplar : exemplars) {
					const auto identifier = exemplar.asString();
					const auto form = GetFormFromIdentifier(identifier);
					copyExemplars.exemplars.Add(form);
				}
			}
		}

		if (auto config = root["roundedPotency"]; ReadSetting(roundedPotency, config)) {
			roundedPotency.magnitudeThreshold = config["magnitudeThreshold"].asFloat();
			roundedPotency.magnitudeMult = config["magnitudeMult"].asFloat();
			roundedPotency.durationThreshold = config["durationThreshold"].asFloat();
			roundedPotency.durationMult = config["durationMult"].asFloat();

			const auto& overrides = config["overrides"];
			for (const auto& identifier : overrides.getMemberNames()) {
				if (identifier.empty() || identifier[0] == '$') {
					continue;
				}

				const auto form = GetFormFromIdentifier(identifier);
				const auto baseEffect = form ? form->As<RE::EffectSetting>() : nullptr;
				if (!baseEffect) {
					continue;
				}

				const auto& effectOverride = overrides[identifier];
				if (!effectOverride.isObject()) {
					continue;
				}

				if (effectOverride.isMember("magnitudeThreshold")) {
					roundedPotency.magnitudeOverrides[baseEffect].threshold =
						effectOverride["magnitudeThreshold"].asFloat();
				}
				if (effectOverride.isMember("magnitudeMult")) {
					roundedPotency.magnitudeOverrides[baseEffect].mult =
						effectOverride["magnitudeMult"].asFloat();
				}
				if (effectOverride.isMember("durationThreshold")) {
					roundedPotency.durationOverrides[baseEffect].threshold =
						effectOverride["durationThreshold"].asFloat();
				}
				if (effectOverride.isMember("durationMult")) {
					roundedPotency.durationOverrides[baseEffect].mult =
						effectOverride["durationMult"].asFloat();
				}
			}
		}
	}

	static float GetPotency(RE::Effect* a_effect)
	{
		using EffectFlag = RE::EffectSetting::EffectSettingData::Flag;

		const auto& baseEffect = a_effect->baseEffect;
		const auto flags = baseEffect->data.flags;
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

	void UserSettings::ExemplarList::Add(RE::TESForm* a_form)
	{
		if (!a_form)
			return;

		if (const auto levItem = a_form->As<RE::TESLevItem>()) {
			for (const auto& entry : levItem->entries) {
				Add(entry.form);
			}
		}
		else if (const auto formList = a_form->As<RE::BGSListForm>()) {
			for (const auto& form : formList->forms) {
				Add(form);
			}
		}
		else if (const auto alchemyItem = a_form->As<RE::AlchemyItem>()) {
			if (alchemyItem->effects.size() != 1)
				return;

			const auto effect = alchemyItem->effects[0];
			const auto& baseEffect = effect->baseEffect;
			const float potency = GetPotency(effect);

			auto& defs = effects_[baseEffect];
			defs.try_emplace(potency, ItemDefinition{ alchemyItem->fullName, alchemyItem->model });
		}
	}

	auto UserSettings::ExemplarList::FindClosest(RE::AlchemyItem* a_alchemyItem) const
		-> std::optional<ItemDefinition>
	{
		auto costliestEffect = a_alchemyItem->GetCostliestEffectItem();
		auto baseEffect = costliestEffect->baseEffect;
		float potency = GetPotency(costliestEffect);

		if (const auto it = effects_.find(baseEffect); it != effects_.end()) {
			ItemDefinition def;
			if (util::find_closest_value(it->second, potency, def)) {
				return ItemDefinition{ def.name, def.model };
			}
		}

		return std::nullopt;
	}

	static void ApplyOverride(
		RoundingSetting& a_rounding,
		const UserSettings::OverrideMap& a_overrides,
		const RE::EffectSetting* a_baseEffect)
	{
		if (const auto it = a_overrides.find(a_baseEffect); it != a_overrides.end()) {
			const auto setting = it->second;
			if (setting.threshold.has_value()) {
				a_rounding.threshold = setting.threshold.value();
			}

			if (setting.mult.has_value()) {
				a_rounding.mult = setting.mult.value();
			}
		}
	}

	RoundingSetting UserSettings::RoundedPotency::GetMagnitudeSetting(
		const RE::EffectSetting* a_baseEffect) const
	{
		RoundingSetting rounding{ magnitudeThreshold, magnitudeMult };
		ApplyOverride(rounding, magnitudeOverrides, a_baseEffect);
		return rounding;
	}

	RoundingSetting UserSettings::RoundedPotency::GetDurationSetting(
		const RE::EffectSetting* a_baseEffect) const
	{
		RoundingSetting rounding{ durationThreshold, durationMult };
		ApplyOverride(rounding, durationOverrides, a_baseEffect);
		return rounding;
	}
}
