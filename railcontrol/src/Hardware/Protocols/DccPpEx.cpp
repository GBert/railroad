/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2022 Dominik (Teddy) Mahrer - www.railcontrol.org

RailControl is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

RailControl is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RailControl; see the file LICENCE. If not see
<http://www.gnu.org/licenses/>.
*/

#include <string>

#include "DataModel/Feedback.h"
#include "Manager.h"
#include "Hardware/Protocols/DccPpEx.h"

using std::string;
using std::to_string;

namespace Hardware
{
	namespace Protocols
	{
		void DccPpEx::Booster(const BoosterState status)
		{
			string buffer("<");
			logger->Info(status ? Languages::TextTurningBoosterOn : Languages::TextTurningBoosterOff);
			buffer += to_string(static_cast<unsigned char>(status));
			buffer += ">";
			SendInternal(buffer);
		}

		void DccPpEx::LocoSpeed(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const Speed speed)
		{
			locoCache.SetSpeed(address, speed);
			Orientation orientation = locoCache.GetOrientation(address);
			LocoSpeedOrientation(address, speed, orientation);
		}

		void DccPpEx::LocoOrientation(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const Orientation orientation)
		{
			Speed speed = locoCache.GetSpeed(address);
			locoCache.SetOrientation(address, orientation);
			LocoSpeedOrientation(address, speed, orientation);
		}

		void DccPpEx::LocoFunction(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const DataModel::LocoFunctionNr function,
			const DataModel::LocoFunctionState on)
		{
			string buffer("<F ");
			buffer += to_string(address);
			buffer += " ";
			buffer += to_string(function);
			buffer += " ";
			buffer += on ? "1" : "0";
			buffer += ">";
			SendInternal(buffer);
		}

		void DccPpEx::AccessoryOnOrOff(__attribute__((unused)) const Protocol protocol,
			const Address address,
			const DataModel::AccessoryState state,
			__attribute__((unused)) const bool on)
		{
			string buffer("<a ");
			buffer += to_string(address);
			buffer += " ";
			buffer += state == DataModel::AccessoryStateOff ? "0" : "1";
			buffer += ">";
			SendInternal(buffer);
		}

		void DccPpEx::ProgramWrite(const ProgramMode mode,
			const Address address,
			const CvNumber cv,
			const CvValue value)
		{
			switch(mode)
			{
				case ProgramModeDccDirect:
					ProgramWriteProgram(cv, value);
					return;

				case ProgramModeDccPomLoco:
					ProgramWriteMain(address, cv, value);
					return;

				default:
					return;
			}
		}

		void DccPpEx::ProgramWriteMain(const Address address,
			const CvNumber cv,
			const CvValue value)
		{
			string buffer("<w ");
			buffer += to_string(address);
			buffer += " ";
			buffer += to_string(cv);
			buffer += " ";
			buffer += to_string(value);
			buffer += ">";
			SendInternal(buffer);
		}

		void DccPpEx::ProgramWriteProgram(const CvNumber cv,
			const CvValue value)
		{
			string buffer("<W ");
			buffer += to_string(cv);
			buffer += " ";
			buffer += to_string(value);
			buffer += ">";
			SendInternal(buffer);
		}

		void DccPpEx::ProgramRead(const ProgramMode mode,
			__attribute__((unused)) const Address address,
			const CvNumber cv)
		{
			switch(mode)
			{
				case ProgramModeDccDirect:
					ProgramReadProgram(cv);
					return;

				default:
					return;
			}
		}

		void DccPpEx::ProgramReadProgram(const CvNumber cv)
		{
			string buffer("<R ");
			buffer += to_string(cv);
			buffer += " 8 9>";
			SendInternal(buffer);
		}

		void DccPpEx::LocoSpeedOrientation(const Address address,
			const Speed speed,
			const Orientation orientation)
		{
			string buffer("<t 1 ");
			buffer += to_string(address);
			buffer += " ";
			buffer += to_string(speed >= 1008 ? 126 : speed >> 3);
			buffer += " ";
			buffer += to_string(orientation);
			buffer += ">";
			SendInternal(buffer);
		}

		void DccPpEx::Receiver()
		{
			Utils::Utils::SetThreadName("DCC-EX");
			logger->Info(Languages::TextReceiverThreadStarted);
			while (true)
			{
				string buffer;
				const bool ret = ReceiveData(buffer);
				if (!ret)
				{
					break;
				}
				Parse(buffer);
			}
			logger->Info(Languages::TextTerminatingReceiverThread);
		}

		bool DccPpEx::ReceiveData(string& buffer)
		{
			size_t size = 0;
			while (size == 0 || (buffer[size - 1] != '>' && buffer[size - 1] != 0x0a))
			{
				if (!run)
				{
					return false;
				}
				ReceiveInternal(buffer);
				size = buffer.size();
			}
			return true;
		}

		void DccPpEx::Parse(const string& buffer)
		{
			unsigned int pos = 0;
			while (buffer.size() > pos)
			{
				if (buffer[pos] != '<')
				{
					++pos;
					continue;
				}

				++pos;
				if (buffer.size() <= pos)
				{
					return;
				}
				switch(buffer[pos++])
				{
					case 'Q':
					{
						// pin is high
						const FeedbackPin pin = ParseInt(buffer, pos);
						logger->Info(Languages::TextFeedbackChange, pin & 0x000F, pin >> 4, Languages::GetText(Languages::TextOn));
						manager->FeedbackState(controlID, pin, DataModel::Feedback::FeedbackStateOccupied);
						break;
					}

					case 'q':
					{
						// pin is low
						const FeedbackPin pin = ParseInt(buffer, pos);
						logger->Info(Languages::TextFeedbackChange, pin & 0x000F, pin >> 4, Languages::GetText(Languages::TextOff));
						manager->FeedbackState(controlID, pin, DataModel::Feedback::FeedbackStateFree);
						break;
					}

					case 'r':
					{
						// read CV value
						ParseInt(buffer, pos); // should be 8, sent in ProgramReadProgram
						ParsePipe(buffer, pos);
						ParseInt(buffer, pos); // should be 9, sent in ProgramReadProgram
						ParsePipe(buffer, pos);
						const CvNumber cv = ParseInt(buffer, pos);
						const int value = ParseInt(buffer, pos);
						if (value < 0)
						{
							// read error
							break;
						}
						logger->Info(Languages::TextProgramReadValue, cv, static_cast<CvValue>(value));
						manager->ProgramValue(cv, value);
						break;
					}

					default:
						// other answer does not matter
						break;
				}

				do
				{
					++pos;
				}
				while (buffer.size() > pos && buffer[pos] != '>');
			}
		}

		int DccPpEx::ParseInt(const string& buffer, unsigned int& pos)
		{
			ParseSpace(buffer, pos);
			bool minus = false;
			int out = 0;
			while (buffer.size() > pos)
			{
				const char c = buffer[pos];
				if (c == '-')
				{
					minus = true;
					++pos;
					continue;
				}
				if (c < '0' || c > '9')
				{
					break;
				}
				out *= 10;
				out += c - '0';
				++pos;
			}
			return minus ? -out : out;
		}

		void DccPpEx::ParseSpace(const string& buffer, unsigned int& pos)
		{
			while (buffer.size() > pos)
			{
				const char c = buffer[pos];
				if (c != ' ')
				{
					return;
				}
				++pos;
			}
		}

		bool DccPpEx::ParsePipe(const string& buffer, unsigned int& pos)
		{
			if (buffer.size() <= pos)
			{
				return false;
			}

			const char c = buffer[pos];
			if (c != '|')
			{
				return false;
			}

			++pos;
			return true;
		}
	} // namespace
} // namespace
