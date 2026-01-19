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

#include <map>
#include <string>
#include <vector>

#include "DataModel/Relation.h"
#include "Manager.h"

namespace Server { namespace Web
{
	class WebClient;

	class WebClientCluster
	{
		public:
			WebClientCluster() = delete;

			inline WebClientCluster(Manager& manager, WebClient& client)
			:	manager(manager),
				client(client)
			{
			}

			void HandleClusterList();

			static HtmlTag HtmlTagSelectTrackEntry(const std::string& priority,
				const TrackID trackID,
				const std::map<std::string,ObjectID>& trackOptions,
				const DataModel::Relation::Data inverted,
				const std::map<std::string,ObjectID>& invertedOptions);

			HtmlTag HtmlTagSelectTrack(const std::vector<DataModel::Relation*>& relations,
				const ClusterID clusterID) const;

			void HandleClusterEdit(const std::map<std::string,std::string>& arguments);

			void HandleClusterSave(const std::map<std::string,std::string>& arguments);

			void HandleClusterAskDelete(const std::map<std::string,std::string>& arguments);

			void HandleClusterDelete(const std::map<std::string,std::string>& arguments);

			std::map<std::string,ObjectID> GetTrackOptions(const ClusterID clusterId = ClusterNone) const;

		private:
			Manager& manager;
			WebClient& client;
	};

}} // namespace Server::Web

