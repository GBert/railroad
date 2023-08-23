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
	map<string,ObjectID> WebClientTrack::GetSignalOptions(const TrackID trackId) const
	{
		map<string, ObjectID> signalOptions;

		map<string, Signal*> allSignals = manager.SignalListByName();
		for (auto& signal : allSignals)
		{
			Track* trackOfSignal = signal.second->GetTrack();
			if (trackOfSignal != nullptr && trackOfSignal->GetID() != trackId)
			{
				continue;
			}
			signalOptions[signal.first] = signal.second->GetID();
		}
		return signalOptions;
	}

	void WebClientTrack::HandleTrackEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone);
		string name = Languages::GetText(Languages::TextNew);
		bool showName = true;
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		LayoutItemSize height = Utils::Utils::GetIntegerMapEntry(arguments, "length", DataModel::LayoutItem::Height1);
		LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		DataModel::TrackType type = DataModel::TrackTypeStraight;
		vector<FeedbackID> feedbacks;
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
				posx = track->GetPosX();
				posy = track->GetPosY();
				posz = track->GetPosZ();
				height = track->GetHeight();
				rotation = track->GetRotation();
				type = track->GetTrackType();
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
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("feedback", Languages::TextFeedbacks));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("signals", Languages::TextSignals));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("automode", Languages::TextAutomode));
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
		mainContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		HtmlTag i_showName("div");
		i_showName.AddId("i_showname");
		i_showName.AddChildTag(HtmlTagInputCheckboxWithLabel("showname", Languages::TextShowName, "true", showName));
		switch (type)
		{
			case DataModel::TrackTypeStraight:
				break;

			default:
				i_showName.AddAttribute("hidden");
				break;
		}
		mainContent.AddChildTag(i_showName);
		mainContent.AddChildTag(HtmlTagSelectWithLabel("tracktype", Languages::TextType, typeOptions, type).AddAttribute("onchange", "onChangeTrackType();return false;"));
		HtmlTag i_length("div");
		i_length.AddId("i_length");
		i_length.AddChildTag(HtmlTagInputIntegerWithLabel("length", Languages::TextLength, height, DataModel::Track::MinLength, DataModel::Track::MaxLength));
		switch (type)
		{
			case DataModel::TrackTypeTurn:
			case DataModel::TrackTypeTunnelEnd:
				i_length.AddAttribute("hidden");
				break;

			default:
				break;
		}
		mainContent.AddChildTag(i_length);
		formContent.AddChildTag(mainContent);

		formContent.AddChildTag(client.HtmlTagTabPosition(posx, posy, posz, rotation));

		formContent.AddChildTag(HtmlTagTabTrackFeedback(client, feedbacks, trackID));

		formContent.AddChildTag(client.HtmlTagSlaveSelect("signal", signals, GetSignalOptions(trackID)));

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
		const LayoutPosition posX = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posY = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posZ = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		LayoutItemSize height = DataModel::LayoutItem::Height1;
		const LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		const DataModel::TrackType type = static_cast<DataModel::TrackType>(Utils::Utils::GetIntegerMapEntry(arguments, "tracktype", DataModel::TrackTypeStraight));
		switch (type)
		{
			case DataModel::TrackTypeTurn:
			case DataModel::TrackTypeTunnelEnd:
				// height is already 1
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
		vector<FeedbackID> feedbacks;
		unsigned int feedbackCounter = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackcounter", 1);
		for (unsigned int feedback = 1; feedback <= feedbackCounter; ++feedback)
		{
			FeedbackID feedbackID = Utils::Utils::GetIntegerMapEntry(arguments, "feedback_" + to_string(feedback), FeedbackNone);
			if (feedbackID != FeedbackNone)
			{
				feedbacks.push_back(feedbackID);
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
					Relation::TypeTrackSignal));
			}
		}

		const DataModel::SelectRouteApproach selectRouteApproach = static_cast<DataModel::SelectRouteApproach>(Utils::Utils::GetIntegerMapEntry(arguments, "selectrouteapproach", DataModel::SelectRouteSystemDefault));
		const bool allowLocoTurn = Utils::Utils::GetBoolMapEntry(arguments, "allowlocoturn", false);
		const bool releaseWhenFree = Utils::Utils::GetBoolMapEntry(arguments, "releasewhenfree", false);

		string result;
		if (!manager.TrackSave(trackId,
			name,
			showName,
			posX,
			posY,
			posZ,
			height,
			rotation,
			type,
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
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextTrackIsUsedByLoco, track->GetName(), manager.GetLocoName(track->GetLoco()));
			return;
		}

		LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		if (locoID != LocoNone)
		{
			bool ret = manager.LocoIntoTrack(logger, locoID, trackID);
			string trackName = track->GetName();
			ret ? client.ReplyResponse(WebClient::ResponseInfo, Languages::TextLocoIsOnTrack, manager.GetLocoName(locoID), trackName)
				: client.ReplyResponse(WebClient::ResponseError, Languages::TextUnableToAddLocoToTrack, manager.GetLocoName(locoID), trackName);
			return;
		}

		map<string,LocoID> locos = manager.LocoListFree();
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
		Loco::AutoModeType type = static_cast<Loco::AutoModeType>(Utils::Utils::GetIntegerMapEntry(arguments, "automodetype", Loco::AutoModeTypeFull));
		bool ret = manager.TrackStartLoco(trackID, type);
		client.ReplyHtmlWithHeaderAndParagraph(ret ? "Loco started" : "Loco not started");
	}

	void WebClientTrack::HandleTrackStopLoco(const map<string, string>& arguments)
	{
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		bool ret = manager.TrackStopLoco(trackID);
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

	HtmlTag WebClientTrack::HtmlTagTabTrackFeedback(const WebClient& client,
		const std::vector<FeedbackID>& feedbacks,
		const TrackID trackID)
	{
		unsigned int feedbackCounter = 0;
		HtmlTag existingFeedbacks("div");
		existingFeedbacks.AddId("feedbackcontent");
		for (auto feedbackID : feedbacks)
		{
			existingFeedbacks.AddChildTag(client.HtmlTagSelectFeedbackForTrack(++feedbackCounter, trackID, feedbackID));
		}
		existingFeedbacks.AddChildTag(HtmlTag("div").AddId("div_feedback_" + to_string(feedbackCounter + 1)));

		HtmlTag feedbackContent("div");
		feedbackContent.AddId("tab_feedback");
		feedbackContent.AddClass("tab_content");
		feedbackContent.AddClass("hidden");
		feedbackContent.AddChildTag(HtmlTagInputHidden("feedbackcounter", to_string(feedbackCounter)));
		feedbackContent.AddChildTag(existingFeedbacks);
		HtmlTagButton newButton(Languages::TextNew, "newfeedback");
		newButton.AddAttribute("onclick", "addFeedback();return false;");
		newButton.AddClass("wide_button");
		feedbackContent.AddChildTag(newButton);
		feedbackContent.AddChildTag(HtmlTag("br"));
		return feedbackContent;
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
