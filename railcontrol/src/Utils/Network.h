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

#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace Utils
{
	class Network
	{
		public:
			Network() = delete;
			Network(const Network&) = delete;
			Network& operator=(const Network&) = delete;

			static std::string AddressToString(const struct sockaddr_storage *sockaddr);

			static bool CompareAddresses(const struct sockaddr_storage *a1, const struct sockaddr_storage *a);

			static bool HostResolves(const std::string& host);
	};
} // namespace Utils
