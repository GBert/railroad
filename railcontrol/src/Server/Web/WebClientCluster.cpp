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

#include <algorithm>
#include <map>
#include <vector>

#include "DataModel/Cluster.h"
#include "DataModel/ObjectIdentifier.h"
#include "DataModel/Signal.h"
#include "DataModel/Track.h"
#include "Server/Web/HtmlTag.h"
#include "Server/Web/HtmlTagButtonCancel.h"
#include "Server/Web/HtmlTagButtonOK.h"
#include "Server/Web/HtmlTagButtonPopupWide.h"
#include "Server/Web/HtmlTagInputHidden.h"
#include "Server/Web/HtmlTagInputTextWithLabel.h"
#include "Server/Web/HtmlTagSelect.h"
#include "Server/Web/WebClient.h"
#include "Server/Web/WebClientCluster.h"
#include "Server/Web/WebClientStatic.h"

using DataModel::Cluster;
using DataModel::ObjectIdentifier;
using DataModel::Relation;
using DataModel::Signal;
using DataModel::Track;
using std::map;
using std::string;
using std::to_string;
using std::vector;

namespace Server { namespace Web
{
	void WebClientCluster::HandleClusterList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextCluster));
		HtmlTag table("table");
		map<string,string> clusterArgument;
		const map<string,Cluster*> clusterList = manager.ClusterListByName();
		for (auto& cluster : clusterList)
		{
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(cluster.first));
			const string& clusterIdString = to_string(cluster.second->GetID());
			clusterArgument["cluster"] = clusterIdString;
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "clusteredit_list_" + clusterIdString, clusterArgument)));
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "clusteraskdelete_" + clusterIdString, clusterArgument)));
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "clusteredit_0"));
		client.ReplyHtmlWithHeader(content);
	}

	HtmlTag WebClientCluster::HtmlTagSelectTrackEntry(const string& priority,
		const TrackID trackID,
		const map<string,ObjectID>& trackOptions,
		const Relation::Data inverted,
		const map<string,ObjectID>& invertedOptions)
	{
		HtmlTag content("div");
		content.AddId("track_priority_" + priority);

		HtmlTagButton deleteButton(Languages::TextDelete, "track_delete_" + priority);
		deleteButton.AddAttribute("onclick", "deleteElement('track_priority_" + priority + "');return false;");
		deleteButton.AddClass("wide_button");
		content.AddChildTag(deleteButton);

		HtmlTag contentObject("div");
		contentObject.AddId("track_object_" + priority);
		contentObject.AddClass("inline-block");

		contentObject.AddChildTag(HtmlTagSelect("track_id_" + priority, trackOptions, trackID).AddClass("select_relation_id"));
		contentObject.AddChildTag(HtmlTagSelect("track_inverted_" + priority, invertedOptions, inverted).AddClass("select_relation_data"));
		content.AddChildTag(contentObject);
		return content;
	}

	HtmlTag WebClientCluster::HtmlTagSelectTrack(const vector<Relation*>& relations,
		const ClusterID clusterID) const
	{
		HtmlTag content("div");
		content.AddId("tab_tracks");
		content.AddClass("tab_content");
		content.AddClass("hidden");

		HtmlTag div("div");
		div.AddChildTag(HtmlTagInputHidden("trackcounter", to_string(relations.size())));
		div.AddId("tracks");

		const map<string,ObjectID> trackOptions = GetTrackOptions(clusterID);
		map<string,Relation::Data> invertedOptions;
		invertedOptions[Languages::GetText(Languages::TextDefault)] = 0;
		invertedOptions[Languages::GetText(Languages::TextInverted2)] = 1;
		unsigned int counter = 1;
		for (auto relation : relations)
		{
			const TrackID trackID = relation->ObjectID2();
			const Relation::Data inverted = relation->GetData();
			div.AddChildTag(HtmlTagSelectTrackEntry(to_string(counter), trackID, trackOptions, inverted, invertedOptions));
			++counter;
		}
		div.AddChildTag(HtmlTag("div").AddId("track_new_" + to_string(counter)));

		content.AddChildTag(div);
		HtmlTagButton newTrackButton(Languages::TextNew, "newtrack");
		newTrackButton.AddAttribute("onclick", "addSlave('track');return false;");
		newTrackButton.AddClass("wide_button");
		content.AddChildTag(newTrackButton);

		content.AddChildTag(HtmlTag("br"));
		return content;
	}

	void WebClientCluster::HandleClusterEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		ClusterID clusterID = Utils::Utils::GetIntegerMapEntry(arguments, "cluster", ClusterNone);
		string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		vector<Relation*> tracks;

		if (clusterID != ClusterNone)
		{
			Cluster* cluster = manager.GetCluster(clusterID);
			if (cluster)
			{
				name = cluster->GetName();
				tracks = cluster->GetTracks();
			}
		}

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("basic", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("tracks", Languages::TextTracks));
		content.AddChildTag(tabMenu);

		HtmlTag formContent("form");
		formContent.AddId("editform");
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "clustersave"));
		formContent.AddChildTag(HtmlTagInputHidden("cluster", to_string(clusterID)));

		HtmlTag basicContent("div");
		basicContent.AddId("tab_basic");
		basicContent.AddClass("tab_content");
		basicContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		formContent.AddChildTag(basicContent);

		formContent.AddChildTag(HtmlTagSelectTrack(tracks, clusterID));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(formContent));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientCluster::HandleClusterSave(const map<string, string>& arguments)
	{
		ClusterID clusterID = Utils::Utils::GetIntegerMapEntry(arguments, "cluster", ClusterNone);
		string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const unsigned int count = Utils::Utils::GetIntegerMapEntry(arguments, "trackcounter", 0);

		vector<Relation*> tracks;
		{
			for (unsigned int index = 1; index <= count; ++index)
			{
				const string indexAsString = to_string(index);
				const TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track_id_" + indexAsString, TrackNone);
				if (trackID == TrackNone)
				{
					continue;
				}
				const bool inverted = Utils::Utils::GetBoolMapEntry(arguments, "track_inverted_" + indexAsString, false);

				tracks.push_back(new Relation(&manager,
					ObjectIdentifier(ObjectTypeCluster, clusterID),
					ObjectIdentifier(ObjectTypeTrack, trackID),
					Relation::RelationTypeClusterTrack,
					0,
					static_cast<Relation::Data>(inverted)));
			}
		}

		string result;
		if (!manager.ClusterSave(clusterID, name, tracks, result))
		{
			client.ReplyResponse(WebClient::ResponseError, result);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextClusterSaved, name);
	}

	void WebClientCluster::HandleClusterAskDelete(const map<string, string>& arguments)
	{
		ClusterID clusterID = Utils::Utils::GetIntegerMapEntry(arguments, "cluster", ClusterNone);

		if (clusterID == ControlNone)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextClusterDoesNotExist);
			return;
		}

		const Cluster* cluster = manager.GetCluster(clusterID);
		if (!cluster)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextClusterDoesNotExist);
			return;
		}

		HtmlTag content;
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteCluster));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, cluster->GetName()));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "clusterdelete"))
			.AddContent(HtmlTagInputHidden("cluster", to_string(clusterID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientCluster::HandleClusterDelete(const map<string, string>& arguments)
	{
		ClusterID clusterID = Utils::Utils::GetIntegerMapEntry(arguments, "cluster", ClusterNone);
		const Cluster* cluster = manager.GetCluster(clusterID);
		if (!cluster)
		{
			client.ReplyResponse(WebClient::ResponseError, Languages::TextClusterDoesNotExist);
			return;
		}

		string name = cluster->GetName();

		if (!manager.ClusterDelete(clusterID))
		{
			client.ReplyResponse(WebClient::ResponseError, Languages::TextClusterDoesNotExist);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextClusterDeleted, name);
	}

	map<string,ObjectID> WebClientCluster::GetTrackOptions(const ClusterID clusterId) const
	{
		map<string, ObjectID> trackOptions;

		map<string, Track*> tracks = manager.TrackListMasterByName();
		for (auto& track : tracks)
		{
			Cluster* clusterOfTrack = track.second->GetCluster();
			if (clusterOfTrack && clusterOfTrack->GetID() != clusterId)
			{
				continue;
			}
			trackOptions[track.first] = track.second->GetID();
		}
		return trackOptions;
	}
}} // namespace Server::Web
