/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2024 by Teddy / Dominik Mahrer - www.railcontrol.org

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

#include "Hardware/CcSchnitte.h"
#include "Utils/Utils.h"

// OS X does not define B500000
#ifdef __APPLE__
#ifndef B500000
#define B500000 500000
#endif
#endif

namespace Hardware
{
	CcSchnitte::CcSchnitte(const HardwareParams* params)
	:	MaerklinCAN(params,
			"CC-Schnitte / " + params->GetName() + " at serial port " + params->GetArg1(),
			params->GetName()),
	 	serialLine(HardwareInterface::logger, params->GetArg1(), B500000, 8, 'N', 1, true)
	{
		HardwareInterface::logger->Info(Languages::TextStarting, GetFullName());

		Init();
	}

	void CcSchnitte::Send(const unsigned char* buffer)
	{
		if (!serialLine.IsConnected())
		{
			return;
		}
		if (serialLine.Send(buffer, CANCommandBufferLength) == -1)
		{
			HardwareInterface::logger->Error(Languages::TextUnableToSendDataToControl);
		}
	}

	void CcSchnitte::Receiver()
	{
		while (run)
		{
			if (!serialLine.IsConnected())
			{
				HardwareInterface::logger->Error(Languages::TextUnableToReceiveData);
				return;
			}
			unsigned char buffer[CANCommandBufferLength];
			ssize_t datalen = serialLine.ReceiveExact(buffer, sizeof(buffer));
			if (!run)
			{
				break;
			}
			if (datalen == 0)
			{
				continue;
			}
			if (datalen != sizeof(buffer))
			{
				HardwareInterface::logger->Error(Languages::TextInvalidDataReceived);
				continue;
			}
			Parse(buffer);
		}
	}
} // namespace
