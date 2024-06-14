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

#include <cstring>		//memset

#include "DataModel/ObjectIdentifier.h"
#include "Manager.h"
#include "Server/CS2/CS2Client.h"
#include "Utils/Utils.h"

using DataModel::ObjectIdentifier;

namespace Server { namespace CS2
{
	// worker is the thread that handles client requests
	void CS2Client::Receiver()
	{
		Utils::Utils::SetThreadName("CS2Client # " + std::to_string(id) + " receiver");
		logger->Debug(Languages::TextTcpConnectionEstablished, connection->AddressAsString());
		ReceiverImpl();
		logger->Debug(Languages::TextTcpConnectionClosed, connection->AddressAsString());
		terminated = true;
	}

	void CS2Client::ReceiverImpl()
	{
		while (run)
		{
			unsigned char buffer[CANCommandBufferLength];
			int ret = connection->ReceiveExact(buffer, sizeof(buffer), 0);

			if (!run)
			{
				return;
			}

			if (ret == -1)
			{
				if (errno != ETIMEDOUT)
				{
					logger->Error(Languages::TextErrorReadingData, strerror(errno));
					return;
				}
				continue;
			}

			if (ret == 0)
			{
				continue;
			}

			if (ret != 13)
			{
				logger->Error(Languages::TextErrorReadingData, strerror(errno));
			}

			Parse(buffer);
		}
	}

	void CS2Client::Send(const unsigned char* buffer)
	{
		if (connection->Send(buffer, CANCommandBufferLength) == -1)
		{
			logger->Error(Languages::TextUnableToSendDataToControl);
		}
	}
}} // namespace Server::CS2
