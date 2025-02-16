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

#include "DataTypes.h"
#include "DataModel/AccessoryBase.h"
#include "DataModel/HardwareHandle.h"
#include "DataModel/LayoutItem.h"
#include "DataModel/LockableItem.h"
#include "Languages.h"

class Manager;

namespace DataModel
{
	class Accessory : public AccessoryBase, public LayoutItem, public LockableItem
	{
			Accessory() = delete;
			Accessory(const Accessory&) = delete;
			Accessory& operator=(const Accessory&) = delete;

		public:
			inline Accessory(const AccessoryID accessoryID)
			:	AccessoryBase(),
				LayoutItem(accessoryID),
				LockableItem(),
				port(AddressPortRed)
			{
			}

			inline Accessory(__attribute__((unused)) Manager* manager, const AccessoryID accessoryID)
			:	Accessory(accessoryID)
			{
			}

			inline Accessory(const std::string& serialized)
			:	Accessory(AccessoryNone)
			{
				Deserialize(serialized);
			}

			virtual ~Accessory()
			{
			}

			virtual ObjectType GetObjectType() const override
			{
				return ObjectTypeAccessory;
			}

			virtual std::string GetLayoutType() const override
			{
				return Languages::GetText(Languages::TextAccessory);
			}

			virtual std::string Serialize() const override;

			using HardwareHandle::Deserialize;
			virtual bool Deserialize(const std::string& serialized) override;

			Accessory& operator=(const Hardware::AccessoryCacheEntry& accessory);

			inline void SetPort(AddressPort port)
			{
				this->port = port;
			}

			inline AddressPort GetPort() const
			{
				return port;
			}

		private:
			AddressPort port;
	};
} // namespace DataModel

