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
#include "Logger/Logger.h"
#include "DataModel/HardwareHandle.h"
#include "DataModel/LocoFunctions.h"
#include "DataModel/Object.h"
#include "DataModel/Relation.h"
#include "DataModel/Route.h"
#include "Utils/ThreadSafeQueue.h"

class Manager;

namespace Hardware
{
		class LocoCacheEntry;
}

namespace DataModel
{
	class ObjectIdentifier;
	class Route;
	class Track;

	class LocoBase : public Object, public HardwareHandle
	{
		public:
			enum NrOfTracksToReserve : unsigned char
			{
				ReserveOne = 1,
				ReserveTwo = 2
			};

			enum AutoModeType : unsigned char
			{
				AutoModeTypeFull = 0,
				AutoModeTypeTimetable
			};

			LocoBase() = delete;
			LocoBase(const LocoBase&) = delete;
			LocoBase& operator=(const LocoBase&) = delete;

			inline LocoBase(Manager* manager, const LocoID locoID)
			:	Object(locoID),
				HardwareHandle(),
				manager(manager),
				length(0),
				pushpull(false),
				maxSpeed(0),
				travelSpeed(0),
				reducedSpeed(0),
				creepingSpeed(0),
				propulsion(PropulsionUnknown),
				trainType(TrainTypeUnknown),
				speed(MinSpeed),
				orientation(OrientationRight),
				state(LocoStateManual),
				requestManualMode(false),
				trackFrom(nullptr),
				trackFirst(nullptr),
				trackSecond(nullptr),
				routeFirst(nullptr),
				routeSecond(nullptr),
				feedbackIdFirstReduced(FeedbackNone),
				feedbackIdFirstCreep(FeedbackNone),
				feedbackIdFirst(FeedbackNone),
				feedbackIdReduced(FeedbackNone),
				feedbackIdCreep(FeedbackNone),
				feedbackIdStop(FeedbackNone),
				feedbackIdOver(FeedbackNone),
				feedbackIdsReached(),
				wait(0),
				followUpRoute(RouteAuto),
				matchKey("")
			{
				logger = Logger::Logger::GetLogger(GetName());
			}

			inline LocoBase(Manager* manager, const std::string& serialized)
			:	LocoBase(manager, LocoNone)
			{
				LocoBase::Deserialize(serialized);
				logger = Logger::Logger::GetLogger(GetName());
			}

			virtual ~LocoBase();

			inline Logger::Logger* GetLogger() const
			{
				return logger;
			}

			std::string Serialize() const override;

			bool Deserialize(const std::string& serialized) override;

			bool Deserialize(const std::map<std::string,std::string>& arguments) override;

			virtual void SetName(const std::string& name) override
			{
				Object::SetName(name);
				logger = Logger::Logger::GetLogger(name);
			}

			bool GoToAutoMode();

			void RequestManualMode();

			bool GoToManualMode();

			void AddTimeTable(const Route* route, const RouteID followUpRoute);

			bool SetTrack(const TrackID trackID);

			TrackID GetTrackId() const;

			bool Release();

			bool CheckFreeingTrack(const TrackID trackID) const;

			void LocationReached(const FeedbackID feedbackID);

			virtual void SetSpeed(const Speed speed);

			inline Speed GetSpeed() const
			{
				return speed;
			}

			virtual void SetFunctionState(const DataModel::LocoFunctionNr nr,
				const DataModel::LocoFunctionState state);

			inline DataModel::LocoFunctionState GetFunctionState(const DataModel::LocoFunctionNr nr) const
			{
				return functions.GetFunctionState(nr);
			}

			inline std::vector<DataModel::LocoFunctionEntry> GetFunctionStates() const
			{
				return functions.GetFunctionStates();
			}

			inline void GetFunctions(LocoFunctionEntry* out) const
			{
				functions.GetFunctions(out);
			}

			inline void ConfigureFunctions(const std::vector<LocoFunctionEntry>& newEntries)
			{
				functions.ConfigureFunctions(newEntries);
			}

			virtual void SetOrientation(const Orientation orientation);

			inline Orientation GetOrientation() const
			{
				return orientation;
			}

			inline bool IsInManualMode() const
			{
				return this->state == LocoStateManual;
			}

			inline bool IsInUse() const
			{
				return this->speed > 0
					|| this->state != LocoStateManual
					|| this->trackFrom != nullptr
					|| this->routeFirst != nullptr;
			}

			inline Length GetLength() const
			{
				return length;
			}

			inline void SetLength(const Length length)
			{
				this->length = length;
			}

			inline bool GetPushpull() const
			{
				return pushpull;
			}

			inline Speed GetMaxSpeed() const
			{
				return maxSpeed;
			}

			inline Speed GetTravelSpeed() const
			{
				return travelSpeed;
			}

			inline Speed GetReducedSpeed() const
			{
				return reducedSpeed;
			}

			inline Speed GetCreepingSpeed() const
			{
				return creepingSpeed;
			}

			inline void SetPushpull(const bool pushpull)
			{
				this->pushpull = pushpull;
			}

			inline void SetMaxSpeed(const Speed speed)
			{
				maxSpeed = speed;
			}

			inline void SetTravelSpeed(const Speed speed)
			{
				travelSpeed = speed;
			}

			inline void SetReducedSpeed(const Speed speed)
			{
				reducedSpeed = speed;
			}

			inline void SetCreepingSpeed(const Speed speed)
			{
				creepingSpeed = speed;
			}

			inline void SetPropulsion(const Propulsion propulsion)
			{
				this->propulsion = propulsion;
			}

			inline Propulsion GetPropulsion() const
			{
				return propulsion;
			}

			inline void SetTrainType(const TrainType trainType)
			{
				this->trainType = trainType;
			}

			inline TrainType GetTrainType() const
			{
				return trainType;
			}

			inline void SetMatchKey(const std::string& matchKey)
			{
				this->matchKey = matchKey;
			}

			inline void ClearMatchKey()
			{
				matchKey.clear();
			}

			inline std::string GetMatchKey() const
			{
				return matchKey;
			}

			DataModel::LocoFunctionNr GetFunctionNumberFromFunctionIcon(const DataModel::LocoFunctionIcon icon) const;

			DataModel::LocoFunctionIcon GetFunctionIcon(const DataModel::LocoFunctionNr nr) const
			{
				return functions.GetFunctionIcon(nr);
			}

			DataModel::LocoFunctionType GetFunctionType(const DataModel::LocoFunctionNr nr) const
			{
				return functions.GetFunctionType(nr);
			}

			inline LocoID GetLocoIdWithPrefix() const
			{
				return GetID() + (GetObjectType() == ObjectTypeMultipleUnit ? MultipleUnitIdPrefix : 0);
			}

			LocoBase& operator=(const Hardware::LocoCacheEntry& loco);

		protected:
			Manager* manager;

		private:
			typedef std::pair<RouteID,RouteID> TimeTableEntry;

			enum LocoState : unsigned char
			{
				LocoStateManual = 0,
				LocoStateTerminated,
				LocoStateOff,
				LocoStateAutomodeGetFirst,
				LocoStateAutomodeGetSecond,
				LocoStateAutomodeRunning,
				LocoStateStopping,
				LocoStateError
			};

			void SetMinThreadPriorityAndThreadName();

			void AutoMode();

			Route* GetNextDestination(const Track* const track, const bool allowLocoTurn);

			void GetTimetableDestinationFirst();

			void PrepareDestinationFirst(Route* const route);

			void GetTimetableDestinationSecond();

			DataModel::Route* GetDestinationFromTimeTable(const Track* const track, const bool allowLocoTurn);

			void PrepareDestinationSecond(Route* const route);

			DataModel::Route* SearchDestination(const DataModel::Track* const oldToTrack, const bool allowLocoTurn);

			bool ReserveRoute(const Track* const track, const bool allowLocoTurn, Route* const route);

			bool ExecuteRoute(const Track* const track, const bool allowLocoTurn, Route* const route);

			void FeedbackIdFirstReached();

			void FeedbackIdStopReached();

			void ForceManualMode();

			bool GoToAutoModeInternal(const LocoState newState);

			Speed GetRouteSpeed(const Route::Speed routeSpeed);

			mutable std::mutex stateMutex;
			std::thread locoThread;

			Length length;
			bool pushpull;
			Speed maxSpeed;
			Speed travelSpeed;
			Speed reducedSpeed;
			Speed creepingSpeed;

			Propulsion propulsion;
			TrainType trainType;

			Speed speed;
			Orientation orientation;

			volatile LocoState state;
			volatile bool requestManualMode;
			Track* trackFrom;
			Track* trackFirst;
			Track* trackSecond;
			Route* routeFirst;
			Route* routeSecond;
			volatile FeedbackID feedbackIdFirstReduced;
			volatile FeedbackID feedbackIdFirstCreep;
			volatile FeedbackID feedbackIdFirst;
			volatile FeedbackID feedbackIdReduced;
			volatile FeedbackID feedbackIdCreep;
			volatile FeedbackID feedbackIdStop;
			volatile FeedbackID feedbackIdOver;
			Utils::ThreadSafeQueue<FeedbackID> feedbackIdsReached;
			Pause wait;
			Utils::ThreadSafeQueue<TimeTableEntry> timeTableQueue;
			RouteID followUpRoute;
			std::string matchKey;

			LocoFunctions functions;

			Logger::Logger* logger;
	};
} // namespace DataModel
