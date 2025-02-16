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

#pragma once

#include <ctime>

#include "Hardware/LocoCache.h"
#include "Hardware/Protocols/MaerklinCANCommon.h"
#include "Network/TcpConnection.h"
#include "Utils/Utils.h"

namespace Server { namespace CS2
{
	class CS2Server;

	class CS2Client : protected Hardware::Protocols::MaerklinCANCommon
	{
		public:
			CS2Client() = delete;
			CS2Client(const CS2Client&) = delete;
			CS2Client& operator=(const CS2Client&) = delete;

			inline CS2Client(const unsigned int id,
				Network::TcpConnection* connection, // connection must be deleted after using!
				Manager& manager)
			:	MaerklinCANCommon("affeaffe",
					ControlIdCS2Server,
					&manager,
					true,
					Logger::Logger::GetLogger("CS2Client # " + std::to_string(id))),
				id(id),
				connection(connection),
				terminated(false)
			{
				Init();
			}

			virtual ~CS2Client()
			{
			}

			inline void Stop()
			{
				run = false;
			}

			inline bool IsTerminated()
			{
				return terminated;
			}

		protected:
			void Receiver() override;

			void Send(const unsigned char* buffer) override;

		private:
			void ReceiverImpl();

			virtual void CacheSave(__attribute__((unused)) Hardware::LocoCacheEntry& entry) override
			{
			}

			virtual void CacheSave(__attribute__((unused)) Hardware::LocoCacheEntry& entry, __attribute__((unused)) const std::string& oldMatchKey) override
			{
			}

			virtual LocoID CacheDelete(__attribute__((unused)) const std::string& matchKey) override
			{
				return LocoNone;
			}

			virtual void CacheUpdateSlaves() override
			{
			}

			static const unsigned char CANCommandBufferLength = 13;

			unsigned int id;
			Network::TcpConnection* connection;
			bool terminated;
	};
}} // namespace Server::CS2

