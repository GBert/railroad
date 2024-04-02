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

#pragma once

#include <map>

#include "DataTypes.h"

namespace Hardware
{
	namespace Protocols
	{
		class Z21TurnoutCacheEntry
		{
			public:
				Z21TurnoutCacheEntry(const Z21TurnoutCacheEntry&) = delete;

				inline Z21TurnoutCacheEntry()
				:	protocol(ProtocolNone)
				{
				}

				inline Z21TurnoutCacheEntry& operator=(const Protocol protocol)
				{
					this->protocol = protocol;
					return *this;
				}

				inline Z21TurnoutCacheEntry& operator=(const Z21TurnoutCacheEntry&) = delete;

				Protocol protocol;
		};

		class Z21TurnoutCache
		{
			public:
				Z21TurnoutCache& operator=(const Z21TurnoutCache&) = delete;

				inline void SetProtocol(const Address address, const Protocol protocol)
				{
					cache[address] = protocol;
				}

				inline Protocol GetProtocol(const Address address)
				{
					if (cache.count(address) == 0)
					{
						return ProtocolDCC;
					}
					return cache[address].protocol;
				}

			private:
				std::map<Address, Z21TurnoutCacheEntry> cache;
		};
	} // namespace
} // namespace

