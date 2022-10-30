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

	ss << "Time [s],Command,HostToDevice,Data" << std::endl;

	for (U32 i = 0; i < num_frames; i++)
	{
		Frame frame = GetFrame(i);

		if (last_packet_id != frame.mData2)
		{
			/* Start of new packet, end line and output timestamp */
			if (i > 0) ss << std::endl;

			/* Output time string */
			char time_str[ 128 ];
			AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);
			ss << time_str;

			/* Output command byte */
			char number_str[ 128 ];
			AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
			ss << "," << number_str;

			/* Output data direction, by reading command bits from command byte */
			if (Listen == (frame.mData1 & ADBAnalyzer::mADBCommandCodeMask) >> ADBAnalyzer::mADBCommandCodeShift)
			{
				/* Command is listen, meaning host intends to send data to device */
				ss << ",true";
			}
			else
			{
				/* Command is not listen, meaning there is no data, or device should report back to host */
				ss << ",false";
			}

			/* Update last ID */
			last_packet_id = frame.mData2;
		}

		if (frame.mFlags & DATA_BYTE_FLAG)
		{
			/* Output data byte */
			char number_str[ 128 ];
			AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
			ss << "," << number_str;
		}

		/* Ensure final new line is output on last frame */
		if (i == (num_frames - 1)) ss << std::endl;

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
