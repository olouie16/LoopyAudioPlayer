#pragma once
#include <JuceHeader.h>

/// <summary>
/// Representation of opened audio file with all informations and settings  
/// </summary>
class AudioFile
{
public:
	juce::String absPath = "";
	juce::String relPathToLibRoot = "";
	juce::String relPathToExe = ""; //not in use right now
	double loopStart=0;
	double loopEnd=0;
	double length=0;

	AudioFile() {
	}
	AudioFile(juce::String path, double loopStart, double loopEnd) : absPath(path), loopStart(loopStart), loopEnd(loopEnd) {
	}


	void copyPropetiesToDynObj(juce::DynamicObject* obj) {
		obj->setProperty("loopStart", loopStart);
		obj->setProperty("loopEnd", loopEnd);
		obj->setProperty("absPath", absPath);
		if (relPathToLibRoot != "") {
			obj->setProperty("relPathToLibRoot", relPathToLibRoot);
		}
		if (relPathToExe != "") {
			obj->setProperty("relPathToExe", relPathToExe);
		}
		obj->setProperty("length", length);
	}

	static AudioFile fromDynamicObject(const juce::DynamicObject& obj) {
		AudioFile audioFile;
		juce::var prop;

		prop = obj.getProperty("absPath");
		if (prop != juce::var())
			audioFile.absPath = prop;

		prop = obj.getProperty("relPathToLibRoot");
		if (prop != juce::var())
			audioFile.relPathToLibRoot = prop;

		prop = obj.getProperty("relPathToExe");
		if (prop != juce::var())
			audioFile.relPathToExe = prop;

		prop = obj.getProperty("loopStart");
		if (prop != juce::var())
			audioFile.loopStart = prop;

		prop = obj.getProperty("loopEnd");
		if (prop != juce::var())
			audioFile.loopEnd = prop;

		prop = obj.getProperty("length");
		if (prop != juce::var())
			audioFile.length = prop;
		
		return audioFile;
	}

	/// <summary>
	/// Converts the AudioFile to a juce::var which can be converted to JSON. Can be converted back by the fromVar() function.
	/// </summary>
	/// <returns>a juce::var containing propeties of the AudioFile as DynamicObject</returns>
	juce::var toVar(juce::DynamicObject* obj) {
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
};

