/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2026 by Teddy / Dominik Mahrer - www.railcontrol.org

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
#include <mutex>

#include "DataTypes.h"
#include "Logger/Logger.h"

namespace DataModel
{
	class LockableItem
	{
		public:
			enum LockState : unsigned char
			{
				LockStateFree = 0,
				LockStateReserved,
				LockStateSoftLocked,
				LockStateHardLocked
			};

			inline LockableItem()
			:	lockState(LockStateFree),
			 	locoBaseIdentifier()
			{
			}

			virtual ~LockableItem() {};

			std::string Serialize() const;
			void Deserialize(const std::map<std::string,std::string> arguments);

			inline const ObjectIdentifier& GetLocoBase() const
			{
				return locoBaseIdentifier;
			}

			inline LockState GetLockState() const
			{
				return lockState;
			}

			virtual bool Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);
			virtual bool Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);
			virtual bool Release(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);

			inline bool IsInUse() const
			{
				return lockState != LockStateFree || locoBaseIdentifier.IsSet();
			}

		private:
			std::mutex lockMutex;
			LockState lockState;
			ObjectIdentifier locoBaseIdentifier;
	};
} // namespace DataModel

