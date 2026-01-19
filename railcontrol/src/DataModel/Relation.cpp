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

#include <map>
#include <sstream>
#include <string>

#include "DataModel/Relation.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::stringstream;
using std::string;

namespace DataModel
{
	std::string Relation::Serialize() const
	{
		stringstream ss;
		ss << LockableItem::Serialize()
			<< ";type=" << static_cast<int>(type)
			<< ";objectType1=" << static_cast<int>(ObjectType1())
			<< ";objectID1=" << object1.GetObjectID()
			<< ";objectType2=" << static_cast<int>(ObjectType2())
			<< ";objectID2=" << object2.GetObjectID()
			<< ";priority=" << static_cast<int>(priority)
			<< ";data=" << static_cast<int>(data);
		return ss.str();
	}

	void Relation::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		LockableItem::Deserialize(arguments);
		object1.SetObjectType(static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "objectType1")));
		type = static_cast<RelationType>(Utils::Utils::GetIntegerMapEntry(arguments, "type"));
		object1.SetObjectID(static_cast<ObjectID>(Utils::Utils::GetIntegerMapEntry(arguments, "objectID1")));
		object2.SetObjectType(static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "objectType2")));
		object2.SetObjectID(static_cast<ObjectID>(Utils::Utils::GetIntegerMapEntry(arguments, "objectID2")));
		priority = Utils::Utils::GetIntegerMapEntry(arguments, "priority");
		data = Utils::Utils::GetIntegerMapEntry(arguments, "data");
	}

	bool Relation::Execute(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier, const Delay delay)
	{
		switch (ObjectType2())
		{
			case ObjectTypeAccessory:
			{
				bool ret = manager->AccessoryState(ControlTypeInternal, ObjectID2(), static_cast<AccessoryState>(data), true);
				if (!ret)
				{
					return false;
				}
				break;
			}

			case ObjectTypeSwitch:
			{
				bool ret = manager->SwitchState(ControlTypeInternal, ObjectID2(), static_cast<AccessoryState>(data), true);
				if (!ret)
				{
					return false;
				}
				break;
			}

			case ObjectTypeSignal:
			{
				bool ret = manager->SignalState(ControlTypeInternal, ObjectID2(), static_cast<AccessoryState>(data), true);
				if (!ret)
				{
					return false;
				}
				break;
			}

			case ObjectTypeTrack:
				manager->TrackSetLocoOrientation(ObjectID2(), static_cast<Orientation>(data));
				return true;

			case ObjectTypeRoute:
				return manager->RouteExecute(logger, locoBaseIdentifier, ObjectID2());

			case ObjectTypeLoco:
			{
				const DataModel::LocoFunctionNr nr = static_cast<DataModel::LocoFunctionNr>(ObjectID2());
				if (data > DataModel::LocoFunctionStateOn)
				{
					manager->LocoBaseFunctionState(ControlTypeInternal, locoBaseIdentifier, nr, DataModel::LocoFunctionStateOn);
					Utils::Utils::SleepForMilliseconds(static_cast<unsigned int>(data) * 100);
					manager->LocoBaseFunctionState(ControlTypeInternal, locoBaseIdentifier, nr, DataModel::LocoFunctionStateOff);
				}
				else
				{
					manager->LocoBaseFunctionState(ControlTypeInternal, locoBaseIdentifier, nr, static_cast<DataModel::LocoFunctionState>(data));
				}
				return true;
			}

			case ObjectTypePause:
				Utils::Utils::SleepForMilliseconds(static_cast<unsigned int>(data) * 100);
				return true;

			case ObjectTypeMultipleUnit: // abused for loco orientation
				manager->LocoBaseOrientation(ControlTypeInternal, locoBaseIdentifier, static_cast<Orientation>(data));
				return true;

			case ObjectTypeBooster:
				manager->Booster(ControlTypeInternal, static_cast<BoosterState>(data));
				return true;

			case ObjectTypeCounter:
				return manager->Count(ObjectID2(), static_cast<CounterType>(data));

			default:
				return false;
		}
		Utils::Utils::SleepForMilliseconds(delay);
		return true;
	}

	Object* Relation::GetObject2()
	{
		switch (ObjectType2())
		{
			case ObjectTypeAccessory:
				return manager->GetAccessory(ObjectID2());

			case ObjectTypeSwitch:
				return manager->GetSwitch(ObjectID2());

			case ObjectTypeTrack:
				return manager->GetTrack(ObjectID2());

			case ObjectTypeSignal:
				return manager->GetSignal(ObjectID2());

			case ObjectTypeRoute:
				return manager->GetRoute(ObjectID2());

			case ObjectTypeCounter:
				return manager->GetCounter(ObjectID2());

			default:
				return nullptr;
		}
	}

	bool Relation::Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		const bool ret = LockableItem::Reserve(logger, locoBaseIdentifier);
		if (!ret)
		{
			logger->Debug(Languages::TextUnableToReserve);
			return false;
		}

		const ObjectType objectType2 = ObjectType2();
		switch(objectType2)
		{
			case ObjectTypeLoco:
			case ObjectTypePause:
			case ObjectTypeMultipleUnit: // abused for loco orientation
			case ObjectTypeBooster:
				return true;

			case ObjectTypeCounter:
			{
				Counter* counter = manager->GetCounter(ObjectID2());
				if (!counter)
				{
					logger->Debug(Languages::TextRelationTargetNotFound);
					LockableItem::Release(logger, locoBaseIdentifier);
					return false;
				}
				return counter->CheckCount(static_cast<CounterType>(data));
			}

			default:
				break;
		}

		LockableItem* lockable = dynamic_cast<LockableItem*>(GetObject2());
		if (!lockable)
		{
			logger->Debug(Languages::TextRelationTargetNotFound);
			LockableItem::Release(logger, locoBaseIdentifier);
			return false;
		}

		Route* route = dynamic_cast<Route*>(lockable);
		if (route)
		{
			return route->Reserve(logger, locoBaseIdentifier);
		}

		return lockable->Reserve(logger, locoBaseIdentifier);
	}

	bool Relation::Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		const bool ret = LockableItem::Lock(logger, locoBaseIdentifier);
		if (!ret)
		{
			return false;
		}

		const ObjectType objectType2 = ObjectType2();
		switch(objectType2)
		{
			case ObjectTypeLoco:
			case ObjectTypePause:
			case ObjectTypeMultipleUnit: // abused for loco orientation
			case ObjectTypeBooster:
				return true;

			case ObjectTypeCounter:
			{
				Counter* counter = manager->GetCounter(ObjectID2());
				if (!counter)
				{
					logger->Debug(Languages::TextRelationTargetNotFound);
					LockableItem::Release(logger, locoBaseIdentifier);
					return false;
				}
				return counter->CheckCount(static_cast<CounterType>(data));
			}

			default:
				break;
		}

		LockableItem* lockable = dynamic_cast<LockableItem*>(GetObject2());
		if (!lockable)
		{
			LockableItem::Release(logger, locoBaseIdentifier);
			return false;
		}

		Route* route = dynamic_cast<Route*>(lockable);
		if (route)
		{
			return route->Lock(logger, locoBaseIdentifier);
		}

		const bool retLockable = lockable->Lock(logger, locoBaseIdentifier);
		if (retLockable)
		{
			return true;
		}

		const Object* object = dynamic_cast<Object*>(lockable);
		if (!object)
		{
			return false;
		}

		logger->Debug(Languages::TextUnableToLock, object->GetName());
		return false;
	}

	bool Relation::CheckCondition(Logger::Logger* logger, __attribute__((unused)) const ObjectIdentifier& locoBaseIdentifier)
	{
		const ObjectType objectType2 = ObjectType2();
		switch(objectType2)
		{
			case ObjectTypeAccessory:
			{
				Accessory* accessory = manager->GetAccessory(ObjectID2());
				if (!accessory)
				{
					logger->Debug(Languages::TextRelationTargetNotFound);
					return false;
				}
				const bool state = accessory->CheckState(static_cast<AccessoryState>(data));
				if (state)
				{
					return true;
				}
				break;
			}

			case ObjectTypeSwitch:
			{
				Switch* mySwitch = manager->GetSwitch(ObjectID2());
				if (!mySwitch)
				{
					logger->Debug(Languages::TextRelationTargetNotFound);
					return false;
				}
				const bool state = mySwitch->CheckState(static_cast<AccessoryState>(data));
				if (state)
				{
					return true;
				}
				break;
			}

			case ObjectTypeSignal:
			{
				Signal* signal = manager->GetSignal(ObjectID2());
				if (!signal)
				{
					logger->Debug(Languages::TextRelationTargetNotFound);
					return false;
				}
				const bool state = signal->CheckState(static_cast<AccessoryState>(data));
				if (state)
				{
					return true;
				}
				break;
			}

			case ObjectTypeFeedback:
			{
				Feedback* feedback = manager->GetFeedback(ObjectID2());
				if (!feedback)
				{
					logger->Debug(Languages::TextRelationTargetNotFound);
					return false;
				}
				const bool state = feedback->CheckState(static_cast<Feedback::FeedbackState>(data));
				if (state)
				{
					return true;
				}
				break;
			}

			default:
				return true;
		}

		logger->Debug(Languages::TextConditionsNotFulfilled);
		return false;
	}

	bool Relation::Release(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		LockableItem* lockable = dynamic_cast<LockableItem*>(GetObject2());
		if (lockable)
		{
			lockable->Release(logger, locoBaseIdentifier);
		}
		return LockableItem::Release(logger, locoBaseIdentifier);
	}
} // namespace DataModel

