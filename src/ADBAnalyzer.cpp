#pragma warning(push, 0)
#include <sstream>
#include <ios>
#include <algorithm>
#pragma warning(pop)

#include "ADBAnalyzer.h"
#include "ADBAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

/* Calc percentage of value */
#define mPercent(val, pct) (((val) * (pct)) / 100)

/* Add/subtract percentage error to/from value */
#define mAddPercent(val, pct) (((val) * (100 + (pct))) / 100)
#define mSubPercent(val, pct) (((val) * (100 - (pct))) / 100)

/* Calculate sample count given rate and period */
#define mSampleCount(val, rate) (((U64)(val) * (rate)) / 1000000)

/* Calculate min/max sample count at given sample rate with given error */
#define mMaxSampleCount(val, err, rate) (mSampleCount(mAddPercent(val, err), rate))
#define mMinSampleCount(val, err, rate) (mSampleCount(mSubPercent(val, err), rate))

ADBAnalyzer::ADBAnalyzer() : mSettings(new ADBAnalyzerSettings()), Analyzer2(), mSimulationInitialised(false)
{
	SetAnalyzerSettings(mSettings.get());
	UseFrameV2();
}

ADBAnalyzer::~ADBAnalyzer()
{
	KillThread();
}

void ADBAnalyzer::SetupResults()
{
	mResults.reset(new ADBAnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());
	mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

void ADBAnalyzer::WorkerThread()
{
	/* Retrieve input channel and sample rate */
	mADB = GetAnalyzerChannelData(mSettings->mInputChannel);

	/* Calculate min and max attention period in samples */
	mAttentionMin = mMinSampleCount(mADBAttentionTime, mADBPctErrorHost, this->GetSampleRate());
	mAttentionMax = mMaxSampleCount(mADBAttentionTime, mADBPctErrorHost, this->GetSampleRate());

	/* Calculate min and max sync period in samples */
	mSyncMin = mMinSampleCount(mADBSyncTime, mADBPctErrorHost, this->GetSampleRate());
	mSyncMax = mMaxSampleCount(mADBSyncTime, mADBPctErrorHost, this->GetSampleRate());

	/* Calculate host minimum and maximum bit time */
	double mHostBittimeMin = mSubPercent(mADBBitCellTime, mADBPctErrorHost);
	double mHostBittimeMax = mAddPercent(mADBBitCellTime, mADBPctErrorHost);

	/* Host to device min and max zero low and high period */
	mHostZeroLowMin = mMinSampleCount(mPercent(mHostBittimeMin, mADBLowTimeBitCellPctZero - mADBLowTimePctError), 0, this->GetSampleRate());
	mHostZeroLowMax = mMaxSampleCount(mPercent(mHostBittimeMax, mADBLowTimeBitCellPctZero + mADBLowTimePctError), 0, this->GetSampleRate());
	mHostZeroHighMin = mMinSampleCount(mPercent(mHostBittimeMin, (100 - mADBLowTimeBitCellPctZero) - mADBLowTimePctError), 0, this->GetSampleRate());
	mHostZeroHighMax = mMaxSampleCount(mPercent(mHostBittimeMax, (100 - mADBLowTimeBitCellPctZero) + mADBLowTimePctError), 0, this->GetSampleRate());

	/* Host to device min and max zero low and high period */
	mHostOneLowMin = mMinSampleCount(mPercent(mHostBittimeMin, mADBLowTimeBitCellPctOne - mADBLowTimePctError), 0, this->GetSampleRate());
	mHostOneLowMax = mMaxSampleCount(mPercent(mHostBittimeMax, mADBLowTimeBitCellPctOne + mADBLowTimePctError), 0, this->GetSampleRate());
	mHostOneHighMin = mMinSampleCount(mPercent(mHostBittimeMin, (100 - mADBLowTimeBitCellPctOne) - mADBLowTimePctError), 0, this->GetSampleRate());
	mHostOneHighMax = mMaxSampleCount(mPercent(mHostBittimeMax, (100 - mADBLowTimeBitCellPctOne) + mADBLowTimePctError), 0, this->GetSampleRate());

	/* Calculate device minimum and maximum bit time */
	double mDeviceBittimeMin = mSubPercent(mADBBitCellTime, mADBPctErrorDevice);
	double mDeviceBittimeMax = mAddPercent(mADBBitCellTime, mADBPctErrorDevice);

	/* Device to host min and max zero low and high period */
	mDeviceZeroLowMin = mMinSampleCount(mPercent(mDeviceBittimeMin, mADBLowTimeBitCellPctZero - mADBLowTimePctError), 0, this->GetSampleRate());
	mDeviceZeroLowMax = mMaxSampleCount(mPercent(mDeviceBittimeMax, mADBLowTimeBitCellPctZero + mADBLowTimePctError), 0, this->GetSampleRate());
	mDeviceZeroHighMin = mMinSampleCount(mPercent(mDeviceBittimeMin, (100 - mADBLowTimeBitCellPctZero) - mADBLowTimePctError), 0, this->GetSampleRate());
	mDeviceZeroHighMax = mMaxSampleCount(mPercent(mDeviceBittimeMax, (100 - mADBLowTimeBitCellPctZero) + mADBLowTimePctError), 0, this->GetSampleRate());

	/* Device to host min and max zero low and high period */
	mDeviceOneLowMin = mMinSampleCount(mPercent(mDeviceBittimeMin, mADBLowTimeBitCellPctOne - mADBLowTimePctError), 0, this->GetSampleRate());
	mDeviceOneLowMax = mMaxSampleCount(mPercent(mDeviceBittimeMax, mADBLowTimeBitCellPctOne + mADBLowTimePctError), 0, this->GetSampleRate());
	mDeviceOneHighMin = mMinSampleCount(mPercent(mDeviceBittimeMin, (100 - mADBLowTimeBitCellPctOne) - mADBLowTimePctError), 0, this->GetSampleRate());
	mDeviceOneHighMax = mMaxSampleCount(mPercent(mDeviceBittimeMax, (100 - mADBLowTimeBitCellPctOne) + mADBLowTimePctError), 0, this->GetSampleRate());

	/* Host stop bit low time */
	mHostStopMin = mMinSampleCount(mADBStopTime, mADBPctErrorHost, this->GetSampleRate());
	// mHostStopMax = mMaxSampleCount(mADBStopTime, mADBPctErrorHost, this->GetSampleRate());

	/* Device stop bit low time */
	mDeviceStopMin = mMinSampleCount(mADBStopTime, mADBPctErrorDevice, this->GetSampleRate());
	// mDeviceStopMax = mMaxSampleCount(mADBStopTime, mADBPctErrorDevice, this->GetSampleRate());

	/* Global reset min time */
	mGlobalReset = mSampleCount(mADBGlobalResetTime, this->GetSampleRate());

	/* Device service request time (total time, including stop bit time) */
	mServiceRequestMin = mMinSampleCount(mADBServiceReqTime, mADBPctErrorDevice, this->GetSampleRate());
	mServiceRequestMax = mMaxSampleCount(mADBServiceReqTime, mADBPctErrorDevice, this->GetSampleRate());

	/* Stop bit to start bit time */
	mADBStopToStartMin = mSampleCount(mADBStopToStartTimeMin, this->GetSampleRate());
	mADBStopToStartMax = mSampleCount(mADBStopToStartTimeMax, this->GetSampleRate());

	/* Reset state */
	ADBState mState = Attention;

	/* Command and data values */
	U8 mCommand;
	bool bCmdIsListen;
	U8 mData[8];
	U8 mDataLen;

	for (;;)
	{
		/* Capture location of current edge and value */
		mADB->AdvanceToNextEdge();
		U64 curr_edge_location = mADB->GetSampleNumber();
		bool curr_edge_val = (BIT_HIGH == mADB->GetBitState());

		/* Retrieve next edge location (without advancing) */
		U64 next_edge_location = mADB->GetSampleOfNextEdge();

		/* Calculate samples between edges */
		U64 edge_period = next_edge_location - curr_edge_location;

		/* Range of edges covering command and data */
		U64 command_start_location;
		U64 command_end_location;

		/* Data start bit location */
		U64 data_start_location;

		/* Check for global reset (low for minimum period) */
		if (!curr_edge_val && (edge_period >= mGlobalReset))
		{
			/* Global reset asserted, reset state machine */
			mState = Attention;

			/* Add marker */
			mResults->AddMarker(curr_edge_location, AnalyzerResults::Square, mSettings->mInputChannel);
		}

		/* Assume state machine should reset */
		ADBState mNextState = Attention;

		/* Act on state */
		switch (mState)
		{
			case Attention:
			{
				if (	!curr_edge_val
					 && (edge_period >= mAttentionMin)
					 && (edge_period <= mAttentionMax)
				   )
				{
					/* Attention within spec, advance state */
					mNextState = Sync;

					/* Start new packet */
					mResults->CancelPacketAndStartNewPacket();

					/* Reset data length */
					mDataLen = 0;
				}
				break;
			}
			case Sync:
			{
				if (	curr_edge_val
					 && (edge_period >= mSyncMin)
					 && (edge_period <= mSyncMax)
				   )
				{
					/* Sync within spec, advance state */
					mNextState = CommandStop;

					/* Add marker */
					mResults->AddMarker(curr_edge_location, AnalyzerResults::Start, mSettings->mInputChannel);
				}
				break;
			}
			case CommandStop:
			{
				/* Capture current location as command start */
				command_start_location = curr_edge_location;

				/* Read command byte followed by stop */
				if (ReadByte(true, false, &mCommand))
				{
					/* Byte read, flag if the command just send was a listen */
					bCmdIsListen = (Listen == ((mCommand >> mADBCommandCodeShift) & mADBCommandCodeMask));

					/* Update current / next edges and calculate period */
					curr_edge_location = mADB->GetSampleNumber();
					curr_edge_val = (BIT_HIGH == mADB->GetBitState());
					next_edge_location = mADB->GetSampleOfNextEdge();
					edge_period = next_edge_location - curr_edge_location;

					if (	!curr_edge_val
						 && (edge_period >= mHostStopMin)
						 && (edge_period <= mServiceRequestMax)
					   )
					{
						/* Stop within spec, advance state */
						mNextState = StopToStart;

						/* Check for service request signal */
						if (	(edge_period >= mServiceRequestMin)
							 && (edge_period <= mServiceRequestMax)
						   )
						{
							/* Service request detected, flag edge */
							mResults->AddMarker(next_edge_location, AnalyzerResults::UpArrow, mSettings->mInputChannel);
						}
						else
						{
							/* No service request, flag stop */
							mResults->AddMarker(next_edge_location, AnalyzerResults::Stop, mSettings->mInputChannel);
						}

						/* Capture end location */
						command_end_location = curr_edge_location;
					}
				}
				break;
			}
			case StopToStart:
			{
				if (	(edge_period >= mADBStopToStartMin)
					 && (edge_period <= mADBStopToStartMax)
				   )
				{
					/* Stop to start time within spec, advance state */
					mNextState = DataStartLow;
				}
				else
				{
					/* Stop to start of spec, output command and reset */
					OutputBytesForTableAndExport(mCommand, mData, mDataLen, command_start_location, command_end_location);
				}
				break;
			}
			case DataStartLow:
			{
				if (	(bCmdIsListen && (edge_period >= mHostOneLowMin) && (edge_period <= mHostOneLowMax))
					 || (!bCmdIsListen && (edge_period >= mDeviceOneLowMin) && (edge_period <= mDeviceOneLowMax))
				   )
				{
					/* Start bit low period within spec, advance state */
					mNextState = DataStartHigh;

					/* Capture location for data start */
					data_start_location = curr_edge_location;
				}
				break;
			}
			case DataStartHigh:
			{
				if (	(bCmdIsListen && (edge_period >= mHostOneHighMin) && (edge_period <= mHostOneHighMax))
					 || (!bCmdIsListen && (edge_period >= mDeviceOneHighMin) && (edge_period <= mDeviceOneHighMax))
				   )
				{
					/* Start bit high period within spec, advance state */
					mNextState = DataStop;

					/* Add marker */
					mResults->AddMarker(data_start_location, AnalyzerResults::Start, mSettings->mInputChannel);
				}
				break;
			}
			case DataStop:
			{
				/* Read bytes */
				for (int i = 0; i < 8; i++)
				{
					/* Update current and next locations in order to handle stop bit */
					curr_edge_location = mADB->GetSampleNumber();
					curr_edge_val = (BIT_HIGH == mADB->GetBitState());
					next_edge_location = mADB->GetSampleOfNextEdge();

					/* Read byte send from host if command is listen, otherwise from device */
					if (ReadByte(bCmdIsListen, true, &mData[i]))
					{
						/* Byte read, count it */
						mDataLen++;
					}
					else
					{
						/* Failed to read data, break out of loop */
						break;
					}
				}

				if (mDataLen >= 2)
				{
					/* Minimum of two bytes has been transferred, check for stop bit */
					edge_period = next_edge_location - curr_edge_location;
					if (	!curr_edge_val
						 && (	(bCmdIsListen && (edge_period >= mHostStopMin) && (edge_period <= mServiceRequestMax))
							 || (!bCmdIsListen && (edge_period >= mDeviceStopMin) && (edge_period <= mServiceRequestMax))
						    )
					   )
					{
						/* Stop within spec, check for service request signal */
						if (	(edge_period >= mServiceRequestMin)
							 && (edge_period <= mServiceRequestMax)
						   )
						{
							/* Service request detected, flag edge */
							mResults->AddMarker(next_edge_location, AnalyzerResults::UpArrow, mSettings->mInputChannel);
						}
						else
						{
							/* No service request, flag stop */
							mResults->AddMarker(next_edge_location, AnalyzerResults::Stop, mSettings->mInputChannel);
						}

						/* Output command and data */
						OutputBytesForTableAndExport(mCommand, mData, mDataLen, command_start_location, curr_edge_location);
					}
				}
				break;
			}
			default:
			{
				/* Do nothing, allow reset */
				break;
			}
		}

		/* Apply calculated next state */
		mState = mNextState;

		/* Report how far we've got through processing samples */
		ReportProgress(mADB->GetSampleNumber());

		/* Check if this glorious game should come to an end? */
		CheckIfThreadShouldExit();
	}
}

bool ADBAnalyzer::ReadByte(bool bHostToDevice, bool bIsData, U8 *pbyOutput)
{
	bool bSuccess;
	U8 byData;

	/* Frame to display byte */
	Frame frame;

	/* Assume failure */
	bSuccess = false;

	/* Store start location */
	frame.mStartingSampleInclusive = mADB->GetSampleNumber();

	/* Iterate over bits */
	for (int i = 0; i < 8; i++)
	{
		/* Retrieve current edge location */
		U64 curr_edge_location = mADB->GetSampleNumber();

		/* Retrieve next edge location (without advancing) */
		U64 next_edge_location = mADB->GetSampleOfNextEdge();

		/* Calculate samples between edges */
		U64 edge_period = next_edge_location - curr_edge_location;

		U8 uiBit;

		if (	(bHostToDevice && (edge_period >= mHostOneLowMin) && (edge_period <= mHostOneLowMax))
			 || (!bHostToDevice && (edge_period >= mDeviceOneLowMin) && (edge_period <= mDeviceOneLowMax))
		   )
		{
			/* Edge period is correct for a one */
			uiBit = 1;
		}
		else if (	(bHostToDevice && (edge_period >= mHostZeroLowMin) && (edge_period <= mHostZeroLowMax))
				 || (!bHostToDevice && (edge_period >= mDeviceZeroLowMin) && (edge_period <= mDeviceZeroLowMax))
				)
		{
			/* Edge period is correct for a zero */
			uiBit = 0;
		}
		else
		{
			/* Invalid edge period */
			break;
		}

		/* Advance to next edge */
		mADB->AdvanceToNextEdge();

		/* Retrieve next next edge location (without advancing) */
		U64 next_next_edge_location = mADB->GetSampleOfNextEdge();

		/* Update edge period */
		edge_period = next_next_edge_location - next_edge_location;

		if (	uiBit
			 && (	(bHostToDevice && (edge_period >= mHostOneHighMin) && (edge_period <= mHostOneHighMax))
				 || (!bHostToDevice && (edge_period >= mDeviceOneHighMin) && (edge_period <= mDeviceOneHighMax))
				)
		   )
		{
			/* Edge period is correct for a one */
			bSuccess = true;
		}
		else if (	!uiBit
				 && (	(bHostToDevice && (edge_period >= mHostZeroHighMin) && (edge_period <= mHostZeroHighMax))
					 || (!bHostToDevice && (edge_period >= mDeviceZeroHighMin) && (edge_period <= mDeviceZeroHighMax))
					)
				)
		{
			/* Edge period is correct for a zero */
			bSuccess = true;
		}
		else
		{
			/* Invalid edge period */
			break;
		}

		/* Advance to next edge */
		mADB->AdvanceToNextEdge();

		/* Add bit */
		byData <<= 1;
		byData |= uiBit;
	}

	if (bSuccess)
	{
		/* Display byte */
		frame.mEndingSampleInclusive = mADB->GetSampleNumber();
		frame.mData1 = byData; /* data byte */
		frame.mData2 = mPacketID; /* index of packet it goes with */
		frame.mFlags = 0;
		if (bIsData) frame.mFlags |= DATA_BYTE_FLAG;
		mResults->AddFrame(frame);

		/* Output byte */
		*pbyOutput = byData;
	}

	return bSuccess;
}

void ADBAnalyzer::OutputBytesForTableAndExport(U8 byCommand, U8 *pabyData, U8 uiDataLen, U64 uiStart, U64 uiEnd)
{
	/* Decode command */
	U8 uiAddr = ((byCommand >> mADBCommandAddrShift) & mADBCommandAddrMask);
	ADBCommand eCode = ADBCommand((byCommand >> mADBCommandCodeShift) & mADBCommandCodeMask);
	U8 uiReg = ((byCommand >> mADBCommandRegShift) & mADBCommandRegMask);

	/* Output bytes for export / table display */
	FrameV2 frame_v2;
	frame_v2.AddByteArray("addr", &uiAddr, sizeof(uiAddr));
	frame_v2.AddString("cmd", CmdCodeRegToString(eCode, uiReg));
	frame_v2.AddByteArray("reg", &uiReg, sizeof(uiReg));
	frame_v2.AddByteArray("data", pabyData, uiDataLen);
	mResults->AddFrameV2(frame_v2, "adb", uiStart, uiEnd);
	mResults->CommitResults();

	/* Commit packet */
	mResults->CommitPacketAndStartNewPacket();

	/* Increment packet ID */
	mPacketID++;
}

const char* ADBAnalyzer::CmdCodeRegToString(U8 uiCmdCode, U8 uiReg)
{
	if (uiCmdCode == SendResetOrFlush && uiReg == 0) return "SendReset";
	if (uiCmdCode == SendResetOrFlush && uiReg == 1) return "Flush";
	if (uiCmdCode == Listen) return "Listen";
	if (uiCmdCode == Talk) return "Talk";
	return "Reserved";
}

U32 ADBAnalyzer::GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate,
										SimulationChannelDescriptor** simulation_channels)
{
	if (!mSimulationInitialised)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
		mSimulationInitialised = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(newest_sample_requested, sample_rate, simulation_channels);
}

U32 ADBAnalyzer::GetMinimumSampleRateHz()
{
	return mADBBitRate * 8;
}

bool ADBAnalyzer::NeedsRerun()
{
	return false;
}
const char gAnalyzerName[] = "ADB"; // your analyzer must have a unique name

const char* ADBAnalyzer::GetAnalyzerName() const
{
	return gAnalyzerName;
}

const char* GetAnalyzerName()
{
	return gAnalyzerName;
}

Analyzer* CreateAnalyzer()
{
	return new ADBAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}
