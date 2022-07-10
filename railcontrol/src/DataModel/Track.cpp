/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2022 Dominik (Teddy) Mahrer - www.railcontrol.org

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
#include <deque>
#include <map>
#include <string>

#include "DataModel/Feedback.h"
#include "DataModel/Track.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::deque;
using std::map;
using std::string;
using std::to_string;
using std::vector;

namespace DataModel
{
	std::string Track::Serialize() const
	{
		std::string str;
		str = "objectType=Track;";
		std::string feedbackString;
		for (auto feedback : feedbacks)
		{
			if (feedbackString.size() > 0)
			{
				feedbackString += ",";
			}
			feedbackString += std::to_string(feedback);
		}
		str += "feedbacks=";
		str += feedbackString;
		str += ";selectrouteapproach=";
		str += to_string(selectRouteApproach);
		str += ";trackstate=";
		str += to_string(trackState);
		str += ";trackstatedelayed=";
		str += to_string(trackStateDelayed);
		str += ";locoorientation=";
		str += to_string(locoOrientation);
		str += ";blocked=";
		str += to_string(blocked);
		str += ";locodelayed=";
		str += to_string(locoIdDelayed);
		str += ";allowlocoturn=";
		str += to_string(allowLocoTurn);
		str += ";releasewhenfree=";
		str += to_string(releaseWhenFree);
		str += ";showname=";
		str += to_string(showName);
		str += ";";
		str += LayoutItem::Serialize();
		str += ";";
		str += LockableItem::Serialize();
		str += ";tracktype=";
		str += to_string(trackType);
		return str;
	}

	bool Track::Deserialize(const std::string& serialized)
	{
		map<string, string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Track") != 0)
		{
			return false;
		}
		LayoutItem::Deserialize(arguments);
		LockableItem::Deserialize(arguments);
		string feedbackStrings = Utils::Utils::GetStringMapEntry(arguments, "feedbacks");
		deque<string> feedbackStringVector;
		Utils::Utils::SplitString(feedbackStrings, ",", feedbackStringVector);
		for (auto& feedbackString : feedbackStringVector)
		{
			FeedbackID feedbackID = Utils::Utils::StringToInteger(feedbackString);
			if (!manager->FeedbackExists(feedbackID))
			{
				continue;
			}
			feedbacks.push_back(feedbackID);
		}
		selectRouteApproach = static_cast<SelectRouteApproach>(Utils::Utils::GetIntegerMapEntry(arguments, "selectrouteapproach", SelectRouteSystemDefault));
		trackState = static_cast<DataModel::Feedback::FeedbackState>(Utils::Utils::GetBoolMapEntry(arguments, "trackstate", DataModel::Feedback::FeedbackStateFree));
		trackStateDelayed = static_cast<DataModel::Feedback::FeedbackState>(Utils::Utils::GetBoolMapEntry(arguments, "trackstatedelayed", trackState));
		locoOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "locoorientation", OrientationRight));
		blocked = Utils::Utils::GetBoolMapEntry(arguments, "blocked", false);
		locoIdDelayed = static_cast<LocoID>(Utils::Utils::GetIntegerMapEntry(arguments, "locodelayed", GetLoco()));
		allowLocoTurn = Utils::Utils::GetBoolMapEntry(arguments, "allowlocoturn", true);
		releaseWhenFree = Utils::Utils::GetBoolMapEntry(arguments, "releasewhenfree", false);
		showName = Utils::Utils::GetBoolMapEntry(arguments, "showname", true);
		SetWidth(Width1);
		SetVisible(VisibleYes);
		trackType = static_cast<TrackType>(Utils::Utils::GetIntegerMapEntry(arguments, "tracktype", TrackTypeStraight));
		switch (trackType)
		{
			case TrackTypeTurn:
			case TrackTypeTunnelEnd:
				SetHeight(Height1);
				break;

			case TrackTypeCrossingLeft:
			case TrackTypeCrossingRight:
			case TrackTypeCrossingSymetric:
				SetHeight(Height2);
				break;

			default:
				break;
		}
		return true;
	}

	void Track::DeleteSignals()
	{
		while (signals.size() > 0)
		{
			Relation* signalRelation = signals.back();
			Signal* signal = dynamic_cast<Signal*>(signalRelation->GetObject2());
			if (signal != nullptr)
			{
				signal->SetTrack(nullptr);
			}
			signals.pop_back();
			delete signalRelation;
		}
	}

	void Track::DeleteSignal(Signal* signalToDelete)
	{
		for (unsigned int index = 0; index < signals.size(); ++index)
		{
			if (signals[index]->GetObject2() != signalToDelete)
			{
				continue;
			}
			delete signals[index];
			signals.erase(signals.begin() + index);
			signalToDelete->SetTrack(nullptr);
			return;
		}
	}

	void Track::AssignSignals(const std::vector<DataModel::Relation*>& newSignals)
	{
		DeleteSignals();
		signals = newSignals;
		for (auto signalRelation : signals)
		{
			Signal* signal = dynamic_cast<Signal*>(signalRelation->GetObject2());
			if (signal != nullptr)
			{
				signal->SetTrack(this);
			}
		}
	}

	void Track::StopAllSignals(const LocoID locoId)
	{
		for (auto signalRelation : signals)
		{
			Signal* signal = dynamic_cast<Signal*>(signalRelation->GetObject2());
			if (signal == nullptr)
			{
				continue;
			}
			LocoID locoOfSignal = signal->GetLoco();
			if (locoId != locoOfSignal && locoOfSignal != LocoNone)
			{
				continue;
			}
			manager->SignalState(ControlTypeInternal, signal, SignalStateStop, true);
		}
	}

	bool Track::SetLocoOrientation(const Orientation orientation)
	{
		if (locoOrientation == orientation)
		{
			return true;
		}
		locoOrientation = orientation;
		if (cluster == nullptr)
		{
			return true;
		}
		return cluster->SetLocoOrientation(orientation, GetLoco());
	}

	bool Track::Reserve(Logger::Logger* logger, const LocoID locoID)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (this->locoIdDelayed != LocoNone && this->locoIdDelayed != locoID)
		{
			logger->Debug(Languages::TextTrackIsUsedByLoco, GetName(), manager->GetLocoName(locoIdDelayed));
			return false;
		}
		if (blocked == true)
		{
			logger->Debug(Languages::TextTrackStatusIsBlocked, GetName());
			return false;
		}
		if (trackState != DataModel::Feedback::FeedbackStateFree)
		{
			logger->Debug(Languages::TextIsNotFree, GetName());
			return false;
		}
		return ReserveForce(logger, locoID);
	}

	bool Track::ReserveForce(Logger::Logger* logger, const LocoID locoID)
	{
		bool ret = LockableItem::Reserve(logger, locoID);
		if (ret == false)
		{
			return false;
		}
		this->locoIdDelayed = locoID;
		return true;
	}

	bool Track::Lock(Logger::Logger* logger, const LocoID locoID)
	{
		bool ret = LockableItem::Lock(logger, locoID);
		if (ret)
		{
			PublishState();
		}
		return ret;
	}

	bool Track::Release(Logger::Logger* logger, const LocoID locoID)
	{
		StopAllSignals(locoID);
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			bool ret = LockableItem::Release(logger, locoID);
			if (ret == false)
			{
				return false;
			}
			if (trackState != DataModel::Feedback::FeedbackStateFree)
			{
				return true;
			}
			this->locoIdDelayed = LocoNone;
			this->trackStateDelayed = DataModel::Feedback::FeedbackStateFree;
		}
		PublishState();
		return true;
	}

	bool Track::ReleaseForce(Logger::Logger* logger, const LocoID locoId)
	{
		StopAllSignals(locoId);
		bool ret;
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			ret = ReleaseForceUnlocked(logger, locoId);
		}
		PublishState();
		return ret;
	}

	bool Track::ReleaseForceUnlocked(Logger::Logger* logger, const LocoID locoID)
	{
		bool ret = LockableItem::Release(logger, locoID);
		this->trackState = DataModel::Feedback::FeedbackStateFree;
		this->locoIdDelayed = LocoNone;
		this->trackStateDelayed = DataModel::Feedback::FeedbackStateFree;
		return ret;
	}

	bool Track::SetFeedbackState(const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState newTrackState)
	{
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			DataModel::Feedback::FeedbackState oldTrackState = this->trackState;
			bool oldBlocked = blocked;
			bool ret = FeedbackStateInternal(feedbackID, newTrackState);
			if (ret == false)
			{
				return false;
			}
			if (oldTrackState == newTrackState && oldBlocked == blocked)
			{
				return true;
			}
		}
		PublishState();
		return true;
	}

	bool Track::FeedbackStateInternal(const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState newTrackState)
	{
		if (newTrackState == DataModel::Feedback::FeedbackStateOccupied)
		{
			Loco* loco = manager->GetLoco(GetLocoDelayed());
			if (loco == nullptr)
			{
				if (blocked == false && manager->GetStopOnFeedbackInFreeTrack())
				{
					manager->Booster(ControlTypeInternal, BoosterStateStop);
					blocked = true;
				}
			}
			else
			{
				loco->LocationReached(feedbackID);
			}

			this->trackState = newTrackState;
			this->trackStateDelayed = newTrackState;
			return true;
		}

		for (auto feedbackID : feedbacks)
		{
			DataModel::Feedback* feedback = manager->GetFeedbackUnlocked(feedbackID);
			if (feedback == nullptr)
			{
				continue;
			}
			if (feedback->GetState() != DataModel::Feedback::FeedbackStateFree)
			{
				return false;
			}
		}
		this->trackState = DataModel::Feedback::FeedbackStateFree;

		LocoID locoID = GetLoco();
		if (releaseWhenFree)
		{
			Loco* loco = manager->GetLoco(locoID);
			if (loco != nullptr && loco->IsRunningFromTrack(GetID()))
			{
				StopAllSignals(locoID);
				bool ret = ReleaseForceUnlocked(loco->GetLogger(), locoID);
				PublishState();
				return ret;
			}
		}

		if (locoID != LocoNone)
		{
			return true;
		}

		this->trackStateDelayed = DataModel::Feedback::FeedbackStateFree;
		this->locoIdDelayed = LocoNone;
		return true;
	}

	bool Track::AddRoute(Route* route)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		for (auto r : routes)
		{
			if (r == route)
			{
				return false;
			}
		}
		routes.push_back(route);
		return true;
	}

	bool Track::RemoveRoute(Route* route)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		size_t sizeBefore = routes.size();
		routes.erase(std::remove(routes.begin(), routes.end(), route), routes.end());
		size_t sizeAfter = routes.size();
		return sizeBefore > sizeAfter;
	}

	SelectRouteApproach Track::GetSelectRouteApproachCalculated() const
	{
		if (selectRouteApproach == SelectRouteSystemDefault)
		{
			return manager->GetSelectRouteApproach();
		}
		return selectRouteApproach;
	}

	bool Track::GetValidRoutes(Logger::Logger* logger,
		const Loco* loco,
		const bool allowLocoTurn,
		std::vector<Route*>& validRoutes) const
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		for (auto route : routes)
		{
			if (route->FromTrackOrientation(logger, GetID(), locoOrientation, loco, allowLocoTurn))
			{
				validRoutes.push_back(route);
			}
		}
		OrderValidRoutes(validRoutes);
		return true;
	}

	void Track::OrderValidRoutes(vector<Route*>& validRoutes) const
	{
		switch (GetSelectRouteApproachCalculated())
		{

			case SelectRouteRandom:
				std::random_shuffle(validRoutes.begin(), validRoutes.end());
				break;

			case SelectRouteMinTrackLength:
				std::sort(validRoutes.begin(), validRoutes.end(), Route::CompareShortest);
				break;

			case SelectRouteLongestUnused:
				std::sort(validRoutes.begin(), validRoutes.end(), Route::CompareLastUsed);
				break;

			case SelectRouteDoNotCare:
			default:
				// do nothing
				break;
		}
	}

	void Track::PublishState() const
	{
		manager->TrackPublishState(this);
	}
} // namespace DataModel
