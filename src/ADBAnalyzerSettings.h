#ifndef ADB_ANALYZER_SETTINGS
#define ADB_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class ADBAnalyzerSettings : public AnalyzerSettings
{
	public:
		ADBAnalyzerSettings();
		virtual ~ADBAnalyzerSettings();

		virtual bool SetSettingsFromInterfaces();			// Get the settings out of the interfaces, validate them, and save them to your local settings vars.
		virtual void LoadSettings(const char* settings);	// Load your settings from a string.
		virtual const char* SaveSettings();					// Save your settings to a string.

		void UpdateInterfacesFromSettings();

		Channel mInputChannel;

	protected:
		std::unique_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterface;
};

#endif // ADB_ANALYZER_SETTINGS_SETTINGS
