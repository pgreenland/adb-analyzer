#include "ADBAnalyzerResults.h"

#include <AnalyzerHelpers.h>
#include "ADBAnalyzer.h"
#include "ADBAnalyzerSettings.h"
#include <iostream>
#include <sstream>

ADBAnalyzerResults::ADBAnalyzerResults(ADBAnalyzer* analyzer, ADBAnalyzerSettings* settings)
	: AnalyzerResults(), mSettings(settings), mAnalyzer(analyzer)
{
}

ADBAnalyzerResults::~ADBAnalyzerResults()
{
}

void ADBAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& /*channel*/, DisplayBase display_base)
{
	Frame frame = GetFrame(frame_index);
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
	AddResultString(number_str);
}

void ADBAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 /*export_type_user_id*/)
{
	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile(file);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();
	U64 num_frames = GetNumFrames();
	U64 last_packet_id = UINT64_MAX;

	ss << "Time [s],Addr,Cmd,Reg,Data0,Data1,Data2,Data3,Data4,Data5,Data6,Data7,SvcReq" << std::endl;

	/* Reset data count, such that we always output 8 bytes */
	U8 data_count = 0;

	/* Assume neither command nor data contains a service request */
	bool service_request = false;

	for (U32 i = 0; i < num_frames; i++)
	{
		Frame frame = GetFrame(i);

		if (last_packet_id != frame.mData2)
		{
			/* Start of new packet, output empty columns, final service request status and end line */
			if (i > 0)
			{
				for (int j = data_count; j < 8; j++) ss << ",";
				ss << "," << service_request << std::endl;
			}

			/* Output time string */
			char time_str[ 128 ];
			AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);
			ss << time_str;

			/* Decode command */
			U8 uiAddr = ((frame.mData1 >> ADBAnalyzer::mADBCommandAddrShift) & ADBAnalyzer::mADBCommandAddrMask);
			ADBCommand eCode = ADBCommand((frame.mData1 >> ADBAnalyzer::mADBCommandCodeShift) & ADBAnalyzer::mADBCommandCodeMask);
			U8 uiReg = ((frame.mData1 >> ADBAnalyzer::mADBCommandRegShift) & ADBAnalyzer::mADBCommandRegMask);

			/* Output command byte fields */
			char number_str[ 128 ];
			AnalyzerHelpers::GetNumberString(uiAddr, display_base, 8, number_str, 128);
			ss << "," << number_str;
			ss << "," << ADBAnalyzer::CmdCodeRegToString(eCode, uiReg);
			AnalyzerHelpers::GetNumberString(uiReg, display_base, 8, number_str, 128);
			ss << "," << number_str;

			/* Reset data count */
			data_count = 0;

			/* Reset service request flag */
			service_request = false;

			/* Update last ID */
			last_packet_id = frame.mData2;
		}

		if (frame.mFlags & DATA_BYTE_FLAG)
		{
			/* Output data byte */
			char number_str[ 128 ];
			AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
			ss << "," << number_str;

			/* Count byte */
			data_count++;
		}

		/* Or in service request status */
		service_request |= (0 != (frame.mFlags & SERVICE_REQUEST_FLAG));

		/* Ensure final empty columns, service request status and end line is output on last frame */
		if (i == (num_frames - 1))
		{
			for (int j = data_count; j < 8; j++) ss << ",";
			ss << "," << service_request << std::endl;
		}

		/* Output to file and reset buffer */
		AnalyzerHelpers::AppendToFile((U8*)ss.str().c_str(), ss.str().length(), f);
		ss.str(std::string());

		/* Stop early if cancelled */
		if(UpdateExportProgressAndCheckForCancel(i, num_frames) == true)
		{
			AnalyzerHelpers::EndFile(f);
			return;
		}
	}

	/* Final check */
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
	AnalyzerHelpers::EndFile(f);
}

void ADBAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
	ClearTabularText();

	Frame frame = GetFrame(frame_index);

	char number_str[ 128 ];
	AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
	AddTabularText(number_str);
}

void ADBAnalyzerResults::GeneratePacketTabularText(U64 /*packet_id*/, DisplayBase /*display_base*/)
{
}

void ADBAnalyzerResults::GenerateTransactionTabularText(U64 /*transaction_id*/, DisplayBase /*display_base*/)
{
}
