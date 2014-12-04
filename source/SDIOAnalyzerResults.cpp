// The MIT License (MIT)
//
// Copyright (c) 2013 Erick Fuentes http://erickfuent.es
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "SDIOAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SDIOAnalyzer.h"
#include "SDIOAnalyzerSettings.h"
#include <iostream>
#include <fstream>


char const* interpretCardState(unsigned int status)
{
	switch((status >> 9) & 0xF)
	{
	case 0:		return("idle");
	case 1:		return("ready");
	case 2:		return("ident");
	case 3:		return("stby");
	case 4:		return("tran");
	case 5:		return("data");
	case 6:		return("rcv");
	case 7:		return("prg");
	case 8:		return("dis");
	case 15:	return("sdio");
	default:	return("???");
	}
}

size_t	interpretCardStatusShort(char* statusString, size_t length, unsigned int status, int sizeClass)
{
	int numErrors = 0;
	unsigned int errs = status & 0xFFF9E008;

	// count 1 bits
	while(errs > 0)
	{
		errs &= (errs - 1);
		numErrors++;
	}

	if(sizeClass == 1)
	{
		return(	snprintf(statusString, length, "%i err%s [%s]",
						numErrors,
						(numErrors == 1)? "" : "s",
						interpretCardState(status)
					)
			);
	}
	else
	{
		if(numErrors == 0)
		{
			return(snprintf(statusString, length, "ok"));
		}
		else
		{
			return(	snprintf(statusString, length, "%i err%s",
							numErrors,
							(numErrors == 1)? "s" : ""
						)
				);
		}
	}
}

size_t	interpretCardStatus(char* statusString, size_t length, unsigned int status)
{
	char const* state = interpretCardState(status);

	return(	snprintf(statusString, length, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s [%s] %s%s%s",
				((status >> 31) & 1)? "RNGE " : "",
				((status >> 30) & 1)? "ADDR " : "",
				((status >> 29) & 1)? "BLEN " : "",
				((status >> 28) & 1)? "ERSQ " : "",
				((status >> 27) & 1)? "ERPM " : "",
				((status >> 26) & 1)? "WPVI " : "",
				((status >> 25) & 1)? "LOCK " : "",
				((status >> 24) & 1)? "LKFL " : "",
				((status >> 23) & 1)? "!CRC " : "",
				((status >> 22) & 1)? "!CMD " : "",
				((status >> 21) & 1)? "!ECC " : "",
				((status >> 20) & 1)? "CCER " : "",
				((status >> 19) & 1)? "ERR! " : "",
				
				((status >> 16) & 1)? "!CSD " : "",
				((status >> 15) & 1)? "WPSK " : "",
				((status >> 14) & 1)? "ECCD " : "",
				((status >> 13) & 1)? "ERST " : "",
				
				state,

				((status >> 8) & 1)? "rdy" : "bsy",
				
				((status >> 5) & 1)? "acmd " : "",
				((status >> 4) & 1)? "sdio " : "",
				((status >> 3) & 1)? "!AKE" : ""
			)
		);
}

		SDIOAnalyzerResults::SDIOAnalyzerResults(SDIOAnalyzer* analyzer, SDIOAnalyzerSettings* settings):
			AnalyzerResults(),
			mSettings(settings),
			mAnalyzer(analyzer)
{
}

		SDIOAnalyzerResults::~SDIOAnalyzerResults()
{
}

void	SDIOAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame frame = GetFrame(frame_index);

	char number_str1[128];
	char number_str2[128];

	switch(frame.mType)
	{
	case SDIOAnalyzer::FRAME_DIR:
		if(frame.mData1)
		{
			AddResultString("H");
			AddResultString("Host");
			AddResultString("DIR: Host");
		}
		else
		{
			AddResultString("S");
			AddResultString("Slave");
			AddResultString("DIR: Slave");
		}
		break;

	case SDIOAnalyzer::FRAME_CMD:
		AnalyzerHelpers::GetNumberString(frame.mData1 & 0x3F, Decimal, 6, number_str1, 128);
		AddResultString((frame.mData1 & 0x40)? "C" : "R", number_str1);
		AddResultString((frame.mData1 & 0x40)? "CMD" : "RSP", number_str1);
		break;

	case SDIOAnalyzer::FRAME_ARG:
		AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 32, number_str1, 128);
		
		if(frame.mFlags & 0x40)	// command
		{
			switch(frame.mFlags & 0x3F)
			{
			default:
				AddResultString("ARG ", number_str1);
				break;

			case 0:	// GO_IDLE
				{
					AddResultString("Idle");
					AddResultString("Go idle");
				}
				break;

			case 3:	// SEND_RELATIVE_ADDR
				{
					AddResultString("Addr");
					AddResultString("Get address");
				}
				break;

			case 5:	// GET_OCR
				{
					AddResultString("OCR?");
					AddResultString("Get OCR");
				}
				break;

			case 7:	// SELECT_CARD
				{
					char buffer[14];
					snprintf(buffer, sizeof(buffer), "%i", (int)((frame.mData1 >> 16) & 0xFFFF));
					AddResultString("S", buffer);
					AddResultString("Sel ", buffer);
					AddResultString("Select ", buffer);
				}
				break;

			case 52:
				{
					char buffer[24];
					snprintf(buffer, sizeof(buffer), "%c%i %s 0x%05X, %i",
							((frame.mData1 >> 31) & 1)? 'W' : 'R',
							(int)((frame.mData1 >> 28) & 7),
							((frame.mData1 >> 27) & 1)? "RAW " : "",
							(int)((frame.mData1 >> 9) & 0x1FFFF),
							(int)(frame.mData1 & 0x1FF)
						);
					AddResultString(((frame.mData1 >> 31) & 1)? "W" : "R");
					AddResultString(buffer);
				}
				break;

			case 53:
				{
					char buffer[24];
					snprintf(buffer, sizeof(buffer), "%c%i %s%s 0x%05X, %i",
							((frame.mData1 >> 31) & 1)? 'W' : 'R',
							(int)((frame.mData1 >> 28) & 7),
							((frame.mData1 >> 27) & 1)? "B" : "",
							((frame.mData1 >> 26) & 1)? "++" : "",
							(int)((frame.mData1 >> 9) & 0x1FFFF),
							(int)(frame.mData1 & 0x1FF)
						);
					AddResultString(((frame.mData1 >> 31) & 1)? "W" : "R");
					AddResultString(buffer);
				}
				break;
			}
		}
		else	// response
		{
			switch(frame.mFlags & 0x3F)
			{
			default:
				AddResultString("ARG ", number_str1);
				break;

			case 3:	// SEND_RELATIVE_ADDR
				{
					char buffer[14];
					snprintf(buffer, sizeof(buffer), "%i", (int)((frame.mData1 >> 16) & 0xFFFF));
					AddResultString("A=", buffer);
					AddResultString("Addr=", buffer);
					AddResultString("Address = ", buffer);
				}
				break;

			case 7:	// SELECT_CARD
				{
					// R1b
					char buffer[100];
					interpretCardStatusShort(buffer, sizeof(buffer), frame.mData1, 0);
					AddResultString(buffer);
					interpretCardStatusShort(buffer, sizeof(buffer), frame.mData1, 1);
					AddResultString(buffer);
					interpretCardStatus(buffer, sizeof(buffer), frame.mData1);
					AddResultString(buffer);
				}
				break;

			case 52:
			case 53:
				{
					char buffer[14];
					char const* ioState;
					switch((frame.mData1 >> 12) & 3)
					{
					case 0:	ioState = "dis";	break;
					case 1:	ioState = "cmd";	break;
					case 2:	ioState = "trn";	break;
					case 3:	ioState = "???";	break;
					}

					snprintf(buffer, sizeof(buffer), "%s%s[%s] %s%s%s= 0x%02X",
							((frame.mData1 >> 15) & 1)? "!CRC " : "",
							((frame.mData1 >> 14) & 1)? "!CMD " : "",
							ioState,
							((frame.mData1 >> 11) & 1)? "ERR " : "",
							((frame.mData1 >> 9) & 1)? "INV " : "",
							((frame.mData1 >> 8) & 1)? "OOR " : "",
							(int)(frame.mData1 & 0xFF)
						);
					AddResultString(ioState);
					AddResultString(buffer);
				}
				break;
			}
		}
		break;

	case SDIOAnalyzer::FRAME_LONG_ARG:
		AnalyzerHelpers::GetNumberString (frame.mData1, display_base, 64, number_str1, 128);
		AnalyzerHelpers::GetNumberString (frame.mData2, display_base, 64, number_str2, 128);
		AddResultString("LONG: ", number_str1, number_str2);
		break;

	case SDIOAnalyzer::FRAME_CRC:
		//AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, number_str1, 128);
		AddResultString((frame.mData1 & 0x80)? "O" : "X");
		AddResultString((frame.mData1 & 0x80)? "CRC" : "BAD");
		AddResultString((frame.mData1 & 0x80)? "CRC OK" : "BAD CRC");
		break;

	default:
		break;
	}
}

void	SDIOAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for(U32 i=0; i < num_frames; i++)
	{
		Frame frame = GetFrame(i);
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

		char number_str[128];
		AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);

		file_stream << time_str << ",";
		
		if(frame.mType == SDIOAnalyzer::FRAME_DIR)
		{
			file_stream << "DIR:";
			if(frame.mData1)
			{
					file_stream << "from Host";
			}
			else
			{
				file_stream << "from Slave";
			}
		}
		else if(frame.mType == SDIOAnalyzer::FRAME_CMD)
		{
			file_stream << "CMD:" << number_str;
		}
		else if(frame.mType == SDIOAnalyzer::FRAME_ARG)
		{
			file_stream << "ARG:" << number_str;
		}
		else if(frame.mType == SDIOAnalyzer::FRAME_LONG_ARG)
		{
			file_stream << "LONG_ARG:" << number_str;
		}
		else if(frame.mType == SDIOAnalyzer::FRAME_CRC)
		{
			file_stream << "CRC:" << number_str;
		}
		
		file_stream << std::endl;

		if(UpdateExportProgressAndCheckForCancel(i, num_frames) == true)
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void SDIOAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
	// Frame frame = GetFrame(frame_index);
	// ClearResultStrings();

	// char number_str[128];
	// AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
	// AddResultString(number_str);
}

void SDIOAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void SDIOAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}
