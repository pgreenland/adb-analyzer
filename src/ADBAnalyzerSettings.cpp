#include "ADBAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

#pragma warning(disable : 4800) // warning C4800: 'U32' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable : 4996) // warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

ADBAnalyzerSettings::ADBAnalyzerSettings()
	: mInputChannel(UNDEFINED_CHANNEL)
{
	mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
	mInputChannelInterface->SetTitleAndTooltip("ADB", "Apple Desktop Bus");
	mInputChannelInterface->SetChannel(mInputChannel);

	AddInterface(mInputChannelInterface.get());

	AddExportOption(0, "Export as text/csv file");
	AddExportExtension(0, "text", "txt");
	AddExportExtension(0, "csv", "csv");

	ClearChannels();
	AddChannel(mInputChannel, "ADB", false);
}

ADBAnalyzerSettings::~ADBAnalyzerSettings()
{
}

bool ADBAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	ClearChannels();
	AddChannel(mInputChannel, "ADB", true);

	return true;
}
void ADBAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString(settings);

	const char* name_string; // the first thing in the archive is the name of the protocol analyzer that the data belongs to.
	text_archive >> &name_string;
	if(strcmp(name_string, "ADBAnalyzer") != 0)
		AnalyzerHelpers::Assert("ADBAnalyzer: Provided with a settings string that doesn't belong to us;");

	text_archive >> mInputChannel;

	ClearChannels();
	AddChannel(mInputChannel, "ADB", true);

	UpdateInterfacesFromSettings();
}

const char* ADBAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << "ADBAnalyzer";
	text_archive << mInputChannel;

	return SetReturnString(text_archive.GetString());
}

void ADBAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel(mInputChannel);
}
