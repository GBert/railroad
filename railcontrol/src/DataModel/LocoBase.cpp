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

#include <algorithm>
#include <map>
#include <string>

#include "DataModel/LocoBase.h"
#include "DataModel/Track.h"
#include "Hardware/LocoCache.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::string;
using std::to_string;
using std::vector;

namespace DataModel
{
	LocoBase::~LocoBase()
	{
		while (true)
		{
			{
				std::lock_guard<std::mutex> Guard(stateMutex);
				if (state == LocoStateManual)
				{
					break;
				}
			}
			logger->Info(Languages::TextWaitingUntilHasStopped, GetName());
			Utils::Utils::SleepForSeconds(1);
		}
	}

	std::string LocoBase::Serialize() const
	{
		string str;
		str += Object::Serialize();
		str += ";" + HardwareHandle::Serialize();
		str += ";functions=" + functions.Serialize();
		str += ";orientation=" + to_string(orientation);
		str += ";track=" + to_string(trackFrom ? trackFrom->GetID() : TrackNone);
		str += ";length=" + to_string(length);
		str += ";pushpull=" + to_string(pushpull);
		str += ";maxspeed=" + to_string(maxSpeed);
		str += ";travelspeed=" + to_string(travelSpeed);
		str += ";reducedspeed=" + to_string(reducedSpeed);
		str += ";creepingspeed=" + to_string(creepingSpeed);
		str += ";propulsion=" + to_string(propulsion);
		str += ";type=" + to_string(trainType);
		str += ";matchkey=" + matchKey;
		return str;
	}

	bool LocoBase::Deserialize(const std::string& serialized)
	{
		map<string, string> arguments;
		ParseArguments(serialized, arguments);
		return Deserialize(arguments);
	}

	bool LocoBase::Deserialize(const std::map<std::string,std::string>& arguments)
	{
		Object::Deserialize(arguments);
		HardwareHandle::Deserialize(arguments);
		trackFrom = manager->GetTrack(static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone)));
		functions.Deserialize(Utils::Utils::GetStringMapEntry(arguments, "functions", "0"));
		orientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "orientation", OrientationRight));
		length = static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "length", 0));
		pushpull = Utils::Utils::GetBoolMapEntry(arguments, "pushpull", false);
		maxSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "maxspeed", MaxSpeed);
		travelSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "travelspeed", DefaultTravelSpeed);
		reducedSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "reducedspeed", DefaultReducedSpeed);
		creepingSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "creepspeed", DefaultCreepingSpeed);
		creepingSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "creepingspeed", creepingSpeed);
		propulsion = static_cast<Propulsion>(Utils::Utils::GetIntegerMapEntry(arguments, "propulsion", PropulsionUnknown));
		trainType = static_cast<TrainType>(Utils::Utils::GetIntegerMapEntry(arguments, "type", TrainTypeUnknown));
		matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		return true;
	}

	bool LocoBase::GetPushpull() const
	{
		return pushpull;
	}

	Propulsion LocoBase::GetPropulsion() const
	{
		return propulsion;
	}

	bool LocoBase::SetTrack(const TrackID trackID)
	{
		std::lock_guard<std::mutex> Guard(stateMutex);
		// there must not be set a track
		if (this->trackFrom != nullptr)
		{
			return false;
		}
		this->trackFrom = manager->GetTrack(trackID);
		return true;
	}

	TrackID LocoBase::GetTrackId()
	{
		if (trackFrom)
		{
			return trackFrom->GetID();
		}
		return TrackNone;
	}

	bool LocoBase::Release()
	{
		manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
		ForceManualMode();
		std::lock_guard<std::mutex> Guard(stateMutex);

		if (routeFirst != nullptr)
		{
			routeFirst->Release(logger, GetObjectIdentifier());
			routeFirst = nullptr;
		}
		if (routeSecond != nullptr)
		{
			routeSecond->Release(logger, GetObjectIdentifier());
			routeSecond = nullptr;
		}
		if (trackFrom != nullptr)
		{
			trackFrom->Release(logger, GetObjectIdentifier());
			trackFrom = nullptr;
		}
		if (trackFirst != nullptr)
		{
			trackFirst->Release(logger, GetObjectIdentifier());
			trackFirst = nullptr;
		}
		if (trackSecond != nullptr)
		{
			trackSecond->Release(logger, GetObjectIdentifier());
			trackSecond = nullptr;
		}
		feedbackIdOver = FeedbackNone;
		feedbackIdStop = FeedbackNone;
		feedbackIdCreep = FeedbackNone;
		feedbackIdReduced = FeedbackNone;
		feedbackIdFirst = FeedbackNone;
		return true;
	}

	bool LocoBase::IsRunningFromTrack(const TrackID trackID) const
	{
		std::lock_guard<std::mutex> Guard(stateMutex);
		return trackFirst != nullptr && trackFrom != nullptr && trackFrom->GetID() == trackID;
	}

	bool LocoBase::GoToAutoMode(const AutoModeType type)
	{
		std::lock_guard<std::mutex> Guard(stateMutex);
		if (trackFrom == nullptr)
		{
			logger->Warning(Languages::TextCanNotStartNotOnTrack, GetName());
			return false;
		}
		if (state == LocoStateError)
		{
			logger->Warning(Languages::TextCanNotStartInErrorState, GetName());
			return false;
		}
		if (state == LocoStateTerminated)
		{
			locoThread.join();
			state = LocoStateManual;
		}
		if (state != LocoStateManual)
		{
			logger->Info(Languages::TextCanNotStartAlreadyRunning, GetName());
			return false;
		}

		state = (type == AutoModeTypeTimetable ? LocoStateTimetableGetFirst : LocoStateAutomodeGetFirst);
		locoThread = std::thread(&DataModel::LocoBase::AutoMode, this);

		return true;
	}

	void LocoBase::RequestManualMode()
	{
		if (state == LocoStateManual || state == LocoStateTerminated)
		{
			return;
		}
		requestManualMode = true;
	}

	bool LocoBase::GoToManualMode()
	{
		if (state == LocoStateManual)
		{
			return true;
		}
		if (state != LocoStateTerminated)
		{
			return false;
		}
		locoThread.join();
		state = LocoStateManual;
		return true;
	}

	void LocoBase::ForceManualMode()
	{
		{
			std::lock_guard<std::mutex> Guard(stateMutex);
			switch (state)
			{
				case LocoStateManual:
					return;

				case LocoStateTerminated:
					break;

				default:
					state = LocoStateOff;
					break;
			}
		}
		locoThread.join();
		state = LocoStateManual;
	}

	void LocoBase::AutoMode()
	{
		Utils::Utils::SetMinThreadPriority();
		const string& name = GetName();
		Utils::Utils::SetThreadName(name);
		logger->Info(Languages::TextIsNowInAutoMode, name);

		while (true)
		{
			{ // sleep must be outside of locked block
				std::lock_guard<std::mutex> Guard(stateMutex);
				if (feedbackIdsReached.IsEmpty() == false)
				{
					FeedbackID feedbackId = feedbackIdsReached.Dequeue();
					if (feedbackId == feedbackIdFirst)
					{
						FeedbackIdFirstReached();
					}
					else if (feedbackId == feedbackIdStop)
					{
						FeedbackIdStopReached();
					}
					continue;
				}

				switch (state)
				{
					case LocoStateOff:
						// automode is turned off, terminate thread
						logger->Info(Languages::TextIsNowInManualMode, name);
						state = LocoStateTerminated;
						requestManualMode = false;
						return;

					case LocoStateAutomodeGetFirst:
						if (requestManualMode)
						{
							state = LocoStateOff;
							break;
						}
						if (wait > 0)
						{
							--wait;
							break;
						}
						SearchDestinationFirst();
						break;

					case LocoStateAutomodeGetSecond:
						if (requestManualMode)
						{
							logger->Info(Languages::TextIsRunningWaitingUntilDestination, name);
							state = LocoStateStopping;
							break;
						}
						if (manager->GetNrOfTracksToReserve() <= 1)
						{
							break;
						}
						if (wait > 0)
						{
							break;
						}
						SearchDestinationSecond();
						break;

					case LocoStateTimetableGetFirst:
						if (requestManualMode)
						{
							state = LocoStateOff;
							break;
						}
						if (wait > 0)
						{
							--wait;
							break;
						}
						GetTimetableDestinationFirst();
						break;

					case LocoStateTimetableGetSecond:
						if (requestManualMode)
						{
							logger->Info(Languages::TextIsRunningWaitingUntilDestination, name);
							state = LocoStateStopping;
							break;
						}
						if (manager->GetNrOfTracksToReserve() <= 1)
						{
							break;
						}
						if (wait > 0)
						{
							break;
						}
						GetTimetableDestinationSecond();
						break;

					case LocoStateAutomodeRunning:
					case LocoStateTimetableRunning:
						// loco is already running, waiting until destination reached
						if (requestManualMode)
						{
							logger->Info(Languages::TextIsRunningWaitingUntilDestination, name);
							state = LocoStateStopping;
						}
						break;

					case LocoStateStopping:
						logger->Info(Languages::TextHasNotReachedDestination, name);

						break;

					case LocoStateTerminated:
						logger->Error(Languages::TextIsInTerminatedState, name);
						state = LocoStateError;
						break;

					case LocoStateManual:
						logger->Error(Languages::TextIsInManualState, name);
						state = LocoStateError;
						#include "Fallthrough.h"

					case LocoStateError:
						logger->Error(Languages::TextIsInErrorState, name);
						manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
						if (requestManualMode)
						{
							state = LocoStateOff;
						}
						break;
				}
			}
			// FIXME: make configurable
			Utils::Utils::SleepForSeconds(1);
		}
	}

	void LocoBase::SearchDestinationFirst()
	{
		if (routeFirst != nullptr)
		{
			state = LocoStateError;
			logger->Error(Languages::TextHasAlreadyReservedRoute, GetName());
			return;
		}

		Route* route = SearchDestination(trackFrom, true);
		PrepareDestinationFirst(route, LocoStateAutomodeGetSecond);
	}

	void LocoBase::GetTimetableDestinationFirst()
	{
		if (routeFirst != nullptr)
		{
			state = LocoStateError;
			logger->Error(Languages::TextHasAlreadyReservedRoute, GetName());
			return;
		}

		Route* const route = GetDestinationFromTimeTable(trackFrom, true);
		if (route == nullptr)
		{
			logger->Debug(Languages::TextNoValidTimetableEntryFound, GetName());
			return;
		}
		PrepareDestinationFirst(route, LocoStateTimetableGetSecond);
	}

	void LocoBase::PrepareDestinationFirst(Route* const route, const LocoState newState)
	{
		if (route == nullptr)
		{
			return;
		}

		Track* newTrack = manager->GetTrack(route->GetToTrack());
		if (newTrack == nullptr)
		{
			return;
		}

		bool isOrientationSet = newTrack->SetLocoOrientation(static_cast<Orientation>(route->GetToOrientation()));
		if (isOrientationSet == false)
		{
			return;
		}

		bool turnLoco = (trackFrom->GetLocoOrientation() != route->GetFromOrientation());
		Orientation newLocoOrientation = static_cast<Orientation>(orientation != turnLoco);
		if (turnLoco)
		{
			bool canTurnOrientation = trackFrom->SetLocoOrientation(route->GetFromOrientation());
			if (canTurnOrientation == false)
			{
				return;
			}
			manager->TrackPublishState(trackFrom);
		}
		manager->LocoBaseOrientation(ControlTypeInternal, this, newLocoOrientation);
		logger->Info(Languages::TextHeadingToVia, newTrack->GetName(), route->GetName());

		trackFirst = newTrack;
		routeFirst = route;
		feedbackIdFirst = FeedbackNone;
		feedbackIdReduced = routeFirst->GetFeedbackIdReduced();
		feedbackIdCreep = routeFirst->GetFeedbackIdCreep();
		feedbackIdStop = routeFirst->GetFeedbackIdStop();
		feedbackIdOver = routeFirst->GetFeedbackIdOver();
		wait = routeFirst->GetWaitAfterRelease();

		// start loco
		manager->TrackPublishState(newTrack);
		Speed newSpeed;
		switch (routeFirst->GetSpeed())
		{
			case Route::SpeedTravel:
				newSpeed = travelSpeed;
				break;

			case Route::SpeedReduced:
				newSpeed = reducedSpeed;
				break;

			case Route::SpeedCreeping:
			default:
				newSpeed = creepingSpeed;
				break;
		}
		manager->LocoBaseSpeed(ControlTypeInternal, this, newSpeed);
		state = newState;
	}

	void LocoBase::SearchDestinationSecond()
	{
		Route* route = SearchDestination(trackFirst, false);
		PrepareDestinationSecond(route, LocoStateAutomodeRunning);
	}

	void LocoBase::GetTimetableDestinationSecond()
	{
		Route* route = GetDestinationFromTimeTable(trackFirst, false);
		if (route == nullptr)
		{
			return;
		}
		PrepareDestinationSecond(route, LocoStateTimetableRunning);
	}

	Route* LocoBase::GetDestinationFromTimeTable(const Track* const track, const bool allowLocoTurn)
	{
		if (timeTableQueue.IsEmpty())
		{
			return nullptr;
		}

		RouteID routeId = timeTableQueue.Dequeue();
		Route* const route = manager->GetRoute(routeId);
		if (route->GetFromTrack() != track->GetID())
		{
			return nullptr;
		}
		bool ret = ReserveRoute(track, allowLocoTurn, route);
		if (!ret)
		{
			return nullptr;
		}
		logger->Debug(Languages::TextUsingRouteFromTimetable, route->GetID());
		return route;
	}

	bool LocoBase::AddTimeTable(const ObjectIdentifier& identifier)
	{
		switch (identifier.GetObjectType())
		{
			case ObjectTypeRoute:
				timeTableQueue.Enqueue(identifier.GetObjectID());
				logger->Debug(Languages::TextAddingRouteToTimetable, identifier.GetObjectID());
				return true;

			case ObjectTypeTimeTable:
				// FIXME: TimeTable function is not yet implemented
				return false;

			default:
				return false;
		}
	}

	void LocoBase::PrepareDestinationSecond(Route* const route, const LocoState newState)
	{
		if (route == nullptr)
		{
			return;
		}

		Track* newTrack = manager->GetTrack(route->GetToTrack());
		if (newTrack == nullptr)
		{
			return;
		}

		const bool isOrientationSet = newTrack->SetLocoOrientation(static_cast<Orientation>(route->GetToOrientation()));
		if (isOrientationSet == false)
		{
			return;
		}
		logger->Info(Languages::TextHeadingToViaVia, newTrack->GetName(), routeFirst->GetName(), route->GetName());

		trackSecond = newTrack;
		routeSecond = route;
		feedbackIdFirst = feedbackIdStop;
		feedbackIdFirstCreep = feedbackIdCreep;
		feedbackIdFirstReduced = feedbackIdReduced;
		feedbackIdOver = routeSecond->GetFeedbackIdOver();
		feedbackIdStop = routeSecond->GetFeedbackIdStop();
		feedbackIdCreep = routeSecond->GetFeedbackIdCreep();
		feedbackIdReduced = routeSecond->GetFeedbackIdReduced();

		wait = routeSecond->GetWaitAfterRelease();

		manager->TrackPublishState(newTrack);
		state = newState;
	}

	Route* LocoBase::SearchDestination(const Track* const track, const bool allowLocoTurn)
	{
		if (manager->Booster() == BoosterStateStop)
		{
			return nullptr;
		}
		logger->Debug(Languages::TextLookingForDestination, track->GetName());
		if (routeSecond != nullptr)
		{
			state = LocoStateError;
			logger->Error(Languages::TextHasAlreadyReservedRoute, GetName());
			return nullptr;
		}

		if (track == nullptr)
		{
			state = LocoStateOff;
			logger->Info(Languages::TextIsNotOnTrack, GetName());
			return nullptr;
		}

		const ObjectIdentifier locoBaseOfTrack = track->GetLocoBase();
		if (locoBaseOfTrack != GetObjectIdentifier())
		{
			state = LocoStateError;
			logger->Error(Languages::TextIsOnOcupiedTrack, GetName(), track->GetName(), manager->GetLocoBaseName(locoBaseOfTrack));
			return nullptr;
		}

		vector<Route*> validRoutes;
		track->GetValidRoutes(logger, this, allowLocoTurn, validRoutes);
		for (auto route : validRoutes)
		{
			bool ret = ReserveRoute(track, allowLocoTurn, route);
			if (ret)
			{
				return route;
			}
		}
		logger->Debug(Languages::TextNoValidRouteFound, GetName());
		return nullptr;
	}

	bool LocoBase::ReserveRoute(const Track* const track, const bool allowLocoTurn, Route* const route)
	{
		logger->Debug(Languages::TextExecutingRoute, route->GetName());

		const ObjectIdentifier locoBaseIdentifier = GetObjectIdentifier();
		if (route->Reserve(logger, locoBaseIdentifier) == false)
		{
			return false;
		}

		if (route->Lock(logger, locoBaseIdentifier) == false)
		{
			route->Release(logger, locoBaseIdentifier);
			return false;
		}

		Track* newTrack = manager->GetTrack(route->GetToTrack());

		if (newTrack == nullptr)
		{
			route->Release(logger, locoBaseIdentifier);
			return false;
		}

		bool canSetOrientation = newTrack->CanSetLocoBaseOrientation(route->GetToOrientation(), locoBaseIdentifier);
		if (canSetOrientation == false)
		{
			route->Release(logger, locoBaseIdentifier);
			newTrack->Release(logger, locoBaseIdentifier);
			return false;
		}

		if (!allowLocoTurn && track->GetLocoOrientation() != route->GetFromOrientation())
		{
			route->Release(logger, locoBaseIdentifier);
			newTrack->Release(logger, locoBaseIdentifier);
			return false;
		}

		if (route->Execute(logger, locoBaseIdentifier) == false)
		{
			route->Release(logger, locoBaseIdentifier);
			newTrack->Release(logger, locoBaseIdentifier);
			return false;
		}
		return true;
	}

	Speed LocoBase::GetRouteSpeed(const Route::Speed routeSpeed)
	{
		switch (routeSpeed)
		{
			case Route::SpeedTravel:
				return travelSpeed;

			case Route::SpeedReduced:
				return reducedSpeed;

			case Route::SpeedCreeping:
				return creepingSpeed;

			default:
				return MinSpeed;
		}
	}

	void LocoBase::LocationReached(const FeedbackID feedbackID)
	{
		if (feedbackID == feedbackIdOver)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
			manager->Booster(ControlTypeInternal, BoosterStateStop);
			logger->Error(Languages::TextHitOverrun, GetName(), manager->GetFeedbackName(feedbackID));
			return;
		}

		if (feedbackID == feedbackIdStop)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
			if (feedbackIdFirst != 0)
			{
				feedbackIdsReached.Enqueue(feedbackIdFirst);
			}
			feedbackIdsReached.Enqueue(feedbackIdStop);
			return;
		}

		if (feedbackID == feedbackIdCreep)
		{
			if (speed > creepingSpeed)
			{
				manager->LocoBaseSpeed(ControlTypeInternal, this, creepingSpeed);
			}
			if (feedbackIdFirst != 0)
			{
				feedbackIdsReached.Enqueue(feedbackIdFirst);
			}
			return;
		}

		if (feedbackID == feedbackIdReduced)
		{
			if (speed > reducedSpeed)
			{
				manager->LocoBaseSpeed(ControlTypeInternal, this, reducedSpeed);
			}
			if (feedbackIdFirst != 0)
			{
				feedbackIdsReached.Enqueue(feedbackIdFirst);
			}
			return;
		}

		if (feedbackID == feedbackIdFirst)
		{
			Speed newSpeed = GetRouteSpeed(routeSecond->GetSpeed());
			if (speed > newSpeed)
			{
				manager->LocoBaseSpeed(ControlTypeInternal, this, newSpeed);
			}
			feedbackIdsReached.Enqueue(feedbackIdFirst);
			return;
		}

		if (feedbackID == feedbackIdFirstCreep)
		{
			Route::Speed routeSpeed = routeSecond->GetSpeed();
			switch (routeSpeed)
			{
				case Route::SpeedCreeping:
					if (speed > creepingSpeed)
					{
						manager->LocoBaseSpeed(ControlTypeInternal, this, creepingSpeed);
					}
					break;

				default:
					break;
			}
			return;
		}

		if (feedbackID == feedbackIdFirstReduced)
		{
			Route::Speed routeSpeed = routeSecond->GetSpeed();
			switch (routeSpeed)
			{
				case Route::SpeedReduced:
				case Route::SpeedCreeping:
					if (speed > reducedSpeed)
					{
						manager->LocoBaseSpeed(ControlTypeInternal, this, reducedSpeed);
					}
					break;

				default:
					break;
			}
			return;
		}
	}

	void LocoBase::SetSpeed(const Speed speed)
	{
		this->speed = speed;
	}

	void LocoBase::SetFunctionState(const DataModel::LocoFunctionNr nr,
		const DataModel::LocoFunctionState state)
	{
		functions.SetFunctionState(nr, state);
	}

	void LocoBase::SetOrientation(const Orientation orientation)
	{
		this->orientation = orientation;
	}

	void LocoBase::FeedbackIdFirstReached()
	{
		if (routeFirst == nullptr || trackFrom == nullptr)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
			state = LocoStateError;
			logger->Error(Languages::TextIsInAutomodeWithoutRouteTrack, GetName());
			return;
		}

		Speed newSpeed = GetRouteSpeed(routeSecond->GetSpeed());
		if (speed != newSpeed)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, newSpeed);
		}


		routeFirst->Release(logger, GetObjectIdentifier());
		routeFirst = routeSecond;
		routeSecond = nullptr;

		trackFrom->Release(logger, GetObjectIdentifier());
		trackFrom = trackFirst;
		trackFirst = trackSecond;
		trackSecond = nullptr;

		// set state
		switch (state)
		{
			case LocoStateAutomodeRunning:
				state = LocoStateAutomodeGetSecond;
				break;

			case LocoStateTimetableRunning:
				state = LocoStateTimetableGetSecond;
				break;

			case LocoStateStopping:
				// do nothing
				break;

			default:
				logger->Error(Languages::TextIsInInvalidAutomodeState, GetName(), state, manager->GetFeedbackName(feedbackIdFirst));
				state = LocoStateError;
				break;
		}

		feedbackIdFirstCreep = FeedbackNone;
		feedbackIdFirstReduced = FeedbackNone;
		feedbackIdFirst = FeedbackNone;
	}

	void LocoBase::FeedbackIdStopReached()
	{
		if (routeFirst == nullptr || trackFrom == nullptr)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
			state = LocoStateError;
			logger->Error(Languages::TextIsInAutomodeWithoutRouteTrack, GetName());
			return;
		}

		// FIXME: This is needed if FeedbackIdFirst is not hit
		manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);

		manager->LocoDestinationReached(this, routeFirst, trackFrom);
		routeFirst->Release(logger, GetObjectIdentifier());
		routeFirst = nullptr;

		trackFrom->Release(logger, GetObjectIdentifier());
		trackFrom = trackFirst;
		trackFirst = nullptr;
		logger->Info(Languages::TextReachedItsDestination, GetName());

		// set state
		switch (state)
		{
			case LocoStateAutomodeGetSecond:
				state = LocoStateAutomodeGetFirst;
				break;

			case LocoStateTimetableGetSecond:
				state = LocoStateTimetableGetFirst;
				break;

			case LocoStateStopping:
				state = LocoStateOff;
				break;

			default:
				logger->Error(Languages::TextIsInInvalidAutomodeState, GetName(), state, manager->GetFeedbackName(feedbackIdStop));
				state = LocoStateError;
				break;
		}

		feedbackIdStop = FeedbackNone;
		feedbackIdCreep = FeedbackNone;
		feedbackIdReduced = FeedbackNone;
	}

	DataModel::LocoFunctionNr LocoBase::GetFunctionNumberFromFunctionIcon(DataModel::LocoFunctionIcon icon) const
	{
		for (DataModel::LocoFunctionNr nr = 0; nr < NumberOfLocoFunctions; ++nr)
		{
			if (icon == functions.GetFunctionIcon(nr))
			{
				return nr;
			}
		}
		return NumberOfLocoFunctions;
	}

	LocoBase& LocoBase::operator=(const Hardware::LocoCacheEntry& loco)
	{
		SetControlID(loco.GetControlID());
		SetAddress(loco.GetAddress());
		SetProtocol(loco.GetProtocol());
		SetName(loco.GetName());
		SetMatchKey(loco.GetMatchKey());
		ConfigureFunctions(loco.GetFunctionStates());
		return *this;
	}
} // namespace DataModel
