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

#include <algorithm>
#include <deque>
#include <map>
#include <random>
#include <string>

#include "DataModel/Feedback.h"
#include "DataModel/Loco.h"
#include "DataModel/Track.h"
#include "Manager.h"
#include "Utils/Integer.h"

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
		str += LayoutItem::Serialize();
		str += ";";
		str += LockableItem::Serialize();
		str += ";selectrouteapproach=";
		str += to_string(selectRouteApproach);
		str += ";trackstate=";
		str += to_string(trackState);
		str += ";trackstatedelayed=";
		str += to_string(trackStateDelayed);
		str += ";locoorientation=";
		str += to_string(locoBaseOrientation);
		str += ";blocked=";
		str += to_string(blocked);
		str += ";locodelayed=";
		str += to_string(locoBaseDelayed.GetObjectID());
		str += ";locotypedelayed=";
		str += to_string(locoBaseDelayed.GetObjectType());
		str += ";allowlocoturn=";
		str += to_string(allowLocoTurn);
		str += ";releasewhenfree=";
		str += to_string(releaseWhenFree);
		str += ";showname=";
		str += to_string(showName);
		str += ";displayname=";
		str += displayName;
		str += ";tracktype=";
		str += to_string(trackType);
		str += ";main=";
		str += to_string(mainID);
		return str;
	}

	void Track::Deserialize(const std::string& serialized)
	{
		map<string, string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Track") != 0)
		{
			return;
		}
		LayoutItem::Deserialize(arguments);
		LockableItem::Deserialize(arguments);

		// FIXME: remove later: 2024-03-22 feedback vector has been replaced by relation
		string feedbackStrings = Utils::Utils::GetStringMapEntry(arguments, "feedbacks");
		deque<string> feedbackStringVector;
		Utils::Utils::SplitString(feedbackStrings, ",", feedbackStringVector);
		for (auto& feedbackString : feedbackStringVector)
		{
			const FeedbackID feedbackID = Utils::Integer::StringToInteger(feedbackString);
			Feedback* feedback = manager->GetFeedback(feedbackID);
			if (!feedback)
			{
				continue;
			}
			feedback->SetTrack(this);
			feedbacks.push_back(new Relation(manager,
				ObjectIdentifier(ObjectTypeTrack, GetID()),
				ObjectIdentifier(ObjectTypeFeedback, feedbackID),
				Relation::RelationTypeTrackFeedback));
		}

		selectRouteApproach = static_cast<SelectRouteApproach>(Utils::Utils::GetIntegerMapEntry(arguments, "selectrouteapproach", SelectRouteSystemDefault));
		trackState = static_cast<DataModel::Feedback::FeedbackState>(Utils::Utils::GetBoolMapEntry(arguments, "trackstate", DataModel::Feedback::FeedbackStateFree));
		trackStateDelayed = static_cast<DataModel::Feedback::FeedbackState>(Utils::Utils::GetBoolMapEntry(arguments, "trackstatedelayed", trackState));
		locoBaseOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "locoorientation", OrientationRight));
		blocked = Utils::Utils::GetBoolMapEntry(arguments, "blocked", false);
		const ObjectIdentifier& locoBaseDelayedIdentifier = GetLocoBase();
		locoBaseDelayed.SetObjectID(Utils::Utils::GetIntegerMapEntry(arguments, "locodelayed", locoBaseDelayedIdentifier.GetObjectID()));
		locoBaseDelayed.SetObjectType(static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "locotypedelayed", locoBaseDelayedIdentifier.GetObjectType())));
		allowLocoTurn = Utils::Utils::GetBoolMapEntry(arguments, "allowlocoturn", true);
		releaseWhenFree = Utils::Utils::GetBoolMapEntry(arguments, "releasewhenfree", false);
		showName = Utils::Utils::GetBoolMapEntry(arguments, "showname", true);
		displayName = Utils::Utils::GetStringMapEntry(arguments, "displayname", GetName());
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
		mainID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "master", TrackNone)); // FIXME: 2025-02-28 can be removed later
		mainID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "main", mainID));
	}

	void Track::DeleteFeedbacks()
	{
		while (feedbacks.size() > 0)
		{
			Relation* feedbackRelation = feedbacks.back();
			Feedback* feedback = manager->GetFeedback(feedbackRelation->ObjectID2());
			if (feedback)
			{
				feedback->SetTrack();
			}
			feedbacks.pop_back();
			delete feedbackRelation;
		}
	}

	void Track::AssignFeedbacks(const std::vector<DataModel::Relation*>& newFeedbacks)
	{
		DeleteFeedbacks();
		feedbacks = newFeedbacks;
		for (auto feedbackRelation : feedbacks)
		{
			Feedback* feedback = manager->GetFeedback(feedbackRelation->ObjectID2());
			if (feedback)
			{
				feedback->SetTrack(this);
			}
		}
	}

	void Track::DeleteSignals()
	{
		while (signals.size() > 0)
		{
			Relation* signalRelation = signals.back();
			Signal* signal = manager->GetSignal(signalRelation->ObjectID2());
			if (signal)
			{
				signal->SetTrack();
			}
			signals.pop_back();
			delete signalRelation;
		}
	}

	void Track::AssignSignals(const std::vector<DataModel::Relation*>& newSignals)
	{
		DeleteSignals();
		signals = newSignals;
		for (auto signalRelation : signals)
		{
			Signal* signal = manager->GetSignal(signalRelation->ObjectID2());
			if (signal)
			{
				signal->SetTrack(this);
			}
		}
	}

	void Track::StopAllSignals(const ObjectIdentifier& locoBaseIdentifier)
	{
		for (auto signalRelation : signals)
		{
			Signal* signal = manager->GetSignal(signalRelation->ObjectID2());
			if (!signal)
			{
				continue;
			}
			const ObjectIdentifier& locoBaseOfSignal = signal->GetLocoBase();
			if (locoBaseOfSignal.IsSet() && (locoBaseIdentifier != locoBaseOfSignal))
			{
				continue;
			}
			manager->SignalState(ControlTypeInternal, signal, SignalStateStop, true);
		}
	}

	bool Track::CanSetLocoBaseOrientation(const Orientation orientation, const ObjectIdentifier& locoBaseIdentifier)
	{
		if (locoBaseOrientation == orientation)
		{
			return true;
		}

		if (cluster)
		{
			return cluster->CanSetLocoBaseOrientation(static_cast<Orientation>(orientation ^ clusterInverted), locoBaseIdentifier);
		}

		const DataModel::LockableItem::LockState lockState = GetLockState();
		return ((lockState == DataModel::LockableItem::LockStateFree)
			|| ((lockState == DataModel::LockableItem::LockStateReserved)
				&& (!locoBaseIdentifier.IsSet()
					|| (locoBaseIdentifier == GetLocoBase()))));
	}

	bool Track::SetLocoBaseOrientation(const Orientation orientation)
	{
		if (locoBaseOrientation == orientation)
		{
			return true;
		}

		if (cluster)
		{
			return cluster->SetLocoBaseOrientation(static_cast<Orientation>(orientation ^ clusterInverted), GetLocoBase());
		}

		locoBaseOrientation = orientation;
		return true;
	}

	bool Track::Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (this->locoBaseDelayed.IsSet() && (this->locoBaseDelayed != locoBaseIdentifier))
		{
			logger->Debug(Languages::TextTrackIsUsedByLoco, GetName(), manager->GetLocoBaseName(locoBaseDelayed));
			return false;
		}
		if (blocked)
		{
			logger->Debug(Languages::TextTrackStatusIsBlocked, GetName());
			return false;
		}
		if (trackState != DataModel::Feedback::FeedbackStateFree)
		{
			logger->Debug(Languages::TextIsNotFree, GetName());
			return false;
		}
		return ReserveForce(logger, locoBaseIdentifier);
	}

	bool Track::ReserveForce(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		bool ret = LockableItem::Reserve(logger, locoBaseIdentifier);
		if (!ret)
		{
			return false;
		}

		this->locoBaseDelayed = locoBaseIdentifier;
		return true;
	}

	bool Track::Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		bool ret = LockableItem::Lock(logger, locoBaseIdentifier);
		if (ret)
		{
			PublishState();
		}

		return ret;
	}

	bool Track::Release(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		StopAllSignals(locoBaseIdentifier);
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			bool ret = LockableItem::Release(logger, locoBaseIdentifier);
			if (!ret)
			{
				return false;
			}

			if (trackState != DataModel::Feedback::FeedbackStateFree)
			{
				return true;
			}
			this->locoBaseDelayed.Clear();
			this->trackStateDelayed = DataModel::Feedback::FeedbackStateFree;
		}
		PublishState();
		return true;
	}

	bool Track::ReleaseForce(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		StopAllSignals(locoBaseIdentifier);
		bool ret;
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			ret = ReleaseForceUnlocked(logger, locoBaseIdentifier);
		}
		PublishState();
		return ret;
	}

	bool Track::ReleaseForceUnlocked(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		bool ret = LockableItem::Release(logger, locoBaseIdentifier);
		this->trackState = DataModel::Feedback::FeedbackStateFree;
		this->locoBaseDelayed.Clear();
		this->trackStateDelayed = DataModel::Feedback::FeedbackStateFree;
		return ret;
	}

	bool Track::SetFeedbackState(const FeedbackID feedbackID, const DataModel::Feedback::FeedbackState newTrackState)
	{
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			const DataModel::Feedback::FeedbackState oldTrackState = this->trackState;
			const bool oldBlocked = blocked;
			const bool ret = FeedbackStateInternal(feedbackID, newTrackState);
			if (!ret)
			{
				return false;
			}
			if ((oldTrackState == newTrackState) && (oldBlocked == blocked))
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
			const bool ok = manager->LocationReached(GetLocoBaseDelayed(), feedbackID);
			if (!ok)
			{
				if ((!blocked) && manager->GetStopOnFeedbackInFreeTrack())
				{
					manager->Booster(ControlTypeInternal, BoosterStateStop);
					blocked = true;
				}
			}

			this->trackState = DataModel::Feedback::FeedbackStateOccupied;
			this->trackStateDelayed = DataModel::Feedback::FeedbackStateOccupied;
			return true;
		}

		for (auto& feedbackRelation : feedbacks)
		{
			const DataModel::Feedback* feedback = manager->GetFeedbackUnlocked(feedbackRelation->ObjectID2());
			if (!feedback)
			{
				continue;
			}
			if (feedback->GetState() != DataModel::Feedback::FeedbackStateFree)
			{
				// if another feedback is still occupied
				return false;
			}
		}
		this->trackState = DataModel::Feedback::FeedbackStateFree;

		const ObjectIdentifier& locoBaseIdentifier = GetLocoBase();
		if (releaseWhenFree)
		{
			Logger::Logger* logger = manager->CheckFreeingTrack(locoBaseIdentifier, GetID());
			if (logger)
			{
				StopAllSignals(locoBaseIdentifier);
				const bool ret = ReleaseForceUnlocked(logger, locoBaseIdentifier);
				PublishState();
				return ret;
			}
		}

		if (locoBaseIdentifier.IsSet())
		{
			return true;
		}

		this->trackStateDelayed = DataModel::Feedback::FeedbackStateFree;
		this->locoBaseDelayed.Clear();
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

	bool Track::DeleteRoute(Route* route)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		size_t sizeBefore = routes.size();
		routes.erase(std::remove(routes.begin(), routes.end(), route), routes.end());
		size_t sizeAfter = routes.size();
		return sizeBefore > sizeAfter;
	}

	bool Track::AddExtension(Track* track)
	{
		if (mainID != TrackNone)
		{
			return false;
		}

		std::lock_guard<std::mutex> Guard(updateMutex);
		for (auto& extension : extensions)
		{
			if (extension == track)
			{
				return false;
			}
		}
		extensions.push_back(track);
		return true;
	}

	bool Track::DeleteExtension(const Track* track)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		size_t sizeBefore = extensions.size();
		extensions.erase(std::remove(extensions.begin(), extensions.end(), track), extensions.end());
		size_t sizeAfter = extensions.size();
		return sizeBefore > sizeAfter;
	}

	void Track::UpdateMain()
	{
		mainTrack = (mainID == TrackNone ? nullptr : manager->GetTrack(mainID));
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
		const LocoBase* loco,
		const bool allowLocoTurn,
		std::vector<Route*>& validRoutes) const
	{
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			for (auto route : routes)
			{
				if (route->FromTrackOrientation(logger, GetID(), locoBaseOrientation, loco, allowLocoTurn))
				{
					validRoutes.push_back(route);
				}
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
			{
			    static std::random_device rd;
			    static std::mt19937 g(rd());
				std::shuffle(validRoutes.begin(), validRoutes.end(), g);
				return;
			}

			case SelectRouteMinTrackLength:
				std::sort(validRoutes.begin(), validRoutes.end(), Route::CompareShortest);
				return;

			case SelectRouteLongestUnused:
				std::sort(validRoutes.begin(), validRoutes.end(), Route::CompareLastUsed);
				return;

			case SelectRouteDoNotCare:
			default:
				// do nothing
				return;
		}
	}

	void Track::PublishState() const
	{
		manager->TrackPublishState(this);
	}
} // namespace DataModel
