#pragma once
#include <JuceHeader.h>




/// <summary>
/// Representation of opened audio file with all informations and settings
/// </summary>
class AudioFile
{
public:

	AudioFile() {}
	AudioFile(juce::String path, double loopStart, double loopEnd) : absPath(path), loopStart(loopStart), loopEnd(loopEnd) {}

	juce::String absPath = "";
	juce::String relPathToLib = "";

	double loopStart=0;
	double loopEnd=0;
	double length=0;

	bool crossFadeActive = false;
	float crossFadeLength = 0;

	enum class CustomSetting {
		None			= 0,
		LoopStart		= 1 << 0,
		LoopEnd			= 1 << 1,
		CrossFadeActive	= 1 << 2,
		CrossFadeLength	= 1 << 3

	};

	inline friend CustomSetting operator& (CustomSetting a, CustomSetting b) {
		typedef std::underlying_type<CustomSetting>::type type;
		return static_cast<CustomSetting>(static_cast<type>(a) & static_cast<type>(b));
	}
	inline friend CustomSetting operator| (CustomSetting a, CustomSetting b) {
		typedef std::underlying_type<CustomSetting>::type type;
		return static_cast<CustomSetting>(static_cast<type>(a) | static_cast<type>(b));
	}
	inline friend CustomSetting operator~ (CustomSetting a) {
		typedef std::underlying_type<CustomSetting>::type type;
		return static_cast<CustomSetting>(~static_cast<type>(a));
	}


	/// <summary>
	/// defines whether a individual setting or default setting should be used for the given setting
	/// </summary>
	/// <param name="settingToSet">which setting to define</param>
	/// <param name="value">true for individual, false for default</param>
	void setCustomSetting(CustomSetting settingToSet, bool value) {
		if (value) {
			customSetting = customSetting | settingToSet;
		}
		else {
			customSetting = customSetting & ~settingToSet;
		}
	}

	/// <summary>
	/// returns if for this file theres a individual custom setting to be used or not.
	/// CustomSetting::None can be used to check if there is not a single custom setting.
	/// </summary>
	/// <returns>true if there is a custom setting or with CustomSetting::None as argument if there is none</returns>
	bool hasCustomSetting(CustomSetting settingToCheck) {

		if (settingToCheck == CustomSetting::None) {
			return customSetting == CustomSetting::None;
		}
		else {
			return (customSetting & settingToCheck) == settingToCheck;
		}
	}


	/// <summary>
	/// Converts the AudioFile to a juce::var which can be converted to JSON. Can be converted back by the fromVar() function.
	/// </summary>
	/// <returns>a juce::var containing propeties of the AudioFile as DynamicObject</returns>
	juce::var toVar() {
		juce::DynamicObject* obj = new juce::DynamicObject();
		copyPropetiesToDynObj(obj);
		return juce::var(obj);
	}

	/// <summary>
	/// Converts a juce::var to AudioFile. var should be a result of the toVar() function.
	/// </summary>
	/// <param name="var">the juce::var to be converted</param>
	/// <returns>a AudioFile</returns>
	static AudioFile fromVar(const juce::var& var) {
		return AudioFile::fromDynamicObject(*var.getDynamicObject());
	}

private:

	//used to define whether to use a individual "per-file-setting" or the global default-setting
	CustomSetting customSetting = CustomSetting::None;

	void copyPropetiesToDynObj(juce::DynamicObject* obj) {
		obj->setProperty("absPath", absPath);
		if (relPathToLib != "") {
			obj->setProperty("relPathToLib", relPathToLib);
		}
		obj->setProperty("length", length);

		obj->setProperty("customSetting", static_cast<int>(customSetting));

		if(hasCustomSetting(CustomSetting::LoopStart))
			obj->setProperty("loopStart", loopStart);

		if (hasCustomSetting(CustomSetting::LoopEnd))
			obj->setProperty("loopEnd", loopEnd);

		if (hasCustomSetting(CustomSetting::CrossFadeActive))
			obj->setProperty("crossFade", crossFadeActive);

		if (hasCustomSetting(CustomSetting::CrossFadeLength))
			obj->setProperty("crossFadeLength", crossFadeLength);

	}

	static AudioFile fromDynamicObject(const juce::DynamicObject& obj) {
		AudioFile audioFile;
		juce::var prop;

		prop = obj.getProperty("absPath");
		if (prop != juce::var())
			audioFile.absPath = prop;

		prop = obj.getProperty("relPathToLib");
		if (prop != juce::var())
			audioFile.relPathToLib = prop;

		prop = obj.getProperty("length");
		if (prop != juce::var())
			audioFile.length = prop;

		bool legacyBeforeCustomSettings = true;
		prop = obj.getProperty("customSetting");
		if (prop != juce::var()) {
			audioFile.customSetting = static_cast<CustomSetting>(static_cast<int>(prop));
			legacyBeforeCustomSettings = false;
		}


		prop = obj.getProperty("loopStart");
		if (prop != juce::var()){
			audioFile.loopStart = prop;
			if (legacyBeforeCustomSettings) {
				audioFile.setCustomSetting(CustomSetting::LoopStart, true);
			}
		}

		prop = obj.getProperty("loopEnd");
		if (prop != juce::var()){
			audioFile.loopEnd = prop;
			if (legacyBeforeCustomSettings) {
				audioFile.setCustomSetting(CustomSetting::LoopEnd, true);
			}
		}

		prop = obj.getProperty("crossFade");
		if (prop != juce::var()){
			audioFile.crossFadeActive = prop;
			if (legacyBeforeCustomSettings) {
				audioFile.setCustomSetting(CustomSetting::CrossFadeActive, true);
			}
		}

		prop = obj.getProperty("crossFadeLength");
		if (prop != juce::var()){
			audioFile.crossFadeLength = prop;
			if (legacyBeforeCustomSettings) {
				audioFile.setCustomSetting(CustomSetting::CrossFadeLength, true);
			}
		}

		return audioFile;
	}

};


