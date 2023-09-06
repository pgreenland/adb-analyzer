#ifndef ADB_ANALYSER
#define ADB_ANALYSER

#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#define __cdecl
#define __stdcall
#define __fastcall
#endif

#include "Analyzer.h"
#include "ADBAnalyzerResults.h"
#include "ADBSimulationDataGenerator.h"

/* mType bit values */
#define DATA_BYTE_FLAG ( 1 << 0 )
#define SERVICE_REQUEST_FLAG ( 1 << 1 )

class ADBAnalyzerSettings;

enum ADBState
{
	/* Command attention pulse */
	Attention,

	/* Command sync pulse */
	Sync,

	/* Command byte and stop bit */
	CommandStop,

	/* Start to stop delay */
	StopToStart,

	/* Data start bit low period */
	DataStartLow,

	/* Data start bit high period */
	DataStartHigh,

	/* Data bytes and stop bit */
	DataStop
};

enum ADBCommand
{
	/* Reset devices to power on state (if reg = 0) or flush internal buffers / state (if reg = 1) */
	SendResetOrFlush = 0x00,

	/* Receive data from host (write) */
	Listen = 0x02,

	/* Send data to host (read) */
	Talk = 0x03
};

class ADBAnalyzer : public Analyzer2
{
	public:
		ADBAnalyzer();
		virtual ~ADBAnalyzer();
		virtual void SetupResults();
		virtual void WorkerThread();
		virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
		virtual U32 GetMinimumSampleRateHz();
		virtual bool NeedsRerun();
		virtual const char* GetAnalyzerName() const;

		/* Constant for command mask / shift */
		static const U8 mADBCommandAddrShift = 4;
		static const U8 mADBCommandCodeShift = 2;
		static const U8 mADBCommandRegShift = 0;
		static const U8 mADBCommandAddrMask = 0x0f;
		static const U8 mADBCommandCodeMask = 0x03;
		static const U8 mADBCommandRegMask = 0x03;

		/* Convert command code and register to string */
		static const char* CmdCodeRegToString(U8 uiCmdCode, U8 uiReg);

#pragma warning(push)
#pragma warning(disable : 4251)	// warning C4251: 'ADBAnalyzer::<...>' : class <...> needs to have dll-interface to be used by
								// clients of class
	protected:
		/* Microseconds per second */
		const U32 mUSPerSec = 1000000;

		/* ADB timing constants (from Guide to Macintosh Family Hardware) - bit cell time */
		const U32 mADBBitCellTime = 100; /* us */

		/* ADB bitrate */
		const U32 mADBBitRate = mUSPerSec / mADBBitCellTime;

		/* ADB allowable timing errors for host / device as a percentage of nominal timings */
		const U32 mADBPctErrorHost = 10; /* +/- percent */
		const U32 mADBPctErrorDevice = 30; /* +/- percent */

		/*
		** Bit cell low periods as a percentage of bit cell time for zero and one,
		** along with allowable errors, zero for example can be 60 -> 70 % of bit cell period
		*/
		const U32 mADBLowTimeBitCellPctZero = 65; /* percent */
		const U32 mADBLowTimeBitCellPctOne = 35; /* percent */
		const U32 mADBLowTimePctError = 5; /* +/- percent */

		/* Host attention pulse time */
		const U32 mADBAttentionTime = 750; /* us subject to host error */

		/* Host sync pulse time */
		const U32 mADBSyncTime = 65; /* us subject to host error */

		/* Host / device stop bit pulse time */
		const U32 mADBStopTime = 70; /* us subject to host/device error */

		/* Global reset pulse time */
		const U32 mADBGlobalResetTime = 3000; /* us minimum */

		/* Service request pulse time */
		const U32 mADBServiceReqTime = 300; /* us subject to device error */

		/* Stop bit to start bit time */
		const U32 mADBStopToStartTimeMin = 140; /* us */
		const U32 mADBStopToStartTimeMax = 260; /* us */

		/* Shared settings and results */
		std::unique_ptr<ADBAnalyzerSettings> mSettings;
		std::unique_ptr<ADBAnalyzerResults> mResults;

		/* Source channel */
		AnalyzerChannelData* mADB;

		/* Calculated sample counts */
		U64 mAttentionMin, mAttentionMax;
		U64 mSyncMin, mSyncMax;
		U64 mHostZeroLowMin, mHostZeroLowMax;
		U64 mHostZeroHighMin, mHostZeroHighMax;
		U64 mHostOneLowMin, mHostOneLowMax;
		U64 mHostOneHighMin, mHostOneHighMax;
		U64 mDeviceZeroLowMin, mDeviceZeroLowMax;
		U64 mDeviceZeroHighMin, mDeviceZeroHighMax;
		U64 mDeviceOneLowMin, mDeviceOneLowMax;
		U64 mDeviceOneHighMin, mDeviceOneHighMax;
		U64 mHostStopMin/*, mHostStopMax */;
		U64 mDeviceStopMin/*, mDeviceStopMax */;
		U64 mGlobalReset;
		U64 mServiceRequestMin, mServiceRequestMax;
		U64 mADBStopToStartMin, mADBStopToStartMax;

		/* Read byte from stream */
		bool ReadByte(bool bHostToDevice, U8 *pbyOutput);

		/* Output byte for display on waveform / export */
		void OutputByteForDisplayAndExport(bool bIsData, bool bServiceRequested, U8 byData, U64 uiStart, U64 uiEnd);

		/* Output bytes for display in table */
		void OutputBytesForTable(U8 byCommand, U8 *pabyData, U8 uiDataLen, bool bServiceRequested, U64 uiStart, U64 uiEnd);

		/* Packet id/index */
		U64 mPacketID;

		/* Simulation state */
		bool mSimulationInitialised;
		ADBSimulationDataGenerator mSimulationDataGenerator;
#pragma warning(pop)
};
extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);
#endif // ADB_ANALYSER
