#pragma once

namespace Settings
{
	class INISettings final
	{
	public:
		static INISettings* GetSingleton();

		~INISettings() = default;
		INISettings(const INISettings&) = delete;
		INISettings(INISettings&&) = delete;
		INISettings& operator=(const INISettings&) = delete;
		INISettings& operator=(INISettings&&) = delete;

		void LoadSettings();

		bool bEnableBetterModels;
		bool bEnableBetterNames;
		bool bEnableMixtureNames;
		bool bEnableImpureCostFix;
		bool bEnableRoundedPotency;

		float fMagnitudeThreshold;
		float fMagnitudeMult;
		float fDurationThreshold;
		float fDurationMult;

	private:
		INISettings() = default;
	};
}
