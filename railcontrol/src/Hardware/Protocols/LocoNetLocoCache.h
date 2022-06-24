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

#pragma once

#include "DataTypes.h"

namespace Hardware
{
	namespace Protocols
	{
		class LocoNetLocoCacheEntry
		{
			public:
				inline LocoNetLocoCacheEntry()
				:	address(0),
					orientationF0F4(0)
				{
				}

				inline void SetAddress(const Address address)
				{
					this->address = address;
				}

				inline Address GetAddress() const
				{
					return address;
				}

				inline void SetOrientationF0F4(const unsigned char orientationF0F4)
				{
					this->orientationF0F4 = orientationF0F4;
				}

				inline unsigned char GetOrientationF0F4()
				{
					return orientationF0F4;
				}

				Address address;
				unsigned char orientationF0F4;
		};

		class LocoNetLocoCache
		{
			public:
				LocoNetLocoCache(const LocoNetLocoCache&) = delete;
				LocoNetLocoCache& operator=(const LocoNetLocoCache&) = delete;

				LocoNetLocoCache()
				{
				}

				virtual ~LocoNetLocoCache()
				{
				}

				inline void SetAddress(const unsigned char slot, const Address address)
				{
					if (slot == 0 || slot > MaxLocoNetSlot)
					{
						return;
					}
					entries[slot].SetAddress(address);
				}

				inline unsigned char GetSlotOfAddress(const Address address)
				{
					for (unsigned char slot = MinLocoNetSlot; slot <= MaxLocoNetSlot; ++slot)
					{
						if (address == entries[slot].GetAddress())
						{
							return slot;
						}
					}
					return 0;
				}

				inline Address GetAddressOfSlot(unsigned char slot)
				{
					if (slot == 0 || slot > MaxLocoNetSlot)
					{
						return 0;
					}
					return entries[slot].GetAddress();
				}

				inline void SetOrientationF0F4(const unsigned char slot, const unsigned char orientationF0F4)
				{
					if (slot == 0 || slot > MaxLocoNetSlot)
					{
						return;
					}
					entries[slot].SetOrientationF0F4(orientationF0F4);
				}

				inline unsigned char GetOrientationF0F4(const unsigned char slot)
				{
					if (slot == 0 || slot > MaxLocoNetSlot)
					{
						return 0;
					}
					return entries[slot].GetOrientationF0F4();
				}

				static const unsigned char MinLocoNetSlot = 1;
				static const unsigned char MaxLocoNetSlot = 119;

			private:
				LocoNetLocoCacheEntry entries[MaxLocoNetSlot + 1];

		};
	} // namespace
} // namespace

