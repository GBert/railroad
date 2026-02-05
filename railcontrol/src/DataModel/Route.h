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

#include <mutex>
#include <string>
#include <vector>

#include "DataTypes.h"
#include "DataModel/LayoutItem.h"
#include "DataModel/LockableItem.h"
#include "DataModel/ObjectIdentifier.h"
#include "Logger/Logger.h"

class Manager;

namespace DataModel
{
	class LocoBase;
	class Relation;

	class Route : public LayoutItem, public LockableItem
	{
		public:
			static const Delay DefaultDelay = 250;

			enum PushpullType : unsigned char
			{
				PushpullTypeNo = 0,
				PushpullTypeOnly = 1,
				PushpullTypeBoth = 2
			};

			enum Speed : unsigned char
			{
				SpeedMax = 3,
				SpeedTravel = 2,
				SpeedReduced = 1,
				SpeedCreeping = 0
			};

			Route() = delete;

			Route(Manager* manager, const RouteID routeID)
			:	LayoutItem(routeID),
				LockableItem(),
				manager(manager),
				executeAtUnlock(false),
				delay(0),
				pushpull(PushpullTypeBoth),
				propulsion(PropulsionUnknown),
				trainType(TrainTypeAll),
				minTrainLength(0),
				maxTrainLength(0),
				automode(AutomodeNo),
				fromTrack(),
				fromOrientation(OrientationRight),
				toTrack(),
				toOrientation(OrientationRight),
				speed(SpeedTravel),
				feedbackIdReduced(FeedbackNone),
				reducedDelay(0),
				feedbackIdCreep(FeedbackNone),
				creepDelay(0),
				feedbackIdStop(FeedbackNone),
				stopDelay(0),
				feedbackIdOver(FeedbackNone),
				waitAfterRelease(0),
				lastUsed(0),
				counter(0)
			{
				SetRotation(LayoutItem::Rotation0);
			}

			Route(Manager* manager, const std::string& serialized);

			inline ~Route()
			{
				DeleteRelations(relationsAtLock);
				DeleteRelations(relationsAtUnlock);
				DeleteRelations(relationsConditions);
			}

			inline ObjectType GetObjectType() const override
			{
				return ObjectTypeRoute;
			}

			std::string Serialize() const override;
			void Deserialize(const std::string& serialized) override;

			inline std::string GetLayoutType() const override
			{
				return Languages::GetText(Languages::TextRoute);
			}

			inline void DeleteRelationsAtLock()
			{
				DeleteRelations(relationsAtLock);
			}

			inline void DeleteRelationsAtUnlock()
			{
				DeleteRelations(relationsAtUnlock);
			}

			inline bool AssignRelationsAtLock(const std::vector<DataModel::Relation*>& newRelations)
			{
				return AssignRelations(relationsAtLock, newRelations);
			}

			inline bool AssignRelationsAtUnlock(const std::vector<DataModel::Relation*>& newRelations)
			{
				return AssignRelations(relationsAtUnlock, newRelations);
			}

			inline bool AssignRelationsConditions(const std::vector<DataModel::Relation*>& newConditions)
			{
				return AssignRelations(relationsConditions, newConditions);
			}

			inline const std::vector<DataModel::Relation*>& GetRelationsAtLock() const
			{
				return relationsAtLock;
			}

			inline const std::vector<DataModel::Relation*>& GetRelationsAtUnlock() const
			{
				return relationsAtUnlock;
			}

			inline const std::vector<DataModel::Relation*>& GetRelationsConditions() const
			{
				return relationsConditions;
			}

			bool FromTrackOrientation(Logger::Logger* logger,
				const TrackID trackID,
				const Orientation trackOrientation,
				const DataModel::LocoBase* loco,
				const bool allowLocoTurn);

			bool Execute(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);

			inline static bool ExecuteStatic(Logger::Logger* logger, Route* route)
			{
				return route->Execute(logger, ObjectIdentifier());
			}

			bool Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;

			bool Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override;

			bool CheckConditions(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);

			bool Release(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier) override
			{
				std::lock_guard<std::mutex> Guard(updateMutex);
				return ReleaseInternal(logger, locoBaseIdentifier);
			}

			inline Delay GetDelay() const
			{
				return delay;
			}

			inline void SetDelay(Delay delay)
			{
				this->delay = delay;
			}

			inline PushpullType GetPushpull() const
			{
				return pushpull;
			}

			inline void SetPushpull(const PushpullType pushpull)
			{
				this->pushpull = pushpull;
			}

			inline Propulsion GetPropulsion() const
			{
				return propulsion;
			}

			inline void SetPropulsion(const Propulsion propulsion)
			{
				this->propulsion = propulsion;
			}

			inline TrainType GetTrainType() const
			{
				return trainType;
			}

			inline void SetTrainType(const TrainType trainType)
			{
				this->trainType = trainType;
			}

			inline Length GetMinTrainLength() const
			{
				return minTrainLength;
			}

			inline void SetMinTrainLength(const Length length)
			{
				this->minTrainLength = length;
			}

			inline Length GetMaxTrainLength() const
			{
				return maxTrainLength;
			}

			inline void SetMaxTrainLength(const Length length)
			{
				this->maxTrainLength = length;
			}

			inline time_t GetLastUsed() const
			{
				return lastUsed;
			}

			inline void SetAutomode(const Automode automode)
			{
				this->automode = automode;
			}

			inline Automode GetAutomode() const
			{
				return automode;
			}

			inline void SetFromTrack(const TrackID fromTrack)
			{
				this->fromTrack = fromTrack;
			}

			inline TrackID GetFromTrack() const
			{
				return fromTrack;
			}

			inline void SetFromOrientation(const Orientation fromOrientation)
			{
				this->fromOrientation = fromOrientation;
			}

			inline Orientation GetFromOrientation() const
			{
				return fromOrientation;
			}

			inline void SetToTrack(const TrackID toTrack)
			{
				this->toTrack = toTrack;
			}

			inline TrackID GetToTrack() const
			{
				return toTrack;
			}

			inline void SetToOrientation(const Orientation toOrientation)
			{
				this->toOrientation = toOrientation;
			}

			inline Orientation GetToOrientation() const
			{
				return toOrientation;
			}

			inline void SetSpeed(Speed startSpeed)
			{
				this->speed = startSpeed;
			}

			inline Speed GetSpeed() const
			{
				return speed;
			}

			inline void SetFeedbackIdReduced(const FeedbackID feedbackIdReduced)
			{
				this->feedbackIdReduced = feedbackIdReduced;
			}

			inline FeedbackID GetFeedbackIdReduced() const
			{
				return feedbackIdReduced;
			}

			inline void SetReducedDelay(const Delay reducedDelay)
			{
				this->reducedDelay = reducedDelay;
			}

			inline Delay GetReducedDelay() const
			{
				return reducedDelay;
			}

			inline void SetFeedbackIdCreep(const FeedbackID feedbackIdCreep)
			{
				this->feedbackIdCreep = feedbackIdCreep;
			}

			inline FeedbackID GetFeedbackIdCreep() const
			{
				return feedbackIdCreep;
			}

			inline void SetCreepDelay(const Delay creepDelay)
			{
				this->creepDelay = creepDelay;
			}

			inline Delay GetCreepDelay() const
			{
				return creepDelay;
			}

			inline void SetFeedbackIdStop(const FeedbackID feedbackIdStop)
			{
				this->feedbackIdStop = feedbackIdStop;
			}

			inline FeedbackID GetFeedbackIdStop() const
			{
				return feedbackIdStop;
			}

			inline void SetStopDelay(const Delay stopDelay)
			{
				this->stopDelay = stopDelay;
			}

			inline Delay GetStopDelay() const
			{
				return stopDelay;
			}

			inline void SetFeedbackIdOver(const FeedbackID feedbackIdOver)
			{
				this->feedbackIdOver = feedbackIdOver;
			}

			inline FeedbackID GetFeedbackIdOver() const
			{
				return feedbackIdOver;
			}

			inline void SetWaitAfterRelease(const Pause wait)
			{
				this->waitAfterRelease = wait;
			}

			inline Pause GetWaitAfterRelease() const
			{
				return waitAfterRelease;
			}

			inline void SetFollowUpRoute(const RouteID followUpRoute)
			{
				this->followUpRoute = followUpRoute;
			}

			inline RouteID GetFollowUpRoute() const
			{
				return followUpRoute;
			}

			static inline bool CompareShortest(const Route* s1, const Route* s2)
			{
				return s1->GetMinTrainLength() < s2->GetMinTrainLength();
			}
			static inline bool CompareLastUsed(const Route* s1, const Route* s2)
			{
				return s1->GetLastUsed() < s2->GetLastUsed();
			}

			bool ObjectIsPartOfRoute(const ObjectIdentifier& identifier) const;

		private:
			bool ReleaseInternal(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);
			void ReleaseInternalWithToTrack(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier);
			static void DeleteRelations(std::vector<DataModel::Relation*>& relations);
			bool AssignRelations(std::vector<DataModel::Relation*>& relations, const std::vector<DataModel::Relation*>& newRelations);

			Manager* manager;
			std::mutex updateMutex;
			bool executeAtUnlock;

			Delay delay;
			std::vector<DataModel::Relation*> relationsAtLock;
			std::vector<DataModel::Relation*> relationsAtUnlock;
			std::vector<DataModel::Relation*> relationsConditions;
			PushpullType pushpull;
			Propulsion propulsion;
			TrainType trainType;
			Length minTrainLength;
			Length maxTrainLength;
			Automode automode;
			TrackID fromTrack;
			Orientation fromOrientation;
			TrackID toTrack;
			Orientation toOrientation;

			Speed speed;
			FeedbackID feedbackIdReduced;
			Delay reducedDelay;
			FeedbackID feedbackIdCreep;
			Delay creepDelay;
			FeedbackID feedbackIdStop;
			Delay stopDelay;
			FeedbackID feedbackIdOver;
			Pause waitAfterRelease;
			RouteID followUpRoute;

			time_t lastUsed;
			unsigned int counter;
	};
} // namespace DataModel

