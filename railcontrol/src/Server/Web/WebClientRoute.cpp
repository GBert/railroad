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
#include <string>
#include <utility>
#include <vector>

#include "DataModel/DataModel.h"
#include "DataModel/ObjectIdentifier.h"
#include "Server/Web/HtmlTagButton.h"
#include "Server/Web/HtmlTagButtonCancel.h"
#include "Server/Web/HtmlTagButtonCommandWide.h"
#include "Server/Web/HtmlTagButtonOK.h"
#include "Server/Web/HtmlTagButtonPopupWide.h"
#include "Server/Web/HtmlTagButtonSwapRelation.h"
#include "Server/Web/HtmlTagInputCheckboxWithLabel.h"
#include "Server/Web/HtmlTagInputHidden.h"
#include "Server/Web/HtmlTagInputIntegerWithLabel.h"
#include "Server/Web/HtmlTagInputTextWithLabel.h"
#include "Server/Web/HtmlTagRoute.h"
#include "Server/Web/HtmlTagSelect.h"
#include "Server/Web/HtmlTagSelectMultipleWithLabel.h"
#include "Server/Web/HtmlTagSelectOrientation.h"
#include "Server/Web/HtmlTagSelectWithLabel.h"
#include "Server/Web/HtmlTagSpace.h"
#include "Server/Web/WebClient.h"
#include "Server/Web/WebClientRoute.h"
#include "Server/Web/WebClientStatic.h"
#include "Utils/Integer.h"

using namespace DataModel;
using LayoutPosition = DataModel::LayoutItem::LayoutPosition;
using LayoutItemSize = DataModel::LayoutItem::LayoutItemSize;
using LayoutRotation = DataModel::LayoutItem::LayoutRotation;
using Visible = DataModel::LayoutItem::Visible;
using std::map;
using std::pair;
using std::string;
using std::to_string;
using std::vector;

namespace Server { namespace Web
{
	void WebClientRoute::HandleRouteGet(const map<string, string>& arguments)
	{
		RouteID routeID = Utils::Utils::GetIntegerMapEntry(arguments, "route");
		const DataModel::Route* route = manager.GetRoute(routeID);
		if (route == nullptr || route->GetVisible() == DataModel::LayoutItem::VisibleNo)
		{
			client.ReplyHtmlWithHeader(HtmlTag());
			return;
		}
		client.ReplyHtmlWithHeader(HtmlTagRoute(route));
	}

	void WebClientRoute::HandleRouteEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		const RouteID routeID = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);
		string name = Languages::GetText(Languages::TextNew);
		Delay delay = Route::DefaultDelay;
		Route::PushpullType pushpull = Route::PushpullTypeBoth;
		Propulsion propulsion = PropulsionAll;
		TrainType trainType = TrainTypeAll;
		Length minTrainLength = 0;
		Length maxTrainLength = 0;
		vector<Relation*> relationsAtLock;
		vector<Relation*> relationsAtUnlock;
		vector<Relation*> relationsConditions;
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", LayerUndeletable);
		DataModel::LayoutItem::Visible visible = static_cast<Visible>(Utils::Utils::GetBoolMapEntry(arguments, "visible", routeID == RouteNone && ((posx || posy) && posz >= LayerUndeletable) ? DataModel::LayoutItem::VisibleYes : DataModel::LayoutItem::VisibleNo));
		Automode automode = static_cast<Automode>(Utils::Utils::GetBoolMapEntry(arguments, "automode", AutomodeNo));
		TrackID fromTrack = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "fromtrack", TrackNone));
		Orientation fromOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "fromorientation", OrientationRight));
		TrackID toTrack = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "totrack", TrackNone));
		Orientation toOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "toorientation", OrientationRight));
		Route::Speed speed = static_cast<Route::Speed>(Utils::Utils::GetIntegerMapEntry(arguments, "speed", Route::SpeedTravel));
		FeedbackID feedbackIdReduced = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackreduced", FeedbackNone);
		Delay reducedDelay = Utils::Utils::GetIntegerMapEntry(arguments, "reduceddelay", 0);
		FeedbackID feedbackIdCreep = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackcreep", FeedbackNone);
		Delay creepDelay = Utils::Utils::GetIntegerMapEntry(arguments, "creepdelay", 0);
		FeedbackID feedbackIdStop = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackstop", FeedbackNone);
		Delay stopDelay = Utils::Utils::GetIntegerMapEntry(arguments, "stopdelay", 0);
		FeedbackID feedbackIdOver = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackover", FeedbackNone);
		Pause waitAfterRelease = Utils::Utils::GetIntegerMapEntry(arguments, "waitafterrelease", 0);
		RouteID followUpRoute = Utils::Utils::GetIntegerMapEntry(arguments, "followuproute", RouteNone);
		if (routeID > RouteNone)
		{
			const DataModel::Route* route = manager.GetRoute(routeID);
			if (route != nullptr)
			{
				name = route->GetName();
				delay = route->GetDelay();
				pushpull = route->GetPushpull();
				propulsion = route->GetPropulsion();
				trainType = route->GetTrainType();
				minTrainLength = route->GetMinTrainLength();
				maxTrainLength = route->GetMaxTrainLength();
				relationsAtLock = route->GetRelationsAtLock();
				relationsAtUnlock = route->GetRelationsAtUnlock();
				relationsConditions = route->GetRelationsConditions();
				visible = route->GetVisible();
				posx = route->GetPosX();
				posy = route->GetPosY();
				posz = route->GetPosZ();
				automode = route->GetAutomode();
				fromTrack = route->GetFromTrack();
				fromOrientation = route->GetFromOrientation();
				toTrack = route->GetToTrack();
				toOrientation = route->GetToOrientation();
				speed = route->GetSpeed();
				feedbackIdReduced = route->GetFeedbackIdReduced();
				reducedDelay = route->GetReducedDelay();
				feedbackIdCreep = route->GetFeedbackIdCreep();
				creepDelay = route->GetCreepDelay();
				feedbackIdStop = route->GetFeedbackIdStop();
				stopDelay = route->GetStopDelay();
				feedbackIdOver = route->GetFeedbackIdOver();
				waitAfterRelease = route->GetWaitAfterRelease();
				followUpRoute = route->GetFollowUpRoute();
			}
		}

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("basic", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("relationatlock", Languages::TextAtLock));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("relationatunlock", Languages::TextAtUnlock, false, !automode));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("relationconditions", Languages::TextConditions));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("position", Languages::TextPosition));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("automode", Languages::TextAutomode));
		content.AddChildTag(tabMenu);

		HtmlTag formContent("form");
		formContent.AddId("editform");
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "routesave"));
		formContent.AddChildTag(HtmlTagInputHidden("route", to_string(routeID)));

		HtmlTag basicContent("div");
		basicContent.AddId("tab_basic");
		basicContent.AddClass("tab_content");
		basicContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		basicContent.AddChildTag(HtmlTagInputIntegerWithLabel("delay", Languages::TextWaitingTimeBetweenMembers, delay, 1, USHRT_MAX));
		formContent.AddChildTag(basicContent);

		HtmlTag relationDivAtLock("div");
		relationDivAtLock.AddId("relationatlock");
		Priority priorityAtLock = 1;
		for (auto relation : relationsAtLock)
		{
			relationDivAtLock.AddChildTag(HtmlTagRelation("atlock", relation->GetPriority(), relation->ObjectType2(), relation->ObjectID2(), relation->GetData()));
			priorityAtLock = relation->GetPriority() + 1;
		}
		const string priorityAtLockString = to_string(priorityAtLock);
		relationDivAtLock.AddChildTag(HtmlTagInputHidden("relationcounteratlock", priorityAtLockString));
		relationDivAtLock.AddChildTag(HtmlTag("div").AddId("new_atlock_priority_" + priorityAtLockString));

		HtmlTag relationContentAtLock("div");
		relationContentAtLock.AddId("tab_relationatlock");
		relationContentAtLock.AddClass("tab_content");
		relationContentAtLock.AddClass("hidden");
		relationContentAtLock.AddChildTag(relationDivAtLock);
		HtmlTagButton newButtonAtLock(Languages::TextNew, "newrelationatlock");
		newButtonAtLock.AddAttribute("onclick", "addRelation('atlock');return false;");
		newButtonAtLock.AddClass("wide_button");
		relationContentAtLock.AddChildTag(newButtonAtLock);
		relationContentAtLock.AddChildTag(HtmlTag("br"));
		formContent.AddChildTag(relationContentAtLock);

		HtmlTag relationDivAtUnlock("div");
		relationDivAtUnlock.AddId("relationatunlock");
		Priority priorityAtUnlock = 1;
		for (auto relation : relationsAtUnlock)
		{
			relationDivAtUnlock.AddChildTag(HtmlTagRelation("atunlock", relation->GetPriority(), relation->ObjectType2(), relation->ObjectID2(), relation->GetData()));
			priorityAtUnlock = relation->GetPriority() + 1;
		}
		const string priorityAtUnlockString = to_string(priorityAtUnlock);
		relationDivAtUnlock.AddChildTag(HtmlTagInputHidden("relationcounteratunlock", priorityAtUnlockString));
		relationDivAtUnlock.AddChildTag(HtmlTag("div").AddId("new_atunlock_priority_" + priorityAtUnlockString));

		HtmlTag relationContentAtUnlock("div");
		relationContentAtUnlock.AddId("tab_relationatunlock");
		relationContentAtUnlock.AddClass("tab_content");
		relationContentAtUnlock.AddClass("hidden");
		relationContentAtUnlock.AddChildTag(relationDivAtUnlock);
		HtmlTagButton newButtonAtUnlock(Languages::TextNew, "newrelationatunlock");
		newButtonAtUnlock.AddAttribute("onclick", "addRelation('atunlock');return false;");
		newButtonAtUnlock.AddClass("wide_button");
		relationContentAtUnlock.AddChildTag(newButtonAtUnlock);
		relationContentAtUnlock.AddChildTag(HtmlTag("br"));
		formContent.AddChildTag(relationContentAtUnlock);

		HtmlTag relationDivConditions("div");
		relationDivConditions.AddId("relationconditions");
		Priority priorityConditions = 1;
		for (auto relation : relationsConditions)
		{
			relationDivConditions.AddChildTag(HtmlTagRelation("conditions", relation->GetPriority(), relation->ObjectType2(), relation->ObjectID2(), relation->GetData()));
			priorityConditions = relation->GetPriority() + 1;
		}
		const string priorityConditionsString = to_string(priorityConditions);
		relationDivConditions.AddChildTag(HtmlTagInputHidden("relationcounterconditions", priorityConditionsString));
		relationDivConditions.AddChildTag(HtmlTag("div").AddId("new_conditions_priority_" + priorityConditionsString));

		HtmlTag relationConditions("div");
		relationConditions.AddId("tab_relationconditions");
		relationConditions.AddClass("tab_content");
		relationConditions.AddClass("hidden");
		relationConditions.AddChildTag(relationDivConditions);
		HtmlTagButton newButtonConditions(Languages::TextNew, "newrelationconditions");
		newButtonConditions.AddAttribute("onclick", "addRelation('conditions');return false;");
		newButtonConditions.AddClass("wide_button");
		relationConditions.AddChildTag(newButtonConditions);
		relationConditions.AddChildTag(HtmlTag("br"));
		formContent.AddChildTag(relationConditions);

		formContent.AddChildTag(client.HtmlTagTabPosition(posx, posy, posz, visible));

		HtmlTag automodeContent("div");
		automodeContent.AddId("tab_automode");
		automodeContent.AddClass("tab_content");
		automodeContent.AddClass("hidden");

		HtmlTagInputCheckboxWithLabel checkboxAutomode("automode", Languages::TextAutomode, "automode", static_cast<bool>(automode));
		checkboxAutomode.AddId("automode");
		checkboxAutomode.AddAttribute("onchange", "onChangeCheckboxShowHide('automode', 'tracks', 'tab_button_relationatunlock');");
		automodeContent.AddChildTag(checkboxAutomode);

		HtmlTag tracksDiv("div");
		tracksDiv.AddId("tracks");
		if (automode == AutomodeNo)
		{
			tracksDiv.AddAttribute("hidden");
		}
		tracksDiv.AddChildTag(HtmlTagSelectTrack("from", Languages::TextStartTrack, fromTrack, fromOrientation));
		tracksDiv.AddChildTag(HtmlTagSelectTrack("to", Languages::TextDestinationTrack, toTrack, toOrientation, "updateFeedbacksOfTrack(); return false;"));

		map<Route::Speed,Languages::TextSelector> speedOptions;
		speedOptions[Route::SpeedTravel] = Languages::TextTravelSpeed;
		speedOptions[Route::SpeedReduced] = Languages::TextReducedSpeed;
		speedOptions[Route::SpeedCreeping] = Languages::TextCreepingSpeed;
		tracksDiv.AddChildTag(HtmlTagSelectWithLabel("speed", Languages::TextSpeed, speedOptions, speed));
		HtmlTag feedbackDiv("div");
		feedbackDiv.AddId("feedbacks");
		feedbackDiv.AddChildTag(HtmlTagSelectFeedbacksOfTrack(toTrack, followUpRoute, feedbackIdReduced, reducedDelay, feedbackIdCreep, creepDelay, feedbackIdStop, stopDelay, feedbackIdOver));
		tracksDiv.AddChildTag(feedbackDiv);

		map<Route::PushpullType,Languages::TextSelector> pushpullOptions;
		pushpullOptions[Route::PushpullTypeNo] = Languages::TextNoPushPull;
		pushpullOptions[Route::PushpullTypeBoth] = Languages::TextAllTrains;
		pushpullOptions[Route::PushpullTypeOnly] = Languages::TextPushPullOnly;
		tracksDiv.AddChildTag(HtmlTagSelectWithLabel("pushpull", Languages::TextAllowedPushPull, pushpullOptions, pushpull));

		vector<pair<Propulsion,Languages::TextSelector>> propulsionOptions;
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionAll,Languages::TextPropulsionAll));
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionSteam,Languages::TextPropulsionSteam));
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionDiesel,Languages::TextPropulsionDiesel));
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionGas,Languages::TextPropulsionGas));
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionElectric,Languages::TextPropulsionElectric));
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionHydrogen,Languages::TextPropulsionHydrogen));
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionAccu,Languages::TextPropulsionAccu));
		propulsionOptions.push_back(pair<Propulsion,Languages::TextSelector>(PropulsionOther,Languages::TextPropulsionOther));
		tracksDiv.AddChildTag(HtmlTagSelectMultipleWithLabel("propulsion", Languages::TextAllowedPropulsions, propulsionOptions, propulsion));

		vector<pair<TrainType,Languages::TextSelector>> trainTypeOptions;
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeAll,Languages::TextTrainTypeAll));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypePassenger,Languages::TextTrainTypePassenger));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeInternationalHighSpeed,Languages::TextTrainTypeInternationalHighSpeed));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeNationalHighSpeed,Languages::TextTrainTypeNationalHighSpeed));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeInternationalLongDistance,Languages::TextTrainTypeInternationalLongDistance));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeNationalLongDistance,Languages::TextTrainTypeNationalLongDistance));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeInternationalNight,Languages::TextTrainTypeInternationalNight));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeNationalNight,Languages::TextTrainTypeNationalNight));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeLongDistanceFastLocal,Languages::TextTrainTypeLongDistanceFastLocal));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeFastLocal,Languages::TextTrainTypeFastLocal));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeLocal,Languages::TextTrainTypeLocal));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeSuburban,Languages::TextTrainTypeSuburban));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeUnderground,Languages::TextTrainTypeUnderground));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeHistoric,Languages::TextTrainTypeHistoric));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeExtra,Languages::TextTrainTypeExtra));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypePassengerWithCargo,Languages::TextTrainTypePassengerWithCargo));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCargo,Languages::TextTrainTypeCargo));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCargoLongDistance,Languages::TextTrainTypeCargoLongDistance));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCargoLocal,Languages::TextTrainTypeCargoLocal));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCargoBlock,Languages::TextTrainTypeCargoBlock));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCargoTractor,Languages::TextTrainTypeCargoTractor));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCargoExpress,Languages::TextTrainTypeCargoExpress));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCargoWithPassenger,Languages::TextTrainTypeCargoWithPassenger));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeRescue,Languages::TextTrainTypeRescue));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeConstruction,Languages::TextTrainTypeConstruction));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeEmpty,Languages::TextTrainTypeEmpty));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeLoco,Languages::TextTrainTypeLoco));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeCleaning,Languages::TextTrainTypeCleaning));
		trainTypeOptions.push_back(pair<TrainType,Languages::TextSelector>(TrainTypeOther,Languages::TextTrainTypeOther));
		tracksDiv.AddChildTag(HtmlTagSelectMultipleWithLabel("traintype", Languages::TextAllowedTrainTypes, trainTypeOptions, trainType));

		tracksDiv.AddChildTag(HtmlTagInputIntegerWithLabel("mintrainlength", Languages::TextMinTrainLength, minTrainLength, 0, 99999));
		tracksDiv.AddChildTag(HtmlTagInputIntegerWithLabel("maxtrainlength", Languages::TextMaxTrainLength, maxTrainLength, 0, 99999));
		tracksDiv.AddChildTag(HtmlTagInputIntegerWithLabel("waitafterrelease", Languages::TextWaitAfterRelease, waitAfterRelease, 0, 300));

		automodeContent.AddChildTag(tracksDiv);
		formContent.AddChildTag(automodeContent);

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(formContent));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientRoute::HandleRouteSave(const map<string, string>& arguments)
	{
		const RouteID routeID = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const Delay delay = static_cast<Delay>(Utils::Utils::GetIntegerMapEntry(arguments, "delay"));
		const Route::PushpullType pushpull = static_cast<Route::PushpullType>(Utils::Utils::GetIntegerMapEntry(arguments, "pushpull", Route::PushpullTypeBoth));
		Propulsion propulsion = static_cast<Propulsion>(Utils::Utils::GetIntegerMapEntry(arguments, "propulsion", PropulsionAll));
		if (propulsion == PropulsionUnknown)
		{
			propulsion = PropulsionAll;
		}
		TrainType trainType = static_cast<TrainType>(Utils::Utils::GetIntegerMapEntry(arguments, "traintype", TrainTypeAll));
		if (trainType == TrainTypeUnknown)
		{
			trainType = TrainTypeAll;
		}
		const Length mintrainlength = static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "mintrainlength", 0));
		const Length maxtrainlength = static_cast<Length>(Utils::Utils::GetIntegerMapEntry(arguments, "maxtrainlength", 0));
		const Visible visible = static_cast<Visible>(Utils::Utils::GetBoolMapEntry(arguments, "visible"));
		const LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		const Automode automode = static_cast<Automode>(Utils::Utils::GetBoolMapEntry(arguments, "automode"));
		const TrackID fromTrack = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "fromtrack", TrackNone));
		const Orientation fromOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "fromorientation", OrientationRight));
		const TrackID toTrack = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "totrack", TrackNone));
		const Orientation toOrientation = static_cast<Orientation>(Utils::Utils::GetBoolMapEntry(arguments, "toorientation", OrientationRight));
		const Route::Speed speed = static_cast<Route::Speed>(Utils::Utils::GetIntegerMapEntry(arguments, "speed", Route::SpeedTravel));
		const FeedbackID feedbackIdReduced = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackreduced", FeedbackNone);
		const Delay reducedDelay = Utils::Utils::GetIntegerMapEntry(arguments, "reduceddelay", 0);
		const FeedbackID feedbackIdCreep = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackcreep", FeedbackNone);
		const Delay creepDelay = Utils::Utils::GetIntegerMapEntry(arguments, "creepdelay", 0);
		const FeedbackID feedbackIdStop = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackstop", FeedbackNone);
		const Delay stopDelay = Utils::Utils::GetIntegerMapEntry(arguments, "stopdelay", 0);
		const FeedbackID feedbackIdOver = Utils::Utils::GetIntegerMapEntry(arguments, "feedbackover", FeedbackNone);
		const Pause waitAfterRelease = Utils::Utils::GetIntegerMapEntry(arguments, "waitafterrelease", 0);
		const RouteID followUpRoute = Utils::Utils::GetIntegerMapEntry(arguments, "followuproute", RouteNone);

		Priority relationCountAtLock = Utils::Utils::GetIntegerMapEntry(arguments, "relationcounteratlock", 0);
		Priority relationCountAtUnlock = Utils::Utils::GetIntegerMapEntry(arguments, "relationcounteratunlock", 0);
		Priority relationCounterConditions = Utils::Utils::GetIntegerMapEntry(arguments, "relationcounterconditions", 0);

		vector<Relation*> relationsAtLock;
		Priority priorityAtLock = 1;
		for (Priority relationId = 1; relationId <= relationCountAtLock; ++relationId)
		{
			string priorityString = to_string(relationId);
			ObjectType objectType = static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "relation_atlock_" + priorityString + "_type"));
			ObjectID objectId = Utils::Utils::GetIntegerMapEntry(arguments, "relation_atlock_" + priorityString + "_id", ObjectNone);
			if (objectId == 0
				&& objectType != ObjectTypeLoco
				&& objectType != ObjectTypePause
				&& objectType != ObjectTypeMultipleUnit
				&& objectType != ObjectTypeBooster)
			{
				continue;
			}
			if (objectId == fromTrack && objectType == ObjectTypeTrack)
			{
				continue;
			}
			if (objectId == toTrack && objectType == ObjectTypeTrack)
			{
				continue;
			}
			unsigned short state = Utils::Utils::GetIntegerMapEntry(arguments, "relation_atlock_" + priorityString + "_state");
			relationsAtLock.push_back(new Relation(&manager,
				ObjectIdentifier(ObjectTypeRoute, routeID),
				ObjectIdentifier(objectType, objectId),
				Relation::RelationTypeRouteAtLock,
				priorityAtLock,
				state));
			++priorityAtLock;
		}

		vector<Relation*> relationsAtUnlock;
		Priority priorityAtUnlock = 1;
		for (Priority relationId = 1; relationId <= relationCountAtUnlock; ++relationId)
		{
			string priorityString = to_string(relationId);
			ObjectType objectType = static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "relation_atunlock_" + priorityString + "_type"));
			ObjectID objectId = Utils::Utils::GetIntegerMapEntry(arguments, "relation_atunlock_" + priorityString + "_id", ObjectNone);
			if (objectId == 0
				&& objectType != ObjectTypeLoco
				&& objectType != ObjectTypePause
				&& objectType != ObjectTypeMultipleUnit
				&& objectType != ObjectTypeBooster)
			{
				continue;
			}
			if (objectId == fromTrack && objectType == ObjectTypeTrack)
			{
				continue;
			}
			if (objectId == toTrack && objectType == ObjectTypeTrack)
			{
				continue;
			}
			unsigned char state = Utils::Utils::GetIntegerMapEntry(arguments, "relation_atunlock_" + priorityString + "_state");
			relationsAtUnlock.push_back(new Relation(&manager,
				ObjectIdentifier(ObjectTypeRoute, routeID),
				ObjectIdentifier(objectType, objectId),
				Relation::RelationTypeRouteAtUnlock,
				priorityAtUnlock,
				state));
			++priorityAtUnlock;
		}

		vector<Relation*> conditions;
		Priority priorityCounterConditions = 1;
		for (Priority relationId = 1; relationId <= relationCounterConditions; ++relationId)
		{
			string priorityString = to_string(relationId);
			ObjectType objectType = static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "relation_conditions_" + priorityString + "_type"));
			ObjectID objectId = Utils::Utils::GetIntegerMapEntry(arguments, "relation_conditions_" + priorityString + "_id", ObjectNone);
			if (objectId == 0
				&& objectType != ObjectTypeLoco
				&& objectType != ObjectTypePause
				&& objectType != ObjectTypeMultipleUnit
				&& objectType != ObjectTypeBooster)
			{
				continue;
			}
			if (objectId == fromTrack && objectType == ObjectTypeTrack)
			{
				continue;
			}
			if (objectId == toTrack && objectType == ObjectTypeTrack)
			{
				continue;
			}
			unsigned char state = Utils::Utils::GetIntegerMapEntry(arguments, "relation_conditions_" + priorityString + "_state");
			conditions.push_back(new Relation(&manager,
				ObjectIdentifier(ObjectTypeRoute, routeID),
				ObjectIdentifier(objectType, objectId),
				Relation::RelationTypeRouteConditions,
				priorityCounterConditions,
				state));
			++priorityCounterConditions;
		}

		string result;
		if (!manager.RouteSave(routeID,
			name,
			delay,
			pushpull,
			propulsion,
			trainType,
			mintrainlength,
			maxtrainlength,
			relationsAtLock,
			relationsAtUnlock,
			conditions,
			visible,
			posx,
			posy,
			posz,
			automode,
			fromTrack,
			fromOrientation,
			toTrack,
			toOrientation,
			speed,
			feedbackIdReduced,
			reducedDelay,
			feedbackIdCreep,
			creepDelay,
			feedbackIdStop,
			stopDelay,
			feedbackIdOver,
			waitAfterRelease,
			followUpRoute,
			result))
		{
			client.ReplyResponse(client.ResponseError, result);
			return;
		}
		client.ReplyResponse(client.ResponseInfo, Languages::TextRouteSaved, name);
	}

	void WebClientRoute::HandleRouteAskDelete(const map<string, string>& arguments)
	{
		RouteID routeID = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);

		if (routeID == RouteNone)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextRouteDoesNotExist);
			return;
		}

		const DataModel::Route* route = manager.GetRoute(routeID);
		if (route == nullptr)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextRouteDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& routeName = route->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteRoute));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, routeName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "routedelete"))
			.AddContent(HtmlTagInputHidden("route", to_string(routeID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientRoute::HandleRouteDelete(const map<string, string>& arguments)
	{
		RouteID routeID = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);
		const DataModel::Route* route = manager.GetRoute(routeID);
		if (route == nullptr)
		{
			client.ReplyResponse(client.ResponseError, Languages::TextRouteDoesNotExist);
			return;
		}

		string name = route->GetName();
		string result;
		if (!manager.RouteDelete(routeID, result))
		{
			client.ReplyResponse(client.ResponseError, result);
			return;
		}

		client.ReplyResponse(client.ResponseInfo, Languages::TextRouteDeleted, name);
	}

	void WebClientRoute::HandleRouteList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextRoutes));
		HtmlTag table("table");
		const map<string,DataModel::Route*> routeList = manager.RouteListByName();
		map<string,string> routeArgument;
		for (auto& route : routeList)
		{
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(route.first));
			const string& routeIdString = to_string(route.second->GetID());
			routeArgument["route"] = routeIdString;
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "routeedit_list_" + routeIdString, routeArgument)));
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "routeaskdelete_" + routeIdString, routeArgument)));
			if (route.second->IsInUse())
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonCommandWide(Languages::TextRelease, "routerelease_" + routeIdString, routeArgument, "hideElement('b_routerelease_" + routeIdString + "');")));
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "routeedit_0"));
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientRoute::HandleRouteExecute(const map<string, string>& arguments)
	{
		RouteID routeID = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);
		manager.RouteExecuteAsync(logger, routeID);
		client.ReplyHtmlWithHeaderAndParagraph("Route executed");
	}

	void WebClientRoute::HandleRouteRelease(const map<string, string>& arguments)
	{
		RouteID routeID = Utils::Utils::GetIntegerMapEntry(arguments, "route");
		bool ret = manager.RouteRelease(routeID);
		client.ReplyHtmlWithHeaderAndParagraph(ret ? "Route released" : "Route not released");
	}

	void WebClientRoute::HandleRelationAdd(const map<string, string>& arguments)
	{
		string priorityString = Utils::Utils::GetStringMapEntry(arguments, "priority", "1");
		string type = Utils::Utils::GetStringMapEntry(arguments, "type", "atlock");
		if ((type.compare("atunlock") != 0)
			&& (type.compare("conditions") != 0))
		{
			type = "atlock";
		}
		Priority priority = Utils::Integer::StringToInteger(priorityString, 1);
		HtmlTag container;
		container.AddChildTag(HtmlTagRelation(type, priority));
		container.AddChildTag(HtmlTag("div").AddId("new_" + type + "_priority_" + to_string(priority + 1)));
		client.ReplyHtmlWithHeader(container);
	}

	void WebClientRoute::HandleRelationObject(const map<string, string>& arguments)
	{
		const string priority = Utils::Utils::GetStringMapEntry(arguments, "priority");
		const string atlock = Utils::Utils::GetStringMapEntry(arguments, "atlock");
		const ObjectType objectType = static_cast<ObjectType>(Utils::Utils::GetIntegerMapEntry(arguments, "objecttype"));
		const ObjectID id = static_cast<ObjectID>(Utils::Utils::GetIntegerMapEntry(arguments, "id"));
		const DataModel::Relation::Data state = static_cast<DataModel::Relation::Data>(Utils::Utils::GetIntegerMapEntry(arguments, "state"));
		client.ReplyHtmlWithHeader(HtmlTagRelationObject(atlock, priority, objectType, id, state));
	}

	void WebClientRoute::HandleRelationSwitchStates(const map<string, string>& arguments)
	{
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const SwitchID switchId = static_cast<SwitchID>(Utils::Utils::GetIntegerMapEntry(arguments, "switch"));
		client.ReplyHtmlWithHeader(HtmlTagRelationSwitchState(name, switchId));
	}

	HtmlTag WebClientRoute::HtmlTagRelation(const string& atlock,
		const Priority priority,
		const ObjectType objectType,
	    const ObjectID objectId,
	    const DataModel::Relation::Data state)
	{
		HtmlTag content("div");
		string priorityString = to_string(priority);
		string name = "relation_" + atlock + "_" + priorityString;
		content.AddId(name);
		HtmlTagButton deleteButton(Languages::TextDelete, "delete_" + name);
		deleteButton.AddAttribute("onclick", "deleteElement('" + name + "');return false;");
		deleteButton.AddClass("wide_button");
		content.AddChildTag(deleteButton);

		if (priority > 1)
		{
			content.AddChildTag(HtmlTagButtonSwapRelation(atlock, priorityString, true));
		}
		else
		{
			content.AddChildTag(HtmlTagSpace().AddClass("small_button").AddClass("button_replacement"));
		}
		content.AddChildTag(HtmlTagButtonSwapRelation(atlock, priorityString, false));

		HtmlTag contentObject("div");
		contentObject.AddId(name + "_object");
		contentObject.AddClass("inline-block");
		contentObject.AddChildTag(HtmlTagRelationObject(atlock, priorityString, objectType, objectId, state));
		content.AddChildTag(contentObject);
		return content;
	}

	HtmlTag WebClientRoute::HtmlTagRelationSwitchState(const string& name,
		const SwitchID switchId,
		const DataModel::Relation::Data data)
	{
		map<DataModel::AccessoryState,Languages::TextSelector> stateOptions;
		Switch* mySwitch = manager.GetSwitch(switchId);
		if (mySwitch != nullptr)
		{
			stateOptions = mySwitch->GetStateOptions();
		}

		return HtmlTagSelect(name + "_state", stateOptions, static_cast<DataModel::AccessoryState>(data)).AddClass("select_relation_state");
	}

	HtmlTag WebClientRoute::HtmlTagRelationObject(const string& atlock,
		const string& priorityString,
		const ObjectType objectType,
		const ObjectID objectId,
		const DataModel::Relation::Data state)
	{
		const string name = "relation_" + atlock + "_" + priorityString;
		HtmlTag content;
		map<ObjectType, Languages::TextSelector> objectTypeOptions;
		objectTypeOptions[ObjectTypeAccessory] = Languages::TextAccessory;
		objectTypeOptions[ObjectTypeSignal] = Languages::TextSignal;
		objectTypeOptions[ObjectTypeSwitch] = Languages::TextSwitch;
		if (atlock.compare("conditions") != 0)
		{
			objectTypeOptions[ObjectTypeTrack] = Languages::TextTrack;
			objectTypeOptions[ObjectTypeRoute] = Languages::TextRoute;
			objectTypeOptions[ObjectTypeLoco] = Languages::TextLoco;
			objectTypeOptions[ObjectTypeMultipleUnit] = Languages::TextOrientation;
			objectTypeOptions[ObjectTypePause] = Languages::TextPause;
			objectTypeOptions[ObjectTypeBooster] = Languages::TextBooster;
			if (atlock.compare("atlock") == 0)
			{
				objectTypeOptions[ObjectTypeCounter] = Languages::TextCounter;
			}
		}
		HtmlTagSelect select(name + "_type", objectTypeOptions, objectType);
		select.AddClass("select_relation_objecttype");
		select.AddAttribute("onchange", "loadRelationObject('" + atlock + "', '" + priorityString + "');return false;");
		content.AddChildTag(select);

		switch (objectType)
		{
			case ObjectTypeSwitch:
			{
				std::map<string, AccessoryConfig> switches = manager.SwitchConfigByName();
				map<string, SwitchID> switchOptions;
				for (auto& mySwitch : switches)
				{
					switchOptions[mySwitch.first] = mySwitch.second.GetObjectIdentifier().GetObjectID();
				}
				SwitchID switchId = objectId;
				if (switchId == SwitchNone && switchOptions.size() > 0)
				{
					switchId = switchOptions.begin()->second;
				}
				HtmlTagSelect selectSwitch(name + "_id", switchOptions, switchId);
				selectSwitch.AddClass("select_relation_id");
				selectSwitch.AddAttribute("onchange", "loadRelationObjectStates('switch', '" + name + "', '" + to_string(switchId) + "');return false;");
				content.AddChildTag(selectSwitch);

				HtmlTag contentState("div");
				contentState.AddId(name + "_state");
				contentState.AddClass("inline-block");
				contentState.AddChildTag(HtmlTagRelationSwitchState(name, switchId, state));
				content.AddChildTag(contentState);
				return content;
			}

			case ObjectTypeSignal:
			{
				std::map<string, AccessoryConfig> signals = manager.SignalConfigByName();
				map<string, SignalID> signalOptions;
				for (auto& signal : signals)
				{
					signalOptions[signal.first] = signal.second.GetObjectIdentifier().GetObjectID();
				}
				SignalID signalId = objectId;
				if (signalId == SignalNone && signalOptions.size() > 0)
				{
					signalId = signalOptions.begin()->second;
				}
				HtmlTagSelect selectSignal(name + "_id", signalOptions, signalId);
				selectSignal.AddClass("select_relation_id");
				selectSignal.AddAttribute("onchange", "loadRelationObjectStates('signal', '" + name + "', '" + to_string(signalId) + "');return false;");
				content.AddChildTag(selectSignal);

				HtmlTag contentState("div");
				contentState.AddId(name + "_state");
				contentState.AddClass("inline-block");
				contentState.AddChildTag(client.HtmlTagRelationSignalState(name, signalId, state));
				content.AddChildTag(contentState);
				return content;
			}

			case ObjectTypeAccessory:
			{
				std::map<string, AccessoryConfig> accessories = manager.AccessoryConfigByName();
				map<string, AccessoryID> accessoryOptions;
				for (auto& accessory : accessories)
				{
					accessoryOptions[accessory.first] = accessory.second.GetObjectIdentifier().GetObjectID();
				}
				content.AddChildTag(HtmlTagSelect(name + "_id", accessoryOptions, objectId).AddClass("select_relation_id"));

				map<DataModel::AccessoryState,Languages::TextSelector> stateOptions;
				stateOptions[DataModel::AccessoryStateOn] = Languages::TextOn;
				stateOptions[DataModel::AccessoryStateOff] = Languages::TextOff;
				content.AddChildTag(HtmlTagSelect(name + "_state", stateOptions, static_cast<DataModel::AccessoryState>(state)).AddClass("select_relation_state"));
				return content;
			}

			case ObjectTypeTrack:
			{
				std::map<string, Track*> tracks = manager.TrackListMasterByName();
				map<string, TrackID> trackOptions;
				for (auto& track : tracks)
				{
					trackOptions[track.first] = track.second->GetID();
				}
				content.AddChildTag(HtmlTagSelect(name + "_id", trackOptions, objectId).AddClass("select_relation_id"));

				content.AddChildTag(HtmlTagSelectOrientation(name + "_state", static_cast<Orientation>(state)).AddClass("select_relation_state"));
				return content;
			}

			case ObjectTypeRoute:
			{
				std::map<string, Route*> routes = manager.RouteListByName();
				map<string, RouteID> routeOptions;
				for (auto& route : routes)
				{
					routeOptions[route.first] = route.second->GetID();
				}
				content.AddChildTag(HtmlTagSelect(name + "_id", routeOptions, objectId).AddClass("select_relation_id"));
				return content;
			}

			case ObjectTypeLoco:
			{
				map<string,string> functionOptions;
				for (DataModel::LocoFunctionNr function = 0; function < NumberOfLocoFunctions; ++function)
				{
					functionOptions[Utils::Utils::ToStringWithLeadingZeros(function, 3)] = "F" + to_string(function);
				}

				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconShuntingMode, 3)] = Languages::GetText(Languages::TextLocoFunctionIconShuntingMode);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconInertia, 3)] = Languages::GetText(Languages::TextLocoFunctionIconInertia);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconLight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconLight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconHeadlightLowBeamForward, 3)] = Languages::GetText(Languages::TextLocoFunctionIconHeadlightLowBeamForward);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconHeadlightLowBeamReverse, 3)] = Languages::GetText(Languages::TextLocoFunctionIconHeadlightLowBeamReverse);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconHeadlightHighBeamForward, 3)] = Languages::GetText(Languages::TextLocoFunctionIconHeadlightHighBeamForward);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconHeadlightHighBeamReverse, 3)] = Languages::GetText(Languages::TextLocoFunctionIconHeadlightHighBeamReverse);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBacklightForward, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBacklightForward);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBacklightReverse, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBacklightReverse);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconShuntingLight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconShuntingLight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBlinkingLight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBlinkingLight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconInteriorLight1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconInteriorLight1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconInteriorLight2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconInteriorLight2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTableLight1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTableLight1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTableLight2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTableLight2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTableLight3, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTableLight3);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCabLight1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCabLight1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCabLight2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCabLight2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCabLight12, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCabLight12);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconDriversDeskLight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconDriversDeskLight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTrainDestinationIndicator, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTrainDestinationIndicator);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconLocomotiveNumberIndicator, 3)] = Languages::GetText(Languages::TextLocoFunctionIconLocomotiveNumberIndicator);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconEngineLight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconEngineLight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFireBox, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFireBox);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconStairsLight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconStairsLight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSmokeGenerator, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSmokeGenerator);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTelex1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTelex1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTelex2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTelex2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTelex12, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTelex12);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconPanto1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconPanto1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconPanto2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconPanto2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconPanto12, 3)] = Languages::GetText(Languages::TextLocoFunctionIconPanto12);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconUp, 3)] = Languages::GetText(Languages::TextLocoFunctionIconUp);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconDown, 3)] = Languages::GetText(Languages::TextLocoFunctionIconDown);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconUpDown1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconUpDown1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconUpDown2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconUpDown2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconLeft, 3)] = Languages::GetText(Languages::TextLocoFunctionIconLeft);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconRight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconRight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconLeftRight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconLeftRight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTurnLeft, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTurnLeft);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTurnRight, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTurnRight);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconTurn, 3)] = Languages::GetText(Languages::TextLocoFunctionIconTurn);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCrane, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCrane);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconMagnet, 3)] = Languages::GetText(Languages::TextLocoFunctionIconMagnet);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCraneHook, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCraneHook);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFan, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFan);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBreak, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBreak);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconNoSound, 3)] = Languages::GetText(Languages::TextLocoFunctionIconNoSound);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSoundGeneral, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSoundGeneral);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconRunning1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconRunning1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconRunning2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconRunning2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconEngine1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconEngine1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconEngine2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconEngine2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBreak1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBreak1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBreak2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBreak2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCurve, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCurve);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconHorn1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconHorn1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconHorn2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconHorn2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconWhistle1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconWhistle1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconWhistle2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconWhistle2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBell, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBell);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconStationAnnouncement1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconStationAnnouncement1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconStationAnnouncement2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconStationAnnouncement2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconStationAnnouncement3, 3)] = Languages::GetText(Languages::TextLocoFunctionIconStationAnnouncement3);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSpeak, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSpeak);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconRadio, 3)] = Languages::GetText(Languages::TextLocoFunctionIconRadio);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconMusic1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconMusic1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconMusic2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconMusic2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconOpenDoor, 3)] = Languages::GetText(Languages::TextLocoFunctionIconOpenDoor);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCloseDoor, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCloseDoor);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFan1, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFan1);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFan2, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFan2);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFan3, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFan3);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconShovelCoal, 3)] = Languages::GetText(Languages::TextLocoFunctionIconShovelCoal);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCompressedAir, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCompressedAir);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconReliefValve, 3)] = Languages::GetText(Languages::TextLocoFunctionIconReliefValve);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSteamBlowOut, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSteamBlowOut);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSteamBlow, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSteamBlow);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconDrainValve, 3)] = Languages::GetText(Languages::TextLocoFunctionIconDrainValve);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconShakingRust, 3)] = Languages::GetText(Languages::TextLocoFunctionIconShakingRust);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconAirPump, 3)] = Languages::GetText(Languages::TextLocoFunctionIconAirPump);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconWaterPump, 3)] = Languages::GetText(Languages::TextLocoFunctionIconWaterPump);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconBufferPush, 3)] = Languages::GetText(Languages::TextLocoFunctionIconBufferPush);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconGenerator, 3)] = Languages::GetText(Languages::TextLocoFunctionIconGenerator);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconGearBox, 3)] = Languages::GetText(Languages::TextLocoFunctionIconGearBox);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconGearUp, 3)] = Languages::GetText(Languages::TextLocoFunctionIconGearUp);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconGearDown, 3)] = Languages::GetText(Languages::TextLocoFunctionIconGearDown);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFillWater, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFillWater);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFillDiesel, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFillDiesel);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconFillGas, 3)] = Languages::GetText(Languages::TextLocoFunctionIconFillGas);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSand, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSand);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconRailJoint, 3)] = Languages::GetText(Languages::TextLocoFunctionIconRailJoint);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconCoupler, 3)] = Languages::GetText(Languages::TextLocoFunctionIconCoupler);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconPanto, 3)] = Languages::GetText(Languages::TextLocoFunctionIconPanto);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconMainSwitch, 3)] = Languages::GetText(Languages::TextLocoFunctionIconMainSwitch);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSoundLouder, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSoundLouder);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconSoundLower, 3)] = Languages::GetText(Languages::TextLocoFunctionIconSoundLower);
				functionOptions[Utils::Utils::ToStringWithLeadingZeros(256 + LocoFunctionIconNoBreak, 3)] = Languages::GetText(Languages::TextLocoFunctionIconNoBreak);

				content.AddChildTag(HtmlTagSelect(name + "_id", functionOptions, Utils::Utils::ToStringWithLeadingZeros(objectId, 3)).AddClass("select_relation_id"));

				// FIXME: load available functions of loco
				map<DataModel::LocoFunctionState,string> stateOptions;
				stateOptions[DataModel::LocoFunctionStateOff] = Languages::GetText(Languages::TextOff);
				stateOptions[DataModel::LocoFunctionStateOn] = Languages::GetText(Languages::TextOn);
				stateOptions[DataModel::LocoFunctionState0s5] = "0.5s";
				stateOptions[DataModel::LocoFunctionState1s0] = "1s";
				stateOptions[DataModel::LocoFunctionState1s5] = "1.5s";
				stateOptions[DataModel::LocoFunctionState2s0] = "2s";
				content.AddChildTag(HtmlTagSelect(name + "_state", stateOptions, static_cast<DataModel::LocoFunctionState>(state)).AddClass("select_relation_state"));
				return content;
			}

			case ObjectTypePause:
			{
				map<unsigned int,string> time;
				time[1u] = "0.1s";
				time[2u] = "0.2s";
				time[3u] = "0.3s";
				time[4u] = "0.4s";
				time[5u] = "0.5s";
				time[6u] = "0.6s";
				time[7u] = "0.7s";
				time[8u] = "0.8s";
				time[9u] = "0.9s";
				time[10u] = "1s";
				time[15u] = "1.5s";
				time[20u] = "2s";
				time[25u] = "2.5s";
				time[30u] = "3s";
				time[40u] = "4s";
				time[50u] = "5s";
				time[60u] = "6s";
				time[70u] = "7s";
				time[80u] = "8s";
				time[90u] = "9s";
				time[100u] = "10s";
				content.AddChildTag(HtmlTagSelect(name + "_state", time, static_cast<unsigned int>(state)).AddClass("select_relation_state"));
				return content;
			}

			case ObjectTypeMultipleUnit: // abused for loco orientation
			{
				map<unsigned int,string> orientation;
				orientation[0] = Languages::Languages::GetText(Languages::Languages::TextLeft);
				orientation[1u] = Languages::Languages::GetText(Languages::Languages::TextRight);
				orientation[2u] = Languages::Languages::GetText(Languages::Languages::TextChange);;
				content.AddChildTag(HtmlTagSelect(name + "_state", orientation, static_cast<unsigned int>(state)).AddClass("select_relation_state"));
				return content;
			}

			case ObjectTypeBooster:
			{
				map<unsigned int,string> booster;
				booster[0] = Languages::Languages::GetText(Languages::Languages::TextOff);
				booster[1u] = Languages::Languages::GetText(Languages::Languages::TextOn);
				content.AddChildTag(HtmlTagSelect(name + "_state", booster, static_cast<unsigned int>(state)).AddClass("select_relation_state"));
				return content;
			}

			case ObjectTypeCounter:
			{
				std::map<string, Counter*> counters = manager.CounterListByName();
				map<string, CounterID> counterOptions;
				for (auto& counter : counters)
				{
					counterOptions[counter.first] = counter.second->GetID();
				}
				CounterID counterId = objectId;
				if ((counterId == CounterNone) && (counterOptions.size() > 0))
				{
					counterId = counterOptions.begin()->second;
				}
				HtmlTagSelect selectCounter(name + "_id", counterOptions, counterId);
				selectCounter.AddClass("select_relation_id");
				selectCounter.AddAttribute("onchange", "loadRelationObjectStates('counter', '" + name + "', '" + to_string(counterId) + "');return false;");
				content.AddChildTag(selectCounter);

				map<unsigned int,string> countOptions;
				countOptions[0] = Languages::Languages::GetText(Languages::Languages::TextIncrement);
				countOptions[1u] = Languages::Languages::GetText(Languages::Languages::TextDecrement);
				content.AddChildTag(HtmlTagSelect(name + "_state", countOptions, static_cast<unsigned int>(state)).AddClass("select_relation_state"));
				return content;
			}

			default:
			{
				content.AddContent(Languages::TextUnknownObjectType);
				return content;
			}
		}
	}

	HtmlTag WebClientRoute::HtmlTagSelectTrack(const std::string& name,
		const Languages::TextSelector label,
		const TrackID trackID,
		const Orientation orientation,
		const string& onchange) const
	{
		HtmlTag tag;
		map<string,TrackID> tracks = manager.TrackListIdByName();
		HtmlTagSelectWithLabel selectTrack(name + "track", label, tracks, trackID);
		selectTrack.AddClass("select_track");
		if (onchange.size() > 0)
		{
			selectTrack.AddAttribute("onchange", onchange);
		}
		tag.AddChildTag(selectTrack);
		tag.AddChildTag(HtmlTagSelectOrientation(name + "orientation", orientation).AddClass("select_orientation"));
		return tag;
	}

	HtmlTag WebClientRoute::HtmlTagSelectFeedbacksOfTrack(const TrackID trackID,
		const RouteID followUpRoute,
		const FeedbackID feedbackIdReduced,
		const Delay reducedDelay,
		const FeedbackID feedbackIdCreep,
		const Delay creepDelay,
		const FeedbackID feedbackIdStop,
		const Delay stopDelay,
		const FeedbackID feedbackIdOver) const
	{
		HtmlTag tag;
		map<RouteID,string> followUpRouteOptions = manager.RoutesOfTrack(trackID);
		followUpRouteOptions[RouteStop] = Languages::Languages::GetText(Languages::TextNone);
		followUpRouteOptions[RouteAuto] = Languages::Languages::GetText(Languages::TextSelectAutomatically);
		tag.AddChildTag(HtmlTagSelectWithLabel("followuproute", Languages::TextFollowUpRoute, followUpRouteOptions, followUpRoute));

		map<string,FeedbackID> feedbacks = manager.FeedbacksOfTrack(trackID);
		map<string,FeedbackID> feedbacksWithNone = feedbacks;
		feedbacksWithNone["-"] = FeedbackNone;
		map<string,Delay> delayOptions;
		delayOptions["0.0s"] = 0;
		delayOptions["0.1s"] = 1;
		delayOptions["0.2s"] = 2;
		delayOptions["0.3s"] = 3;
		delayOptions["0.4s"] = 4;
		delayOptions["0.5s"] = 5;
		delayOptions["0.6s"] = 6;
		delayOptions["0.7s"] = 7;
		delayOptions["0.8s"] = 8;
		delayOptions["0.9s"] = 9;
		delayOptions["1.0s"] = 10;
		delayOptions["1.5s"] = 15;
		delayOptions["2.0s"] = 20;
		delayOptions["2.5s"] = 25;
		delayOptions["3.0s"] = 30;
		delayOptions["3.5s"] = 35;
		delayOptions["4.0s"] = 40;
		delayOptions["4.5s"] = 45;
		delayOptions["5.0s"] = 50;
		delayOptions["6.0s"] = 60;
		delayOptions["7.0s"] = 70;
		delayOptions["8.0s"] = 80;
		delayOptions["9.0s"] = 90;

		tag.AddChildTag(HtmlTagSelectWithLabel("feedbackreduced", Languages::TextReducedSpeedAt, feedbacksWithNone, feedbackIdReduced).AddClass("select_feedback"));
		tag.AddChildTag(HtmlTagSelect("reduceddelay", delayOptions, reducedDelay).AddClass("select_delay"));
		tag.AddChildTag(HtmlTagSelectWithLabel("feedbackcreep", Languages::TextCreepAt, feedbacksWithNone, feedbackIdCreep).AddClass("select_feedback"));
		tag.AddChildTag(HtmlTagSelect("creepdelay", delayOptions, creepDelay).AddClass("select_delay"));
		tag.AddChildTag(HtmlTagSelectWithLabel("feedbackstop", Languages::TextStopAt, feedbacks, feedbackIdStop).AddClass("select_feedback"));
		tag.AddChildTag(HtmlTagSelect("stopdelay", delayOptions, stopDelay).AddClass("select_delay"));
		tag.AddChildTag(HtmlTagSelectWithLabel("feedbackover", Languages::TextOverrunAt, feedbacksWithNone, feedbackIdOver).AddClass("select_feedback"));
		return tag;
	}

	void WebClientRoute::HandleFeedbacksOfTrack(const map<string, string>& arguments)
	{
		const TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
		client.ReplyHtmlWithHeader(HtmlTagSelectFeedbacksOfTrack(trackID));
	}
}} // namespace Server::Web
