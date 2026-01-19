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

#include <vector>

#include "DataModel/ObjectIdentifier.h"
#include "DataModel/Track.h"
#include "Utils/Utils.h"
#include "Server/Web/HtmlTag.h"
#include "Server/Web/HtmlTagButtonCancel.h"
#include "Server/Web/HtmlTagButtonCommandWide.h"
#include "Server/Web/HtmlTagButtonOK.h"
#include "Server/Web/HtmlTagButtonPopupWide.h"
#include "Server/Web/HtmlTagInputCheckboxWithLabel.h"
#include "Server/Web/HtmlTagInputIntegerWithLabel.h"
#include "Server/Web/HtmlTagInputHidden.h"
#include "Server/Web/HtmlTagInputTextWithLabel.h"
#include "Server/Web/HtmlTagSelect.h"
#include "Server/Web/HtmlTagSelectWithLabel.h"
#include "Server/Web/HtmlTagTrack.h"
#include "Server/Web/WebClient.h"
#include "Server/Web/WebClientCluster.h"
#include "Server/Web/WebClientStatic.h"
#include "Server/Web/WebClientTrack.h"

using namespace DataModel;
using LayoutPosition = DataModel::LayoutItem::LayoutPosition;
using LayoutItemSize = DataModel::LayoutItem::LayoutItemSize;
using LayoutRotation = DataModel::LayoutItem::LayoutRotation;
using std::map;
using std::string;
using std::to_string;
using std::vector;

namespace Server { namespace Web
{
	map<string,ObjectID> WebClientTrack::GetFeedbackOptions(const TrackID trackId) const
	{
		map<string, ObjectID> feedbackOptions;

		map<string, Feedback*> allFeedbacks = manager.FeedbackListByName();
		for (auto& feedback : allFeedbacks)
		{
			Track* trackOfFeedback = feedback.second->GetTrack();
			if (trackOfFeedback && (trackOfFeedback->GetID() != trackId))
			{
				continue;
			}
			feedbackOptions[feedback.first] = feedback.second->GetID();
		}
		return feedbackOptions;
	}

	map<string,ObjectID> WebClientTrack::GetSignalOptions(const TrackID trackId) const
	{
		map<string, ObjectID> signalOptions;

		map<string, Signal*> allSignals = manager.SignalListByName();
		for (auto& signal : allSignals)
		{
			Track* trackOfSignal = signal.second->GetTrack();
			if (trackOfSignal && (trackOfSignal->GetID() != trackId))
			{
				continue;
			}
			signalOptions[signal.first] = signal.second->GetID();
		}
		return signalOptions;
	}

	HtmlTag WebClientTrack::HtmlTagSelectTrack(const std::string& name,
		const Languages::TextSelector label,
		const Languages::TextSelector hint,
		const TrackID trackID,
		const TrackID excludeTrackID,
		const string& onchange) const
	{
		HtmlTag tag;
		map<string,TrackID> tracks = manager.TrackListIdByName(excludeTrackID);
		tracks["-"] = TrackNone;
		HtmlTagSelectWithLabel selectTrack(name, label, hint, tracks, trackID);
		if (onchange.size() > 0)
		{
			selectTrack.AddAttribute("onchange", onchange);
		}
		tag.AddChildTag(selectTrack);
		return tag;
	}

	void WebClientTrack::HandleTrackEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone);
		string name = Languages::GetText(Languages::TextNew);
		bool showName = true;
		string displayName;
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		LayoutItemSize height = Utils::Utils::GetIntegerMapEntry(arguments, "length", DataModel::LayoutItem::Height1);
		LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation90);
		DataModel::TrackType type = DataModel::TrackTypeStraight;
		TrackID main = TrackNone;
		vector<Relation*> feedbacks;
		vector<Relation*> signals;
		Cluster* cluster = nullptr;
		DataModel::SelectRouteApproach selectRouteApproach = static_cast<DataModel::SelectRouteApproach>(Utils::Utils::GetIntegerMapEntry(arguments, "selectrouteapproach", DataModel::SelectRouteSystemDefault));
		bool allowLocoTurn = Utils::Utils::GetBoolMapEntry(arguments, "allowlocoturn", false);
		bool releaseWhenFree = Utils::Utils::GetBoolMapEntry(arguments, "releasewhenfree", false);
		if (trackID > TrackNone)
		{
			const DataModel::Track* track = manager.GetTrack(trackID);
			if (track != nullptr)
			{
				name = track->GetName();
				showName = track->GetShowName();
				displayName = track->GetDisplayName();
				posx = track->GetPosX();
				posy = track->GetPosY();
				posz = track->GetPosZ();
				height = track->GetHeight();
				rotation = track->GetRotation();
				type = track->GetTrackType();
				main = track->GetOwnMainID();
				feedbacks = track->GetFeedbacks();
				signals = track->GetSignals();
				cluster = track->GetCluster();
				selectRouteApproach = track->GetSelectRouteApproach();
				allowLocoTurn = track->GetAllowLocoTurn();
				releaseWhenFree = track->GetReleaseWhenFree();
			}
		}
		switch (type)
		{
			case DataModel::TrackTypeTurn:
			case DataModel::TrackTypeTunnelEnd:
				height = DataModel::LayoutItem::Height1;
				break;

			case DataModel::TrackTypeCrossingLeft:
			case DataModel::TrackTypeCrossingRight:
			case DataModel::TrackTypeCrossingSymetric:
				height = DataModel::LayoutItem::Height2;
				break;

			default:
				break;
		}

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("main", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("position", Languages::TextPosition));
		if (main != TrackNone)
		{
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("feedbacks", Languages::TextFeedbacks).AddClass("hidden"));
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("signals", Languages::TextSignals).AddClass("hidden"));
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("automode", Languages::TextAutomode).AddClass("hidden"));
		}
		else
		{
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("feedbacks", Languages::TextFeedbacks));
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("signals", Languages::TextSignals));
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("automode", Languages::TextAutomode));
		}
		content.AddChildTag(tabMenu);

		HtmlTag formContent("form");
		formContent.AddId("editform");
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "tracksave"));
		formContent.AddChildTag(HtmlTagInputHidden("track", to_string(trackID)));

		std::map<DataModel::TrackType, Languages::TextSelector> typeOptions;
		typeOptions[DataModel::TrackTypeStraight] = Languages::TextStraight;
		typeOptions[DataModel::TrackTypeTurn] = Languages::TextTurn;
		typeOptions[DataModel::TrackTypeEnd] = Languages::TextBufferStop;
		typeOptions[DataModel::TrackTypeBridge] = Languages::TextBridge;
		typeOptions[DataModel::TrackTypeTunnel] = Languages::TextTunnelTwoSides;
		typeOptions[DataModel::TrackTypeTunnelEnd] = Languages::TextTunnelOneSide;
		typeOptions[DataModel::TrackTypeLink] = Languages::TextLink;
		typeOptions[DataModel::TrackTypeCrossingLeft] = Languages::TextCrossingLeft;
		typeOptions[DataModel::TrackTypeCrossingRight] = Languages::TextCrossingRight;
		typeOptions[DataModel::TrackTypeCrossingSymetric] = Languages::TextCrossingSymetric;

		HtmlTag mainContent("div");
		mainContent.AddId("tab_main");
		mainContent.AddClass("tab_content");
		mainContent.AddChildTag(HtmlTagSelectWithLabel("tracktype", Languages::TextType, typeOptions, type).AddAttribute("onchange", "onChangeTrackTypeMainTrack();return false;"));
		mainContent.AddChildTag(HtmlTagSelectTrack("main", Languages::TextMainTrack, Languages::TextMainTrackHint, main, trackID, "onChangeTrackTypeMainTrack();return false;"));

		HtmlTag i_name("div");
		i_name.AddId("i_name");
		i_name.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		if (main != TrackNone)
		{
			i_name.AddClass("hidden");
		}
		mainContent.AddChildTag(i_name);

		HtmlTag i_showName("div");
		i_showName.AddId("i_showname");
		i_showName.AddChildTag(HtmlTagInputCheckboxWithLabel("showname", Languages::TextShowName, "true", showName).AddAttribute("onchange", "onChangeTrackTypeMainTrack();return false;"));
		if ((type != DataModel::TrackTypeStraight) && (main != TrackNone))
		{
			i_showName.AddClass("hidden");
		}
		mainContent.AddChildTag(i_showName);

		HtmlTag i_displayName("div");
		i_displayName.AddId("i_displayname");
		i_displayName.AddChildTag(HtmlTagInputTextWithLabel("displayname", Languages::TextDisplayName, displayName));
		if ((main != TrackNone) || (type != DataModel::TrackTypeStraight) || (!showName))
		{
			i_displayName.AddClass("hidden");
		}
		mainContent.AddChildTag(i_displayName);

		HtmlTag i_length("div");
		i_length.AddId("i_length");
		i_length.AddChildTag(HtmlTagInputIntegerWithLabel("length", Languages::TextLength, height, DataModel::Track::MinLength, DataModel::Track::MaxLength));
		switch (type)
		{
			case DataModel::TrackTypeTurn:
			case DataModel::TrackTypeTunnelEnd:
			case DataModel::TrackTypeCrossingLeft:
			case DataModel::TrackTypeCrossingRight:
			case DataModel::TrackTypeCrossingSymetric:
				i_length.AddClass("hidden");
				break;

			default:
				break;
		}
		mainContent.AddChildTag(i_length);

		formContent.AddChildTag(mainContent);

		formContent.AddChildTag(client.HtmlTagTabPosition(posx, posy, posz, rotation));

		formContent.AddChildTag(client.HtmlTagSelectSlave("feedback", feedbacks, GetFeedbackOptions(trackID)));

		formContent.AddChildTag(client.HtmlTagSelectSlave("signal", signals, GetSignalOptions(trackID)));

		formContent.AddChildTag(HtmlTagTabTrackAutomode(selectRouteApproach, allowLocoTurn, releaseWhenFree, cluster));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(formContent));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientTrack::HandleTrackSave(const map<string, string>& arguments)
	{
		const TrackID trackId = Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const bool showName = Utils::Utils::GetBoolMapEntry(arguments, "showname", true);
		const string displayName = Utils::Utils::GetStringMapEntry(arguments, "displayname");
		const LayoutPosition posX = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posY = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posZ = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		LayoutItemSize height;
		const LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation90);
		const DataModel::TrackType type = static_cast<DataModel::TrackType>(Utils::Utils::GetIntegerMapEntry(arguments, "tracktype", DataModel::TrackTypeStraight));
		const TrackID main = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "main", TrackNone));
		switch (type)
		{
			case DataModel::TrackTypeTurn:
			case DataModel::TrackTypeTunnelEnd:
				height = DataModel::LayoutItem::Height1;
				break;

			case DataModel::TrackTypeCrossingLeft:
			case DataModel::TrackTypeCrossingRight:
			case DataModel::TrackTypeCrossingSymetric:
				height = DataModel::LayoutItem::Height2;
				break;

			default:
				height = Utils::Utils::GetIntegerMapEntry(arguments, "length", 1);
				break;
		}

		vector<Relation*> feedbacks;
		{
			vector<FeedbackID> feedbackIDs = WebClientStatic::InterpretSlaveData("feedback", arguments);
			for (auto feedbackId : feedbackIDs)
			{
				feedbacks.push_back(new Relation(&manager,
					ObjectIdentifier(ObjectTypeTrack, trackId),
					ObjectIdentifier(ObjectTypeFeedback, feedbackId),
					Relation::RelationTypeTrackFeedback));
			}
		}

		vector<Relation*> signals;
		{
			vector<SignalID> signalIds = WebClientStatic::InterpretSlaveData("signal", arguments);
			for (auto signalId : signalIds)
			{
				signals.push_back(new Relation(&manager,
					ObjectIdentifier(ObjectTypeTrack, trackId),
					ObjectIdentifier(ObjectTypeSignal, signalId),
					Relation::RelationTypeTrackSignal));
			}
		}

		const DataModel::SelectRouteApproach selectRouteApproach = static_cast<DataModel::SelectRouteApproach>(Utils::Utils::GetIntegerMapEntry(arguments, "selectrouteapproach", DataModel::SelectRouteSystemDefault));
		const bool allowLocoTurn = Utils::Utils::GetBoolMapEntry(arguments, "allowlocoturn", false);
		const bool releaseWhenFree = Utils::Utils::GetBoolMapEntry(arguments, "releasewhenfree", false);

		string result;
		if (!manager.TrackSave(trackId,
			name,
			showName,
			displayName,
			posX,
			posY,
			posZ,
			height,
			rotation,
			type,
			main,
			feedbacks,
			signals,
			selectRouteApproach,
			allowLocoTurn,
			releaseWhenFree,
			result))
		{
			client.ReplyResponse(WebClient::ResponseError, result);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextTrackSaved, name);
	}

	void WebClientTrack::HandleTrackAskDelete(const map<string, string>& arguments)
	{
		TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone);

		if (trackID == TrackNone)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextTrackDoesNotExist);
			return;
		}

		const DataModel::Track* track = manager.GetTrack(trackID);
		if (track == nullptr)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextTrackDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& trackName = track->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteTrack));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, trackName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "trackdelete"))
			.AddContent(HtmlTagInputHidden("track", to_string(trackID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientTrack::HandleTrackList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextTracks));
		HtmlTag table("table");
		const map<string,DataModel::Track*> trackList = manager.TrackListByName();
		map<string,string> trackArgument;
		for (auto& track : trackList)
		{
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(track.first));
			const string& trackIdString = to_string(track.second->GetID());
			trackArgument["track"] = trackIdString;
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "trackedit_list_" + trackIdString, trackArgument)));
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "trackaskdelete_" + trackIdString, trackArgument)));
			if (track.second->IsInUse())
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonCommandWide(Languages::TextRelease, "trackrelease_" + trackIdString, trackArgument, "hideElement('b_trackrelease_" + trackIdString + "');")));
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "trackedit_0"));
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientTrack::HandleTrackDelete(const map<string, string>& arguments)
	{
		TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone);
		const DataModel::Track* track = manager.GetTrack(trackID);
		if (track == nullptr)
		{
			client.ReplyResponse(WebClient::ResponseError, Languages::TextTrackDoesNotExist);
			return;
		}

		string name = track->GetName();
		string result;
		if (!manager.TrackDelete(trackID, result))
		{
			client.ReplyResponse(WebClient::ResponseError, result);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextTrackDeleted, name);
	}

	void WebClientTrack::HandleTrackGet(const map<string, string>& arguments)
	{
		TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track");
		const DataModel::Track* track = manager.GetTrack(trackID);
		if (track == nullptr)
		{
			client.ReplyHtmlWithHeader(HtmlTag());
			return;
		}
		client.ReplyHtmlWithHeader(HtmlTagTrack(manager, track));
	}

	void WebClientTrack::HandleTrackSetLoco(const map<string, string>& arguments)
	{
		HtmlTag content;
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		Track* track = manager.GetTrack(trackID);
		if (track == nullptr)
		{
			client.ReplyResponse(WebClient::ResponseError, Languages::TextTrackDoesNotExist);
			return;
		}


		if (track->IsInUse())
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextTrackIsUsedByLoco, track->GetName(), manager.GetLocoBaseName(track->GetLocoBase()));
			return;
		}

		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		const ObjectIdentifier locoBaseIdentifier(WebClientStatic::LocoIdToObjectIdentifier(locoID));
		if (locoBaseIdentifier.IsSet())
		{
			const bool ret = manager.LocoBaseIntoTrack(logger, locoBaseIdentifier, trackID);
			const string trackName = track->GetName();
			ret ? client.ReplyResponse(WebClient::ResponseInfo, Languages::TextLocoIsOnTrack, manager.GetLocoBaseName(locoBaseIdentifier), trackName)
				: client.ReplyResponse(WebClient::ResponseError, Languages::TextUnableToAddLocoToTrack, manager.GetLocoBaseName(locoBaseIdentifier), trackName);
			return;
		}

		map<string,LocoID> locos = manager.LocoBaseListFree();
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextSelectLocoForTrack, track->GetName()));
		content.AddChildTag(HtmlTagInputHidden("cmd", "tracksetloco"));
		content.AddChildTag(HtmlTagInputHidden("track", to_string(trackID)));
		content.AddChildTag(HtmlTagSelectWithLabel("loco", Languages::TextLoco, locos));
		content.AddChildTag(HtmlTag("br"));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(HtmlTag("form").AddId("editform").AddChildTag(content));
	}

	void WebClientTrack::HandleTrackRelease(const map<string, string>& arguments)
	{
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		bool ret = manager.TrackRelease(trackID);
		client.ReplyHtmlWithHeaderAndParagraph(ret ? "Track released" : "Track not released");
	}

	void WebClientTrack::HandleTrackStartLoco(const map<string, string>& arguments)
	{
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		bool ret = manager.TrackStartLocoBase(trackID);
		client.ReplyHtmlWithHeaderAndParagraph(ret ? "Loco started" : "Loco not started");
	}

	void WebClientTrack::HandleTrackStopLoco(const map<string, string>& arguments)
	{
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		bool ret = manager.TrackStopLocoBase(trackID);
		client.ReplyHtmlWithHeaderAndParagraph(ret ? "Loco stopped" : "Loco not stopped");
	}

	void WebClientTrack::HandleTrackBlock(const map<string, string>& arguments)
	{
		bool blocked = Utils::Utils::GetBoolMapEntry(arguments, "blocked");
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		manager.TrackBlock(trackID, blocked);
		client.ReplyHtmlWithHeaderAndParagraph(blocked ? "Block received" : "Unblock received");
	}

	void WebClientTrack::HandleTrackOrientation(const map<string, string>& arguments)
	{
		Orientation orientation = (Utils::Utils::GetBoolMapEntry(arguments, "orientation") ? OrientationRight : OrientationLeft);
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		manager.TrackSetLocoOrientation(trackID, orientation);
		client.ReplyHtmlWithHeaderAndParagraph("Loco orientation of track set");
	}

	HtmlTag WebClientTrack::HtmlTagTabTrackAutomode(DataModel::SelectRouteApproach selectRouteApproach,
		const bool allowLocoTurn,
		const bool releaseWhenFree,
		const Cluster* cluster)
	{
		HtmlTag automodeContent("div");
		automodeContent.AddId("tab_automode");
		automodeContent.AddClass("tab_content");
		automodeContent.AddClass("hidden");

		automodeContent.AddChildTag(WebClientStatic::HtmlTagSelectSelectRouteApproach(selectRouteApproach));

		automodeContent.AddChildTag(HtmlTagInputCheckboxWithLabel("allowlocoturn", Languages::TextAllowLocoTurn, "false", allowLocoTurn));

		automodeContent.AddChildTag(HtmlTagInputCheckboxWithLabel("releasewhenfree", Languages::TextReleaseWhenFree, "true", releaseWhenFree));

		if (cluster != nullptr)
		{
			automodeContent.AddChildTag(HtmlTagInputTextWithLabel("cluster", Languages::TextCluster, cluster->GetName(), HtmlTagInput::StyleDisabled));
		}
		return automodeContent;
	}
}} // namespace Server::Web
