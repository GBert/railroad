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

#include <ctime>
#include <map>
#include <string>

#include "DataModel/LocoBase.h"
#include "DataModel/Relation.h"
#include "DataModel/Route.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::string;
using std::to_string;

namespace DataModel
{
	Route::Route(Manager* manager, const std::string& serialized)
	:	LockableItem(),
	 	manager(manager),
	 	executeAtUnlock(false)
	{
		Deserialize(serialized);
		Track* track = manager->GetTrack(fromTrack);
		if (track == nullptr)
		{
			return;
		}
		track->AddRoute(this);
	}

	std::string Route::Serialize() const
	{
		std::string str;
		str = "objectType=Route;";
		str += LayoutItem::Serialize();
		str += ";" + LockableItem::Serialize();
		str += ";delay=" + to_string(delay);
		str += ";lastused=" + to_string(lastUsed);
		str += ";counter=" + to_string(counter);
		str += ";automode=" + to_string(automode);
		if (automode == AutomodeNo)
		{
			return str;
		}
		str += ";fromTrack=" + to_string(fromTrack);
		str += ";fromorientation=" + to_string(fromOrientation);
		str += ";toTrack=" + to_string(toTrack);
		str += ";toorientation=" + to_string(toOrientation);
		str += ";speed=" + to_string(speed);
		str += ";feedbackIdReduced=" + to_string(feedbackIdReduced);
		str += ";feedbackIdCreep=" + to_string(feedbackIdCreep);
		str += ";feedbackIdStop=" + to_string(feedbackIdStop);
		str += ";feedbackIdOver=" + to_string(feedbackIdOver);
		str += ";pushpull=" + to_string(pushpull);
		str += ";propulsion=" + to_string(propulsion);
		str += ";traintype=" + to_string(trainType);
		str += ";mintrainlength=" + to_string(minTrainLength);
		str += ";maxtrainlength=" + to_string(maxTrainLength);
		str += ";waitafterrelease=" + to_string(waitAfterRelease);
		return str;
	}

	bool Route::Deserialize(const std::string& serialized)
	{
		map<string,string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Route") != 0)
		{
			return false;
		}

		LayoutItem::Deserialize(arguments);
		LockableItem::Deserialize(arguments);

		delay = static_cast<Delay>(Utils::Utils::GetIntegerMapEntry(arguments, "delay", DefaultDelay));
		lastUsed = Utils::Utils::GetIntegerMapEntry(arguments, "lastused", 0);
		counter = Utils::Utils::GetIntegerMapEntry(arguments, "counter", 0);
		automode = static_cast<Automode>(Utils::Utils::GetBoolMapEntry(arguments, "automode", AutomodeNo));
		if (automode == AutomodeNo)
		{
			fromTrack = TrackNone;
			fromOrientation = OrientationRight;
			toTrack = TrackNone;
			toOrientation = OrientationRight;
			speed = SpeedTravel;
			feedbackIdReduced = FeedbackNone;
			feedbackIdCreep = FeedbackNone;
			feedbackIdStop = FeedbackNone;
			feedbackIdOver = FeedbackNone;
			pushpull = PushpullTypeBoth;
			propulsion = PropulsionAll;
			trainType = TrainTypeAll;
			minTrainLength = 0;
			maxTrainLength = 0;
			waitAfterRelease = 0;
			return true;
		}
		fromTrack = static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "fromTrack", TrackNone));
		fromOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "fromorientation", OrientationRight));
		toTrack = static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "toTrack", TrackNone));
		toOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "toorientation", OrientationRight));
		speed = static_cast<Speed>(Utils::Utils::GetIntegerMapEntry(arguments, "speed", SpeedTravel));
		feedbackIdReduced = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdReduced", FeedbackNone);
		feedbackIdCreep = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdCreep", FeedbackNone);
		feedbackIdStop = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdStop", FeedbackNone);
		feedbackIdOver = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackIdOver", FeedbackNone);
		pushpull = static_cast<PushpullType>(Utils::Utils::GetIntegerMapEntry(arguments, "pushpull", PushpullTypeBoth));
		propulsion = static_cast<Propulsion>(Utils::Utils::GetIntegerMapEntry(arguments, "propulsion", propulsion));
		trainType = static_cast<TrainType>(Utils::Utils::GetIntegerMapEntry(arguments, "traintype", trainType));
		minTrainLength = static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "mintrainlength", 0));
		maxTrainLength = static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "maxtrainlength", 0));
		waitAfterRelease = Utils::Utils::GetIntegerMapEntry(arguments, "waitafterrelease", 0);
		return true;
	}

	void Route::DeleteRelations(std::vector<DataModel::Relation*>& relations)
	{
		while (!relations.empty())
		{
			delete relations.back();
			relations.pop_back();
		}
	}

	bool Route::AssignRelations(std::vector<DataModel::Relation*>& relations, const std::vector<DataModel::Relation*>& newRelations)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		if (GetLockState() != LockStateFree)
		{
			return false;
		}
		DeleteRelations(relations);
		relations = newRelations;
		return true;
	}

	bool Route::FromTrackOrientation(Logger::Logger* logger,
		const TrackID trackID,
		const Orientation trackOrientation,
		const LocoBase* locoBase,
		const bool allowLocoTurn)
	{
		if (automode == false)
		{
			return false;
		}

		if (fromTrack != trackID)
		{
			return false;
		}

		const Length locoLength = locoBase->GetLength();
		if (locoLength < minTrainLength)
		{
			logger->Debug(Languages::TextTrainIsToShort, GetName());
			return false;
		}
		if (maxTrainLength > 0 && locoLength > maxTrainLength)
		{
			logger->Debug(Languages::TextTrainIsToLong, GetName());
			return false;
		}

		const bool locoPushpull = locoBase->GetPushpull();
		if (pushpull != locoPushpull && pushpull != PushpullTypeBoth)
		{
			logger->Debug(Languages::TextDifferentPushpullTypes, GetName());
			return false;
		}

		const Propulsion locoPropulsion = locoBase->GetPropulsion();
		if ((propulsion & locoPropulsion) != locoPropulsion)
		{
			logger->Debug(Languages::TextDifferentPropulsions, GetName());
			return false;
		}

		const TrainType locoTrainType = locoBase->GetTrainType();
		if (!(trainType & locoTrainType))
		{
			logger->Debug(Languages::TextDifferentTrainTypes, GetName());
			return false;
		}

		if (fromOrientation == trackOrientation)
		{
			return true;
		}

		if (allowLocoTurn == true && locoPushpull == true)
		{
			Track* trackBase = manager->GetTrack(fromTrack);
			if (trackBase != nullptr)
			{
				bool allowTrackTurn = trackBase->GetAllowLocoTurn();
				if (allowTrackTurn == true)
				{
					return true;
				}
			}
		}

		logger->Debug(Languages::TextDifferentOrientations, GetName());
		return false;
	}


	bool Route::Execute(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		const bool isInUse = IsInUse();
		if (isInUse && (locoBaseIdentifier != GetLocoBase()))
		{
			logger->Info(Languages::TextRouteIsLocked, GetName());
			return false;
		}

		std::lock_guard<std::mutex> Guard(updateMutex);
		for (auto relation : relationsAtLock)
		{
			bool retRelation = relation->Execute(logger, locoBaseIdentifier, delay);
			if (retRelation == false)
			{
				return false;
			}
		}
		lastUsed = std::time(nullptr);
		++counter;
		if (isInUse)
		{
			executeAtUnlock = true;
		}

		return true;
	}

	bool Route::Reserve(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		if (manager->Booster() == BoosterStateStop)
		{
			logger->Debug(Languages::TextBoosterIsTurnedOff);
			return false;
		}

		std::lock_guard<std::mutex> Guard(updateMutex);
		bool ret = LockableItem::Reserve(logger, locoBaseIdentifier);
		if (ret == false)
		{
			return false;
		}

		if (automode == AutomodeYes)
		{
			Track* track = manager->GetTrack(toTrack);
			if (track == nullptr)
			{
				ReleaseInternal(logger, locoBaseIdentifier);
				return false;
			}
			if (track->Reserve(logger, locoBaseIdentifier) == false)
			{
				ReleaseInternal(logger, locoBaseIdentifier);
				return false;
			}
		}

		for (auto relation : relationsAtLock)
		{
			bool retRelation = relation->Reserve(logger, locoBaseIdentifier);
			if (retRelation == false)
			{
				ReleaseInternalWithToTrack(logger, locoBaseIdentifier);
				return false;
			}
		}
		return true;
	}

	bool Route::Lock(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		if (manager->Booster() == BoosterStateStop)
		{
			logger->Debug(Languages::TextBoosterIsTurnedOff);
			return false;
		}

		std::lock_guard<std::mutex> Guard(updateMutex);
		bool ret = LockableItem::Lock(logger, locoBaseIdentifier);
		if (ret == false)
		{
			return false;
		}

		if (automode == AutomodeYes)
		{
			Track* track = manager->GetTrack(toTrack);
			if (track == nullptr)
			{
				ReleaseInternal(logger, locoBaseIdentifier);
				return false;
			}
			if (track->Lock(logger, locoBaseIdentifier) == false)
			{
				ReleaseInternal(logger, locoBaseIdentifier);
				return false;
			}
		}

		for (auto relation : relationsAtLock)
		{
			bool retRelation = relation->Lock(logger, locoBaseIdentifier);
			if (retRelation == false)
			{
				ReleaseInternalWithToTrack(logger, locoBaseIdentifier);
				return false;
			}
		}
		return true;
	}

	bool Route::Release(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		std::lock_guard<std::mutex> Guard(updateMutex);
		return ReleaseInternal(logger, locoBaseIdentifier);
	}

	bool Route::ReleaseInternal(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		if (executeAtUnlock)
		{
			for (auto relation : relationsAtUnlock)
			{
				relation->Execute(logger, locoBaseIdentifier, delay);
			}
			executeAtUnlock = false;
		}

		for (auto relation : relationsAtLock)
		{
			relation->Release(logger, locoBaseIdentifier);
		}

		return LockableItem::Release(logger, locoBaseIdentifier);
	}

	void Route::ReleaseInternalWithToTrack(Logger::Logger* logger, const ObjectIdentifier& locoBaseIdentifier)
	{
		Track* track = manager->GetTrack(toTrack);
		if (track != nullptr)
		{
			track->Release(logger, locoBaseIdentifier);
		}
		ReleaseInternal(logger, locoBaseIdentifier);
	}

	bool Route::ObjectIsPartOfRoute(const ObjectIdentifier& identifier) const
	{
		for (auto relation : relationsAtLock)
		{
			if (relation->CompareObject2(identifier))
			{
				return true;
			}
		}
		for (auto relation : relationsAtUnlock)
		{
			if (relation->CompareObject2(identifier))
			{
				return true;
			}
		}
		return false;
	}
} // namespace DataModel

