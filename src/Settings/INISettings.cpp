#include "INISettings.h"

#include <SimpleIni.h>

namespace Settings
{
	INISettings* INISettings::GetSingleton()
	{
		static INISettings singleton{};
		return &singleton;
	}

	void INISettings::LoadSettings()
	{
		::CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)", Plugin::NAME).c_str());

		auto GetFloatValue = [&ini](const char* a_section, const char* a_key, float a_default)
		{
			return static_cast<float>(ini.GetDoubleValue(a_section, a_key, a_default));
		};

		bEnableBetterModels = ini.GetBoolValue("AlchemyPlus", "bEnableBetterModels", true);
		bEnableBetterNames = ini.GetBoolValue("AlchemyPlus", "bEnableBetterNames", true);
		bEnableMixtureNames = ini.GetBoolValue("AlchemyPlus", "bEnableMixtureNames", true);
		bEnableImpureCostFix = ini.GetBoolValue("AlchemyPlus", "bEnableImpureCostFix", true);
		bEnableRoundedPotency = ini.GetBoolValue("AlchemyPlus", "bEnableRoundedPotency", true);

		fMagnitudeThreshold = GetFloatValue("RoundedPotency", "fMagnitudeThreshold", 25.0f);
		fMagnitudeMult = GetFloatValue("RoundedPotency", "fMagnitudeMult", 5.0f);
		fDurationThreshold = GetFloatValue("RoundedPotency", "fDurationThreshold", 15.0f);
		fDurationMult = GetFloatValue("RoundedPotency", "fDurationMult", 5.0f);
	}
}
