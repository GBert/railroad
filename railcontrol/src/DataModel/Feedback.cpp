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

#include <map>
#include <sstream>

#include "DataModel/Feedback.h"
#include "Manager.h"
#include "Utils/Utils.h"

using std::map;
using std::string;
using std::to_string;

namespace DataModel
{
	string Feedback::Serialize() const
	{
		string str;
		str = "objectType=Feedback;" + LayoutItem::Serialize();
		str += ";controlID=" + to_string(controlID);
		str += ";pin=" + to_string(pin);
		str += ";feedbacktype=" + to_string(feedbackType);
		str += ";route=" + to_string(routeId);
		str += ";inverted=" + to_string(inverted);
		str += ";state=" + to_string(stateCounter > 0);
		str += ";matchkey=" + matchKey;
		return str;
	}

	bool Feedback::Deserialize(const string& serialized)
	{
		map<string, string> arguments;
		ParseArguments(serialized, arguments);
		string objectType = Utils::Utils::GetStringMapEntry(arguments, "objectType");
		if (objectType.compare("Feedback") != 0)
		{
			return false;
		}
		LayoutItem::Deserialize(arguments);
		SetHeight(Height1);
		SetWidth(Width1);
		controlID = Utils::Utils::GetIntegerMapEntry(arguments, "controlID", ControlIdNone);
		pin = Utils::Utils::GetIntegerMapEntry(arguments, "pin");
		feedbackType = static_cast<FeedbackType>(Utils::Utils::GetIntegerMapEntry(arguments, "feedbacktype", FeedbackTypeDefault));
		routeId = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);
		inverted = Utils::Utils::GetBoolMapEntry(arguments, "inverted", false);
		stateCounter = Utils::Utils::GetBoolMapEntry(arguments, "state", FeedbackStateFree) ? MaxStateCounter : 0;
		matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		return true;
	}

	void Feedback::SetState(const FeedbackState newState)
	{
		FeedbackState state = static_cast<FeedbackState>(newState != inverted);
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			if (state == FeedbackStateFree)
			{
				if (stateCounter < MaxStateCounter)
				{
					return;
				}
				stateCounter = MaxStateCounter - 1;
				return;
			}

			const unsigned char oldStateCounter = stateCounter;
			stateCounter = MaxStateCounter;

			if (oldStateCounter > 0)
			{
				return;
			}
		}

		manager->FeedbackPublishState(this);
		UpdateTrackState(FeedbackStateOccupied);

		Route* route = manager->GetRoute(routeId);
		if (route)
		{
			static Logger::Logger* logger = Logger::Logger::GetLogger(Languages::GetText(Languages::TextManager));
			route->Execute(logger, ObjectIdentifier());
		}
	}

	void Feedback::UpdateTrackState(const FeedbackState state)
	{
		if (!track)
		{
			return;
		}
		track->SetFeedbackState(GetID(), state);
	}

	void Feedback::Debounce()
	{
		{
			std::lock_guard<std::mutex> Guard(updateMutex);
			if (stateCounter == MaxStateCounter || stateCounter == 0)
			{
				return;
			}

			--stateCounter;
			if (stateCounter != 0)
			{
				return;
			}
		}
		manager->FeedbackPublishState(this);
		UpdateTrackState(FeedbackStateFree);
	}

	Feedback& Feedback::operator=(const Hardware::FeedbackCacheEntry& feedback)
	{
		SetControlID(feedback.GetControlID());
		SetPin(feedback.GetPin());
		SetName(feedback.GetName());
		SetMatchKey(feedback.GetMatchKey());
		return *this;
	}
} // namespace DataModel

