#pragma once

namespace Settings
{
	struct RoundingSetting
	{
		float threshold;
		float mult;

		float Apply(float a_value) const;
	};

	class UserSettings final
	{
	public:
		UserSettings(const UserSettings&) = delete;
		UserSettings(UserSettings&&) = delete;

		~UserSettings() = default;

		UserSettings& operator=(const UserSettings&) = delete;
		UserSettings& operator=(UserSettings&&) = delete;

	private:
		UserSettings() = default;

	public:

		static UserSettings* GetSingleton();

		void LoadSettings();

	public:
		struct Setting
		{
			bool enabled = false;
		};

		class ExemplarList final
		{
		public:
			struct ItemDefinition
			{
				RE::BSFixedString name;
				RE::BSFixedString model;
			};

			using SortedDefs = std::map<float, ItemDefinition>;
			using EffectTable = std::map<const RE::EffectSetting*, SortedDefs>;

			void Add(RE::TESForm* a_form);

			auto FindClosest(RE::AlchemyItem* a_alchemyItem) const
				-> std::optional<ItemDefinition>;

			EffectTable effects_;
		};

		struct RoundingOverride
		{
			std::optional<float> threshold;
			std::optional<float> mult;
		};
		using OverrideMap = std::map<const RE::EffectSetting*, RoundingOverride>;

	public:
		Setting mixtureNames;

		Setting impureCostFix;

		struct CopyExemplars : Setting
		{
			ExemplarList exemplars;
		} copyExemplars;

		struct RoundedPotency : Setting
		{
			float magnitudeThreshold = 9999.0f;
			float magnitudeMult = 0.01f;
			float durationThreshold = 86313600.0f;
			float durationMult = 1.0f;
			OverrideMap magnitudeOverrides;
			OverrideMap durationOverrides;

			RoundingSetting GetMagnitudeSetting(const RE::EffectSetting* a_baseEffect) const;
			RoundingSetting GetDurationSetting(const RE::EffectSetting* a_baseEffect) const;
		} roundedPotency;
	};
}
