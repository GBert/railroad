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
#include <thread>
#include "Hardware/Protocols/LocoNet.h"
#include "Utils/Utils.h"

namespace Hardware
{
	namespace Protocols
	{
		LocoNet::LocoNet(const HardwareParams* params, const std::string& controlName)
		:	HardwareInterface(params->GetManager(),
			   params->GetControlID(),
				controlName + " / " + params->GetName() + " at serial port " + params->GetArg1(),
			   params->GetName()),
			run(true),
			serialLine(logger, params->GetArg1(), B19200, 8, 'N', 1)
		{
			receiverThread = std::thread(&Hardware::Protocols::LocoNet::Receiver, this);
		}

		LocoNet::~LocoNet()
		{
			run = false;
			receiverThread.join();
			logger->Info(Languages::TextTerminatingSenderSocket);
		}

		void LocoNet::Booster(const BoosterState status)
		{
			unsigned char cmd[2];
			cmd[0] = status ? OPC_GPON : OPC_GPOFF;
			CalcCheckSum(cmd, sizeof(cmd));
			serialLine.Send(cmd, sizeof(cmd));
		}

		void LocoNet::Receiver()
		{
			Utils::Utils::SetThreadName("LocoNet Receiver");
			logger->Info(Languages::TextReceiverThreadStarted);

			const unsigned char BufferSize = 128u;
			unsigned char buffer[BufferSize];
			while (run)
			{
				if (!serialLine.IsConnected())
				{
					logger->Error(Languages::TextUnableToReceiveData);
					return;
				}
				ssize_t dataLength = serialLine.ReceiveExact(buffer, sizeof(buffer));
				if (run == false)
				{
					break;
				}
				if (dataLength == 0)
				{
					continue;
				}
				logger->Hex(buffer, dataLength);
//				Parse(buffer);
			}
			logger->Info(Languages::TextTerminatingReceiverThread);
		}

		void LocoNet::CalcCheckSum(unsigned char* data, unsigned char length)
		{
			--length;
			data[length] = 0xFF;
			for (unsigned char i = 0; i < length; ++i)
			{
				data[length] ^= data[i];
			}
		}
	} // namespace
} // namespace
