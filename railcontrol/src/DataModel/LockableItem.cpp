/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2023 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "DataModel/LockableItem.h"
#include "DataModel/Object.h"
#include "Utils/Utils.h"

using std::map;
using std::stringstream;
using std::string;

namespace DataModel
{
	string LockableItem::Serialize() const
	{
		return "lockstate=" + std::to_string(lockState)
			+ ";locobaseid=" + std::to_string(locoBaseIdentifier.GetObjectID())
			+ ";locobasetype=" + std::to_string(locoBaseIdentifier.GetObjectType());
	}

	bool LockableItem::Deserialize(const map<string, string> arguments)
	{
		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "locoID", LocoNone); // FIXME: Remove later: 2024-02-08
		locoBaseIdentifier.SetObjectID(Utils::Utils::GetIntegerMapEntry(arguments, "locobaseid", locoID));
		locoBaseIdentifier.SetObjectType(static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "locobasetype", ObjectTypeLoco)));
		lockState = static_cast<LockState>(Utils::Utils::GetIntegerMapEntry(arguments, "lockState", LockStateFree));  // FIXME: Remove later: 2024-02-08
		lockState = static_cast<LockState>(Utils::Utils::GetIntegerMapEntry(arguments, "lockstate", lockState));
		return true;
	}

	bool LockableItem::Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		std::lock_guard<std::mutex> Guard(lockMutex);
		if (this->locoBaseIdentifier == locoBaseIdentifier)
		{
			if (lockState == LockStateFree)
			{
				lockState = LockStateReserved;
			}
			return true;
		}

		if (this->locoBaseIdentifier.IsSet())
		{
			Object *object = dynamic_cast<Object*>(this);
			if (object == nullptr)
			{
				return false;
			}

			logger->Debug(Languages::TextIsNotFree, object->GetName());
			return false;
		}

		if (lockState != LockStateFree)
		{
			Object *object = dynamic_cast<Object*>(this);
			if (object == nullptr)
			{
				return false;
			}

			logger->Debug(Languages::TextIsNotFree, object->GetName());
			return false;
		}
		lockState = LockStateReserved;
		this->locoBaseIdentifier = locoBaseIdentifier;
		return true;
	}

	bool LockableItem::Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		std::lock_guard<std::mutex> Guard(lockMutex);

		if (this->locoBaseIdentifier != locoBaseIdentifier)
		{
			Object *object = dynamic_cast<Object*>(this);
			if (object == nullptr)
			{
				return false;
			}

			logger->Debug(Languages::TextIsNotFree, object->GetName());
			return false;
		}

		if (lockState != LockStateReserved && lockState != LockStateHardLocked)
		{
			Object *object = dynamic_cast<Object*>(this);
			if (object == nullptr)
			{
				return false;
			}

			logger->Debug(Languages::TextIsNotFree, object->GetName());
			return false;
		}

		lockState = LockStateHardLocked;
		return true;
	}

	bool LockableItem::Release(__attribute__((unused)) Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		std::lock_guard<std::mutex> Guard(lockMutex);
		if (this->locoBaseIdentifier != locoBaseIdentifier && locoBaseIdentifier.IsSet())
		{
			return false;
		}
		this->locoBaseIdentifier.Clear();
		lockState = LockStateFree;
		return true;
	}

} // namespace DataModel

