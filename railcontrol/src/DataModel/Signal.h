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
#include <string>

#include "DataModel/AccessoryBase.h"
#include "DataModel/LayoutItem.h"
#include "DataModel/LockableItem.h"
#include "DataModel/Track.h"
#include "DataTypes.h"
#include "Languages.h"

class Manager;

namespace DataModel
{
	class Signal : public AccessoryBase, public LayoutItem, public LockableItem
	{
		public:
			class StateOption
			{
				public:
					StateOption() = delete;
					StateOption& operator=(const StateOption&) = delete;

					inline StateOption(const Languages::TextSelector text, const AddressOffset addressOffset)
					:	text(text),
						addressOffset(addressOffset)
					{
					}

					inline StateOption(const StateOption&) = default;

					inline operator Languages::TextSelector() const
					{
						return text;
					}

					const Languages::TextSelector text;
					const AddressOffset addressOffset;
			};

			Signal() = delete;
			Signal(const Signal&) = delete;
			Signal& operator=(const Signal&) = delete;

			inline Signal(__attribute__((unused)) const Manager* const manager, const SignalID signalID)
			:	AccessoryBase(),
				LayoutItem(signalID),
				LockableItem(),
				track(nullptr)
			{
			}

			inline Signal(const Manager* const manager, const std::string& serialized)
			:	Signal(manager, SignalNone)
			{
				Deserialize(serialized);
			}

			virtual void SetAccessoryType(AccessoryType type) override;

			virtual ObjectType GetObjectType() const override;

			virtual std::string GetLayoutType() const override;

			std::string Serialize() const override;

			using HardwareHandle::Deserialize;
			virtual bool Deserialize(const std::string& serialized) override;

			inline Track* GetTrack() const
			{
				return track;
			}

			inline void SetTrack(Track* const track = nullptr)
			{
				this->track = track;
			}

			bool UsesAddress(Address address) const;

			std::map<DataModel::AccessoryState,StateOption> GetStateOptions() const;

			inline Address GetMappedAddress() const
			{
				AddressOffset offset = GetStateAddressOffset();
				return offset < 0 ? 0 : GetAddress() + ((offset) >> 1);
			}

			inline AccessoryState GetMappedAccessoryState() const
			{
				return static_cast<AccessoryState>(GetStateAddressOffset() & 0x01);
			}

			inline void SetStateAddressOffset(const AccessoryState state, const AddressOffset addressOffset)
			{
				stateAddressMap[state] = addressOffset;
			}

			inline void SetStateAddressOffsets(const std::map<AccessoryState,AddressOffset>& newOffsets)
			{
				stateAddressMap = newOffsets;
			}

			inline DataModel::AddressOffset GetStateAddressOffset() const
			{
				return GetStateAddressOffset(GetAccessoryState());
			}

			inline AddressOffset GetStateAddressOffset(const AccessoryState state) const
			{
				return stateAddressMap.count(state) != 1 ? -1 : stateAddressMap.at(state);
			}

			AccessoryState CalculateMappedSignalState(const Address address, const AccessoryState state) const;

			Signal& operator=(const Hardware::AccessoryCacheEntry& accessory);

		private:
			void ResetStateAddressMap();

			std::map<AccessoryState,AddressOffset> stateAddressMap;

			Track* track;
	};
} // namespace DataModel

