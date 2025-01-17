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

#include "Logger/Logger.h"
#include "Manager.h"
#include "Server/Web/WebClient.h"

namespace Server { namespace Web
{
	class WebClient;

	class WebClientRoute
	{
		public:
			WebClientRoute() = delete;
			WebClientRoute(const WebClientRoute&) = delete;
			WebClientRoute& operator=(const WebClientRoute&) = delete;

			inline WebClientRoute(Manager& manager, WebClient& client, Logger::Logger* logger)
			:	manager(manager),
				client(client),
				logger(logger)
			{
			}

			void HandleRouteEdit(const std::map<std::string,std::string>& arguments);
			void HandleRouteSave(const std::map<std::string,std::string>& arguments);
			void HandleRouteList();
			void HandleRouteAskDelete(const std::map<std::string,std::string>& arguments);
			void HandleRouteDelete(const std::map<std::string,std::string>& arguments);
			void HandleRouteGet(const std::map<std::string,std::string>& arguments);
			void HandleRouteExecute(const std::map<std::string,std::string>& arguments);
			void HandleRouteRelease(const std::map<std::string,std::string>& arguments);
			void HandleRelationAdd(const std::map<std::string,std::string>& arguments);
			void HandleRelationObject(const std::map<std::string, std::string>& arguments);
			void HandleRelationSwitchStates(const std::map<std::string,std::string>& arguments);
			void HandleFeedbacksOfTrack(const std::map<std::string,std::string>& arguments);

		private:
			HtmlTag HtmlTagRelation(const std::string& atlock,
				const Priority priority,
				const ObjectType objectType = ObjectTypeSwitch,
				const ObjectID objectId = ObjectNone,
				const DataModel::Relation::Data = DataModel::Relation::DefaultData);

			HtmlTag HtmlTagRelationObject(const std::string& atlock,
				const std::string& priorityString,
				const ObjectType objectType,
				const ObjectID objectId = ObjectNone,
				const DataModel::Relation::Data state = DataModel::Relation::DefaultData);

			HtmlTag HtmlTagRelationSwitchState(const std::string& name,
				const SwitchID switchId,
				const DataModel::Relation::Data data = DataModel::SwitchStateStraight);

			HtmlTag HtmlTagSelectTrack(const std::string& name,
				const Languages::TextSelector label,
				const TrackID trackID,
				const Orientation orientation,
				const std::string& onchange = "") const;

			HtmlTag HtmlTagSelectFeedbacksOfTrack(const TrackID trackID,
				const RouteID followUpRoute = RouteNone,
				const FeedbackID feedbackIdReduced = FeedbackNone,
				const Delay reducedDelay = 0,
				const FeedbackID feedbackIdCreep = FeedbackNone,
				const Delay creepDelay = 0,
				const FeedbackID feedbackIdStop = FeedbackNone,
				const Delay stopDelay = 0,
				const FeedbackID feedbackIdOver = FeedbackNone) const;

			Manager& manager;
			WebClient& client;
			Logger::Logger* logger;
	};
}} // namespace Server::Web

