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

#include <map>
#include <string>

#include "DataModel/Accessory.h"
#include "DataModel/LockableItem.h"
#include "DataModel/ObjectIdentifier.h"
#include "DataModel/Serializable.h"
#include "DataTypes.h"
#include "Logger/Logger.h"

class Manager;

namespace DataModel
{
	class Relation : protected Serializable, public LockableItem
	{
		public:
			enum RelationType : unsigned char
			{
				RelationTypeCalculate         = 0,
				RelationTypeLocoSlave         = (ObjectTypeLoco << 3), // FIXME: 2024-03-17: not used anymore, TypeMultipeUnitSlave instead
				RelationTypeTrackSignal       = (ObjectTypeTrack << 3),
				RelationTypeTrackFeedback     = (ObjectTypeTrack << 3) + 1,
				RelationTypeFeedbackAtSet     = (ObjectTypeFeedback << 3),
				RelationTypeFeedbackAtUnset   = (ObjectTypeFeedback << 3) + 1,
				RelationTypeRouteAtLock       = (ObjectTypeRoute << 3),
				RelationTypeRouteAtUnlock     = (ObjectTypeRoute << 3) + 1,
				RelationTypeClusterTrack      = (ObjectTypeCluster << 3),
				RelationTypeMultipleUnitLoco  = (ObjectTypeMultipleUnit << 3),
			};

			typedef unsigned short Data;
			static const Data DefaultData = 0;

			inline Relation(Manager* manager,
				const ObjectIdentifier& object1,
				const ObjectIdentifier& object2,
				const RelationType type,
				const Priority priority = 0,
				const Data data = 0)
			:	manager(manager),
				object1(object1),
				object2(object2),
				type(type),
				priority(priority),
				data(data)
			{
			}

			inline Relation(Manager* manager,
				const std::string& serialized)
			:	manager(manager),
				data(0)
			{
				Deserialize(serialized);
			}

			virtual ~Relation()
			{
			}

			virtual std::string Serialize() const override;
			virtual void Deserialize(const std::string& serialized) override;

			inline ObjectID ObjectID1() const
			{
				return object1.GetObjectID();
			}

			inline void ObjectID1(ObjectID objectID1)
			{
				object1.SetObjectID(objectID1);
			}

			inline ObjectType ObjectType2() const
			{
				return object2.GetObjectType();
			}

			inline ObjectID ObjectID2() const
			{
				return object2.GetObjectID();
			}

			inline ObjectIdentifier ObjectIdentifier2() const
			{
				return object2;
			}

			Object* GetObject2();

			inline RelationType GetType() const
			{
				return type;
			}

			inline Priority GetPriority() const
			{
				return priority;
			}

			inline Data GetData() const
			{
				return data;
			}

			inline bool CompareObject2(const ObjectIdentifier& identifier) const
			{
				return object2 == identifier;
			}

			bool Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;
			bool Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;
			bool Release(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;
			bool Execute(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier, const Delay delay);

		private:
			inline ObjectType ObjectType1() const
			{
				return object1.GetObjectType();
			}

			Manager* manager;
			ObjectIdentifier object1;
			ObjectIdentifier object2;
			RelationType type;
			Priority priority;
			Data data;
	};
} // namespace DataModel

