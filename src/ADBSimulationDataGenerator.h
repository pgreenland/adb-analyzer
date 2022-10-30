#ifndef ADB_SIMULATION_DATA_GENERATOR
#define ADB_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

class ADBAnalyzerSettings;

struct SimDataInfo
{
	const U8 *data;
	const U8 len;
	const bool serviceReq;
};

class ADBSimulationDataGenerator
{
	public:
		ADBSimulationDataGenerator();
		~ADBSimulationDataGenerator();

		void Initialize(U32 simulation_sample_rate, ADBAnalyzerSettings* settings);
		U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);

	protected:
		/* Write a byte to the output */
		void SimWriteByte(U8 value);

		/* Output a cycle of the waveform */
		void SimWriteCycle(U32 lowPeriod, U32 highPeriod);

		/* Convert microseconds to sample count */
		U64 UsToSamples(double us);

		/* Shared settings and simulation sample rate */
		ADBAnalyzerSettings* mSettings;
		U32 mSimulationSampleRateHz;

		/* Demo frames to send */
		static const U8 mSimData0[];
		static const U8 mSimData1[];
		static const U8 mSimData2[];

		/* Frame info */
		static const SimDataInfo mSimDataInfo[];

		/* Current simulation data index */
		U8 mSimIndex;

		/* Channel description */
		SimulationChannelDescriptor mADBSimData;
};

#endif // ADB_SIMULATION_DATA_GENERATOR
