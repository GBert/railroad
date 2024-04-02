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

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "DataTypes.h"
#include "DataModel/LocoFunctions.h"
#include "DataModel/LocoBase.h"
#include "DataModel/Relation.h"

namespace DataModel
{
	class Loco : public LocoBase
	{
		public:

			Loco() = delete;
			Loco(const Loco&) = delete;
			Loco& operator=(const Loco&) = delete;

			inline Loco(Manager* manager, const LocoID locoID)
			:	LocoBase(manager, locoID)
			{
			}

			inline Loco(Manager* manager, const std::string& serialized)
			:	LocoBase(manager, serialized)
			{
				Loco::Deserialize(serialized);
			}

			virtual ~Loco()
			{
			}

			inline ObjectType GetObjectType() const override
			{
				return ObjectTypeLoco;
			}

			std::string Serialize() const override;

			bool Deserialize(const std::string& serialized) override;

			Loco& operator=(const Hardware::LocoCacheEntry& loco);
	};
} // namespace DataModel
