/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2025 by Teddy / Dominik Mahrer - www.railcontrol.org

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

#include "Hardware/CS2Tcp.h"
#include "Utils/Utils.h"

namespace Hardware
{
	CS2Tcp::CS2Tcp(const HardwareParams* params)
	:	MaerklinCAN(params,
			"Maerklin Central Station 2 (CS2) TCP / " + params->GetName() + " at IP " + params->GetArg1(),
			params->GetName()),
	 	connection(Network::TcpClient::GetTcpClientConnection(HardwareInterface::logger, params->GetArg1(), CS2Port))
	{
		HardwareInterface::logger->Info(Languages::TextStarting, GetFullName());

		if (connection.IsConnected() == false)
		{
			HardwareInterface::logger->Error(Languages::TextUnableToCreateTcpSocket, params->GetArg1(), CS2Port);
		}
		Init();
	}

	void CS2Tcp::Send(const unsigned char* buffer)
	{
		if (connection.Send(buffer, CANCommandBufferLength) == -1)
		{
			HardwareInterface::logger->Error(Languages::TextUnableToSendDataToControl);
		}
	}

	void CS2Tcp::Receiver()
	{
		if (!connection.IsConnected())
		{
			HardwareInterface::logger->Error(Languages::TextUnableToReceiveData);
			return;
		}

		unsigned char buffer[CANCommandBufferLength];
		while(run)
		{
			ssize_t datalen = connection.ReceiveExact(buffer, sizeof(buffer));
			if (!run)
			{
				break;
			}

			if (datalen == -1)
			{
				if (errno == ETIMEDOUT)
				{
					continue;
				}
				HardwareInterface::logger->Error(Languages::TextErrorReadingData, strerror(errno));

				break;
			}

			if (datalen == 0)
			{
				// no data received
				continue;
			}

			if (datalen != 13)
			{
				HardwareInterface::logger->Error(Languages::TextErrorReadingData, strerror(errno));
				continue;
			}
			Parse(buffer);
		}
		connection.Terminate();
	}
} // namespace
