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
		return LocoBase::Deserialize(arguments);
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

	bool LocoBase::SetTrack(const TrackID trackID)
	{
		std::lock_guard<std::mutex> Guard(stateMutex);
		// there must not be set a track
		if (trackFrom)
		{
			return false;
		}
		trackFrom = manager->GetTrack(trackID);
		return true;
	}

	TrackID LocoBase::GetTrackId() const
	{
		return (trackFrom ? trackFrom->GetID() : TrackNone);
	}

	bool LocoBase::Release()
	{
		manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
		ForceManualMode();
		{
			std::lock_guard<std::mutex> Guard(stateMutex);

			if (routeFirst)
			{
				Route* removeRoute = routeFirst;
				routeFirst = nullptr;
				releaseRouteQueue.EnqueueBack(removeRoute);
			}
			if (routeSecond)
			{
				Route* removeRoute = routeSecond;
				routeSecond = nullptr;
				releaseRouteQueue.EnqueueBack(removeRoute);
			}
			if (trackFrom)
			{
				Track* removeTrack = trackFrom;
				trackFrom = nullptr;
				releaseTrackQueue.EnqueueBack(removeTrack);
			}
			if (trackFirst)
			{
				Track* removeTrack = trackFirst;
				trackFirst = nullptr;
				releaseTrackQueue.EnqueueBack(removeTrack);
			}
			if (trackSecond)
			{
				Track* removeTrack = trackSecond;
				trackSecond = nullptr;
				releaseTrackQueue.EnqueueBack(removeTrack);
			}
			feedbackIdOver = FeedbackNone;
			feedbackIdStop = FeedbackNone;
			feedbackIdCreep = FeedbackNone;
			feedbackIdReduced = FeedbackNone;
			feedbackIdFirst = FeedbackNone;
		}
		ReleaseRouteAndTrack();
		return true;
	}

	void LocoBase::ReleaseRouteAndTrack()
	{
		const ObjectIdentifier identifier = GetObjectIdentifier();
		while(!releaseTrackQueue.IsEmpty())
		{
			Track* track = releaseTrackQueue.Dequeue();
			track->Release(logger, identifier);
		}
		while(!releaseRouteQueue.IsEmpty())
		{
			Route* route = releaseRouteQueue.Dequeue();
			route->Release(logger, identifier);
		}
	}

	bool LocoBase::CheckFreeingTrack(const TrackID trackID) const
	{
		std::lock_guard<std::mutex> Guard(stateMutex);
		if (trackFrom && (trackFrom->GetID() == trackID) && !trackFirst)
		{
			return false;
		}
		if (trackFirst && (trackFirst->GetID() == trackID))
		{
			return false;
		}
		if (trackSecond && (trackSecond->GetID() == trackID))
		{
			return false;
		}
		return true;
	}

	bool LocoBase::GoToAutoMode()
	{
		std::lock_guard<std::mutex> Guard(stateMutex);
		if (!trackFrom)
		{
			logger->Warning(Languages::TextCanNotStartNotOnTrack);
			return false;
		}
		if (state == LocoStateError)
		{
			logger->Warning(Languages::TextCanNotStartInErrorState);
			return false;
		}
		if (state == LocoStateTerminated)
		{
			locoThread.join();
			state = LocoStateManual;
		}
		if (state != LocoStateManual)
		{
			logger->Info(Languages::TextCanNotStartAlreadyRunning);
			return false;
		}

		followUpRoute = RouteAuto;
		state = LocoStateAutomodeGetFirst;
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
		logger->Info(Languages::TextIsNowInAutoMode);

		while (true)
		{
			unsigned char pause = 0;
			{ // sleep must be outside of locked block
				std::lock_guard<std::mutex> Guard(stateMutex);

				if (!feedbackIdsReached.IsEmpty())
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
				}
				else
				{
					switch (state)
					{
						case LocoStateOff:
							// automode is turned off, terminate thread
							logger->Info(Languages::TextIsNowInManualMode);
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
							GetTimetableDestinationFirst();
							break;

						case LocoStateAutomodeGetSecond:
							if (requestManualMode)
							{
								logger->Info(Languages::TextIsRunningWaitingUntilDestination);
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
							// loco is already running, waiting until destination reached
							if (requestManualMode)
							{
								logger->Info(Languages::TextIsRunningWaitingUntilDestination);
								state = LocoStateStopping;
							}
							break;

						case LocoStateStopping:
							if (requestManualMode)
							{
								logger->Info(Languages::TextHasNotReachedDestination);
								break;
							}

							if (trackSecond)
							{
								state = LocoStateAutomodeRunning;
							}
							else if (trackFirst)
							{
								state = LocoStateAutomodeGetSecond;
							}
							else
							{
								state = LocoStateAutomodeGetFirst;
							}
							break;

						case LocoStateTerminated:
							logger->Error(Languages::TextIsInTerminatedState);
							state = LocoStateError;
							break;

						case LocoStateManual:
							logger->Error(Languages::TextIsInManualState);
							state = LocoStateError;
							#include "Fallthrough.h"

						case LocoStateError:
							logger->Error(Languages::TextIsInErrorState);
							manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
							if (requestManualMode)
							{
								state = LocoStateOff;
							}
							break;
					}
					pause = 1;
				}
			}

			ReleaseRouteAndTrack();

			// FIXME: make configurable
			Utils::Utils::SleepForSeconds(pause);
		}
	}

	Route* LocoBase::GetNextDestination(const Track* const track, const bool allowLocoTurn)
	{
		if (timeTableQueue.IsEmpty())
		{
			if (followUpRoute == RouteStop)
			{
				RequestManualMode();
				return nullptr;
			}

			const Route* route = nullptr;

			if (followUpRoute != RouteAuto)
			{
				route = manager->GetRoute(followUpRoute);
			}
			else
			{
				route = SearchDestination(track, allowLocoTurn);
			}

			if (route)
			{
				AddTimeTable(route, route->GetFollowUpRoute());
			}
		}

		return GetDestinationFromTimeTable(track, allowLocoTurn);
	}

	void LocoBase::GetTimetableDestinationFirst()
	{
		if (routeFirst)
		{
			state = LocoStateError;
			logger->Error(Languages::TextHasAlreadyReservedRoute);
			return;
		}

		Route* route = GetNextDestination(trackFrom, true);
		if (!route)
		{
			return;
		}
		PrepareDestinationFirst(route);
	}

	void LocoBase::GetTimetableDestinationSecond()
	{
		Route* route = GetNextDestination(trackFirst, false);
		if (!route)
		{
			return;
		}
		PrepareDestinationSecond(route);
	}

	void LocoBase::PrepareDestinationFirst(Route* const route)
	{
		if (!route)
		{
			return;
		}

		Track* newTrack = manager->GetTrack(route->GetToTrack());
		if (!newTrack)
		{
			return;
		}

		bool isOrientationSet = newTrack->SetLocoOrientation(static_cast<Orientation>(route->GetToOrientation()));
		if (!isOrientationSet)
		{
			return;
		}

		bool turnLoco = (trackFrom->GetLocoOrientation() != route->GetFromOrientation());
		Orientation newLocoOrientation = static_cast<Orientation>(orientation != turnLoco);
		if (turnLoco)
		{
			bool canTurnOrientation = trackFrom->SetLocoOrientation(route->GetFromOrientation());
			if (!canTurnOrientation)
			{
				return;
			}
			manager->TrackPublishState(trackFrom);
		}
		manager->LocoBaseOrientation(ControlTypeInternal, this, newLocoOrientation);
		logger->Info(Languages::TextHeadingToVia, newTrack->GetName(), route->GetName());

		trackFirst = newTrack;
		routeFirst = route;
		feedbackIdFirstReduced = FeedbackNone;
		feedbackIdFirstCreep = FeedbackNone;
		feedbackIdFirst = FeedbackNone;
		feedbackIdReduced = routeFirst->GetFeedbackIdReduced();
		reducedDelay = routeFirst->GetReducedDelay();
		feedbackIdCreep = routeFirst->GetFeedbackIdCreep();
		creepDelay = routeFirst->GetCreepDelay();
		feedbackIdStop = routeFirst->GetFeedbackIdStop();
		stopDelay = routeFirst->GetStopDelay();
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
		state = LocoStateAutomodeGetSecond;
	}

	Route* LocoBase::GetDestinationFromTimeTable(const Track* const track, const bool allowLocoTurn)
	{
		if (timeTableQueue.IsEmpty())
		{
			return nullptr;
		}

		const TimeTableEntry entry = timeTableQueue.Dequeue();
		Route* const route = manager->GetRoute(entry.first);
		if (route->GetFromTrack() != track->GetID())
		{
			return nullptr;
		}
		logger->Debug(Languages::TextUsingRouteFromTimetable, route->GetName());
		bool ret = ExecuteRoute(track, allowLocoTurn, route);
		if (!ret)
		{
			timeTableQueue.EnqueueFront(entry);
			return nullptr;
		}
		followUpRoute = entry.second;
		return route;
	}

	void LocoBase::AddTimeTable(const Route* route, const RouteID followUpRoute)
	{
		logger->Debug(Languages::TextAddingRouteToTimetable, route->GetName());
		requestManualMode = false;
		const TimeTableEntry entry(route->GetID(), followUpRoute);
		timeTableQueue.EnqueueBack(entry);
	}

	void LocoBase::PrepareDestinationSecond(Route* const route)
	{
		if (!route)
		{
			return;
		}

		Track* newTrack = manager->GetTrack(route->GetToTrack());
		if (!newTrack)
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
		state = LocoStateAutomodeRunning;
	}

	Route* LocoBase::SearchDestination(const Track* const track, const bool allowLocoTurn)
	{
		if (manager->Booster() == BoosterStateStop)
		{
			return nullptr;
		}
		logger->Debug(Languages::TextLookingForDestination, track->GetName());
		if (routeSecond)
		{
			state = LocoStateError;
			logger->Error(Languages::TextHasAlreadyReservedRoute);
			return nullptr;
		}

		if (!track)
		{
			state = LocoStateOff;
			logger->Info(Languages::TextIsNotOnTrack);
			return nullptr;
		}

		const ObjectIdentifier& locoBaseOfTrack = track->GetLocoBase();
		if (locoBaseOfTrack != GetObjectIdentifier())
		{
			state = LocoStateError;
			logger->Error(Languages::TextIsOnOcupiedTrack, track->GetName(), manager->GetLocoBaseName(locoBaseOfTrack));
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
		logger->Debug(Languages::TextReservingRoute, route->GetName());

		const ObjectIdentifier locoBaseIdentifier = GetObjectIdentifier();
		if (!route->Reserve(logger, locoBaseIdentifier))
		{
			return false;
		}

		Track* newTrack = manager->GetTrack(route->GetToTrack());
		if (!newTrack)
		{
			route->Release(logger, locoBaseIdentifier);
			return false;
		}

		bool canSetOrientation = newTrack->CanSetLocoBaseOrientation(route->GetToOrientation(), locoBaseIdentifier);
		if (!canSetOrientation)
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

		return true;
	}

	bool LocoBase::ExecuteRoute(const Track* const track, const bool allowLocoTurn, Route* const route)
	{
		if (route->GetLockState() == LockableItem::LockStateFree)
		{
			if (!ReserveRoute(track, allowLocoTurn, route))
			{
				return false;
			}
		}

		if (route->GetLockState() == LockableItem::LockStateReserved)
		{
			if (!route->Lock(logger, GetObjectIdentifier()))
			{
				return false;
			}
		}

		logger->Debug(Languages::TextExecutingRoute, route->GetName());

		const ObjectIdentifier& locoBaseIdentifier = GetObjectIdentifier();
		if (!route->Execute(logger, locoBaseIdentifier))
		{
			route->Release(logger, locoBaseIdentifier);

			Track* newTrack = manager->GetTrack(route->GetToTrack());
			if (newTrack)
			{
				newTrack->Release(logger, locoBaseIdentifier);
			}
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

	void LocoBase::LocationStopReached(const FeedbackID feedbackID, const Delay stopDelay)
	{
		Utils::Utils::SleepForMilliseconds(stopDelay * 100);
		if (feedbackID != feedbackIdStop)
		{
			return;
		}
		LocationStopReached();
	}

	void LocoBase::LocationStopReached()
	{
		manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
		feedbackIdsReached.EnqueueBack(feedbackIdStop);
	}

	void LocoBase::LocationCreepReached(const FeedbackID feedbackID, const Delay creepDelay)
	{
		Utils::Utils::SleepForMilliseconds(creepDelay * 100);
		if (feedbackID != feedbackIdCreep)
		{
			return;
		}
		LocationCreepReached();
	}

	void LocoBase::LocationCreepReached()
	{
		if (speed > creepingSpeed)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, creepingSpeed);
		}
		if (feedbackIdFirst != FeedbackNone)
		{
			feedbackIdsReached.EnqueueBack(feedbackIdFirst);
		}
	}

	void LocoBase::LocationReducedReached(const FeedbackID feedbackID, const Delay reducedDelay)
	{
		Utils::Utils::SleepForMilliseconds(reducedDelay * 100);
		if (feedbackID != feedbackIdReduced)
		{
			return;
		}
		LocationReducedReached();
	}

	void LocoBase::LocationReducedReached()
	{
		if (speed > reducedSpeed)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, reducedSpeed);
		}
		if (feedbackIdFirst != 0)
		{
			feedbackIdsReached.EnqueueBack(feedbackIdFirst);
		}
	}

	void LocoBase::LocationReached(const FeedbackID feedbackID)
	{
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
		}

		if (feedbackID == feedbackIdFirst)
		{
			Speed newSpeed = GetRouteSpeed(routeSecond->GetSpeed());
			if (speed > newSpeed)
			{
				manager->LocoBaseSpeed(ControlTypeInternal, this, newSpeed);
			}
			feedbackIdsReached.EnqueueBack(feedbackIdFirst);
		}

		if (feedbackID == feedbackIdReduced)
		{
			if (reducedDelay)
			{
				__attribute((unused)) auto r = std::async(std::launch::async, LocationReducedReachedStatic, this, feedbackID, reducedDelay);
			}
			else
			{
				LocationReducedReached();
			}
		}

		if (feedbackID == feedbackIdCreep)
		{
			if (creepDelay)
			{
				__attribute((unused)) auto r = std::async(std::launch::async, LocationCreepReachedStatic, this, feedbackID, creepDelay);
			}
			else
			{
				LocationCreepReached();
			}
		}

		if (feedbackID == feedbackIdStop)
		{
			if (stopDelay)
			{
				__attribute((unused)) auto r = std::async(std::launch::async, LocationStopReachedStatic, this, feedbackID, stopDelay);
			}
			else
			{
				LocationStopReached();
			}
		}

		if (feedbackID == feedbackIdOver)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
			manager->Booster(ControlTypeInternal, BoosterStateStop);
			logger->Error(Languages::TextHitOverrun, manager->GetFeedbackName(feedbackID));
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
		if (!routeFirst || !trackFrom)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
			state = LocoStateError;
			logger->Error(Languages::TextIsInAutomodeWithoutRouteTrack);
			return;
		}

		Speed newSpeed = GetRouteSpeed(routeSecond->GetSpeed());
		if (speed != newSpeed)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, newSpeed);
		}

		ShiftRoute();

		// set state
		switch (state)
		{
			case LocoStateAutomodeRunning:
				state = LocoStateAutomodeGetSecond;
				break;

			case LocoStateStopping:
				// do nothing
				break;

			default:
				logger->Error(Languages::TextIsInInvalidAutomodeState, state, manager->GetFeedbackName(feedbackIdFirst));
				state = LocoStateError;
				break;
		}

		feedbackIdFirstCreep = FeedbackNone;
		feedbackIdFirstReduced = FeedbackNone;
		feedbackIdFirst = FeedbackNone;
	}

	void LocoBase::FeedbackIdStopReached()
	{
		if (!routeFirst || !trackFrom)
		{
			manager->LocoBaseSpeed(ControlTypeInternal, this, MinSpeed);
			state = LocoStateError;
			logger->Error(Languages::TextIsInAutomodeWithoutRouteTrack);
			return;
		}

		if (routeSecond)
		{
			// this happens when FeedbackIdFirst has been missed
			manager->LocoDestinationReached(this, routeSecond, trackSecond);
		}
		else
		{
			manager->LocoDestinationReached(this, routeFirst, trackFirst);
		}
		logger->Info(Languages::TextReachedItsDestination);

		while (routeFirst)
		{
			ShiftRoute();
		}

		// set state
		switch (state)
		{
			case LocoStateAutomodeRunning:
			case LocoStateAutomodeGetSecond:
				state = LocoStateAutomodeGetFirst;
				break;

			case LocoStateStopping:
				state = LocoStateOff;
				break;

			default:
				logger->Error(Languages::TextIsInInvalidAutomodeState, state, manager->GetFeedbackName(feedbackIdStop));
				state = LocoStateError;
				break;
		}

		feedbackIdStop = FeedbackNone;
		feedbackIdCreep = FeedbackNone;
		feedbackIdReduced = FeedbackNone;
	}

	void LocoBase::ShiftRoute()
	{
		Route* removeRoute = routeFirst;
		routeFirst = routeSecond;
		routeSecond = nullptr;

		Track* removeTrack = trackFrom;
		trackFrom = trackFirst;
		trackFirst = trackSecond;
		trackSecond = nullptr;

		releaseRouteQueue.EnqueueBack(removeRoute);
		releaseTrackQueue.EnqueueBack(removeTrack);
	}

	DataModel::LocoFunctionNr LocoBase::GetFunctionNumberFromFunctionIcon(const DataModel::LocoFunctionIcon icon) const
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
