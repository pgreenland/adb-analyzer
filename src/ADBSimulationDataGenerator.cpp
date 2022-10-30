#include "ADBSimulationDataGenerator.h"
#include "ADBAnalyzerSettings.h"

/* Sample data frame */
const U8 ADBSimulationDataGenerator::mSimData0[] = {0x3c};
const U8 ADBSimulationDataGenerator::mSimData1[] = {0x3c, 0x82, 0x80};
const U8 ADBSimulationDataGenerator::mSimData2[] = {0x3c, 0x82, 0x81};

/* Sim data info */
const SimDataInfo ADBSimulationDataGenerator::mSimDataInfo[] =
{
	{mSimData0, sizeof(mSimData0), false},
	{mSimData1, sizeof(mSimData1), false},
	{mSimData2, sizeof(mSimData2), true}
};

ADBSimulationDataGenerator::ADBSimulationDataGenerator()
{
}

ADBSimulationDataGenerator::~ADBSimulationDataGenerator()
{
}

void ADBSimulationDataGenerator::Initialize(U32 simulation_sample_rate, ADBAnalyzerSettings* settings)
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	/* Set channel for simulation and sample rate */
	mADBSimData.SetChannel(mSettings->mInputChannel);
	mADBSimData.SetSampleRate(simulation_sample_rate);

	/* Reset sim index */
	mSimIndex = 0;

	/* Bus starts high */
	mADBSimData.SetInitialBitState(BIT_HIGH);

	/* Delay before first output */
	mADBSimData.Advance(UsToSamples(100));
}

U32 ADBSimulationDataGenerator::GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate,
													   SimulationChannelDescriptor** simulation_channels)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(newest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while (mADBSimData.GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		/* Attention byte and sync flag */
		SimWriteCycle(800, 65);

		/* Data */
		for (U32 i = 0; i < mSimDataInfo[mSimIndex].len; i++)
		{
			if (1 == i)
			{
				/* First data byte, add stop to start time and start bit */
				mADBSimData.Advance(UsToSamples(200));
				SimWriteCycle(35, 65);
			}

			/* Output byte */
			SimWriteByte(mSimDataInfo[mSimIndex].data[i]);

			if (0 == i)
			{
				/* Output stop after command */
				if (mSimDataInfo[mSimIndex].serviceReq)
				{
					/* Include service request */
					SimWriteCycle(300, 0);
				}
				else
				{
					/* Regular stop */
					SimWriteCycle(70, 0);
				}
			}
		}

		if (mSimDataInfo[mSimIndex].len > 1)
		{
			/* Output stop after last data byte */
			SimWriteCycle(70, 0);
		}

		/* Delay before next output */
		mADBSimData.Advance(UsToSamples(11 * 1000)); /* 11ms */

		/* Select next sequence */
		mSimIndex++;
		if (mSimIndex >= (sizeof(mSimDataInfo) / sizeof(mSimDataInfo[0])))
		{
			mSimIndex = 0;
		}
	}

	*simulation_channels = &mADBSimData;

	return 1;
}

void ADBSimulationDataGenerator::SimWriteByte(U8 value)
{
	for (U32 i = 0; i < 8; ++i)
	{
		U8 bit = (0 != ((value << i) & 0x80));
		if (bit)
		{
			/* Output one */
			SimWriteCycle(35, 65);
		}
		else
		{
			/* Output zero */
			SimWriteCycle(65, 35);
		}
	}
}

void ADBSimulationDataGenerator::SimWriteCycle(U32 lowPeriod, U32 highPeriod)
{
	mADBSimData.Transition();
	mADBSimData.Advance(UsToSamples(lowPeriod));
	mADBSimData.Transition();
	mADBSimData.Advance(UsToSamples(highPeriod));
}

U64 ADBSimulationDataGenerator::UsToSamples(double us)
{
	return U64((mSimulationSampleRateHz * us) / 1000000.0);
}
