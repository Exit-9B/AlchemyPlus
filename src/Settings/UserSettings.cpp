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

		const auto& config_mixtureNames = root["mixtureNames"];
		if (config_mixtureNames.isObject()) {
			mixtureNames.enabled = config_mixtureNames["enabled"].asBool();
		}

		const auto& config_impureCostFix = root["impureCostFix"];
		if (config_impureCostFix.isObject()) {
			impureCostFix.enabled = config_impureCostFix["enabled"].asBool();
		}

		const auto& config_copyExemplars = root["copyExemplars"];
		if (config_copyExemplars.isObject()) {
			copyExemplars.enabled = config_copyExemplars["enabled"].asBool();

			const auto& exemplars = config_copyExemplars["exemplars"];
			if (exemplars.isArray()) {
				for (const auto& exemplar : exemplars) {
					const auto identifier = exemplar.asString();
					const auto form = GetFormFromIdentifier(identifier);
					copyExemplars.exemplars.Add(form);
				}
			}
		}

		const auto& config_roundedPotency = root["roundedPotency"];
		if (config_roundedPotency.isObject()) {
			roundedPotency.enabled = config_roundedPotency["enabled"].asBool();
			roundedPotency.magnitudeThreshold =
				config_roundedPotency["magnitudeThreshold"].asFloat();
			roundedPotency.magnitudeMult = config_roundedPotency["magnitudeMult"].asFloat();
			roundedPotency.durationThreshold =
				config_roundedPotency["durationThreshold"].asFloat();
			roundedPotency.durationMult = config_roundedPotency["durationMult"].asFloat();

			const auto& overrides = config_roundedPotency["overrides"];
			for (const auto& identifier : overrides.getMemberNames()) {
				if (identifier.empty() || identifier[0] == '$') {
					continue;
				}

				const auto form = GetFormFromIdentifier(identifier);
				const auto baseEffect = form ? form->As<RE::EffectSetting>() : nullptr;
				if (!baseEffect) {
					continue;
				}

				const auto& setting = overrides[identifier];
				if (!setting.isObject()) {
					continue;
				}

				if (setting.isMember("magnitudeThreshold")) {
					roundedPotency.magnitudeOverrides[baseEffect].threshold =
						setting["magnitudeThreshold"].asFloat();
				}
				if (setting.isMember("magnitudeMult")) {
					roundedPotency.magnitudeOverrides[baseEffect].mult =
						setting["magnitudeMult"].asFloat();
				}
				if (setting.isMember("durationThreshold")) {
					roundedPotency.durationOverrides[baseEffect].threshold =
						setting["durationThreshold"].asFloat();
				}
				if (setting.isMember("durationMult")) {
					roundedPotency.durationOverrides[baseEffect].mult =
						setting["durationMult"].asFloat();
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

			auto effect = alchemyItem->effects[0];
			auto& baseEffect = effect->baseEffect;
			float potency = GetPotency(effect);

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

		if (auto it = effects_.find(baseEffect); it != effects_.end()) {
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
		if (auto it = a_overrides.find(a_baseEffect); it != a_overrides.end()) {
			auto setting = it->second;
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
