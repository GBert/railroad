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

#include <vector>

#include "DataModel/ObjectIdentifier.h"
#include "DataModel/Signal.h"
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
#include "Server/Web/HtmlTagSelectOrientationWithLabel.h"
#include "Server/Web/HtmlTagSignal.h"
#include "Server/Web/WebClient.h"
#include "Server/Web/WebClientCluster.h"
#include "Server/Web/WebClientSignal.h"
#include "Server/Web/WebClientStatic.h"

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
	void WebClientSignal::HandleSignalEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		SignalID signalID = Utils::Utils::GetIntegerMapEntry(arguments, "signal", SignalNone);
		ControlID controlId = manager.GetPossibleControlForAccessory();
		if (controlId == ControlNone)
		{
			controlId = manager.GetPossibleControlForAccessory();
		}
		string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		Protocol protocol = ProtocolNone;
		Address address = AddressDefault;
		Address serverAddress = AddressNone;
		string name = Languages::GetText(Languages::TextNew);
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", LayerUndeletable);
		LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		DataModel::AccessoryType signalType = DataModel::SignalTypeSimpleLeft;
		DataModel::AccessoryPulseDuration duration = manager.GetDefaultAccessoryDuration();
		bool inverted = false;

		if (signalID > SignalNone)
		{
			const DataModel::Signal* signal = manager.GetSignal(signalID);
			if (signal != nullptr)
			{
				controlId = signal->GetControlID();
				matchKey = signal->GetMatchKey();
				protocol = signal->GetProtocol();
				address = signal->GetAddress();
				serverAddress = signal->GetServerAddress();
				name = signal->GetName();
				posx = signal->GetPosX();
				posy = signal->GetPosY();
				posz = signal->GetPosZ();
				rotation = signal->GetRotation();
				signalType = signal->GetAccessoryType();
				duration = signal->GetAccessoryPulseDuration();
				inverted = signal->GetInverted();
			}
		}
		else if (controlId > ControlNone)
		{
			// signal from hardware database
			const DataModel::AccessoryConfig signalConfig = manager.GetAccessoryOfConfigByMatchKey(controlId, matchKey);
			if (signalConfig.GetControlId() == controlId && signalConfig.GetMatchKey() == matchKey)
			{
				protocol = signalConfig.GetProtocol();
				address = signalConfig.GetAddress();
				name = signalConfig.GetName();
			}
		}
		// else new signal

		std::map<DataModel::AccessoryType, Languages::TextSelector> signalTypeOptions;
		signalTypeOptions[DataModel::SignalTypeSimpleLeft] = Languages::TextSimpleLeft;
		signalTypeOptions[DataModel::SignalTypeSimpleRight] = Languages::TextSimpleRight;
		signalTypeOptions[DataModel::SignalTypeChLMain] = Languages::TextChLMain;
		signalTypeOptions[DataModel::SignalTypeChLDistant] = Languages::TextChLDistant;
		signalTypeOptions[DataModel::SignalTypeChDwarf] = Languages::TextChDwarf;
		signalTypeOptions[DataModel::SignalTypeChNMain] = Languages::TextChNMain;
		//signalTypeOptions[DataModel::SignalTypeChNDistant] = Languages::TextChNDistant;
		signalTypeOptions[DataModel::SignalTypeDeCombined] = Languages::TextDeCombined;
		signalTypeOptions[DataModel::SignalTypeDeHVMain] = Languages::TextDeHVMain;
		//signalTypeOptions[DataModel::SignalTypeDeHVDistant] = Languages::TextDeHVDistant;
		signalTypeOptions[DataModel::SignalTypeDeBlock] = Languages::TextDeBlock;

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("main", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("address", Languages::TextAddresses).AddAttribute("onclick", "onClickAddresses(" + to_string(signalID) + ");"));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("position", Languages::TextPosition));

		content.AddChildTag(tabMenu);

		HtmlTag formContent;
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "signalsave"));
		formContent.AddChildTag(HtmlTagInputHidden("signal", to_string(signalID)));

		HtmlTag mainContent("div");
		mainContent.AddId("tab_main");
		mainContent.AddClass("tab_content");
		mainContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		mainContent.AddChildTag(HtmlTagSelectWithLabel("signaltype", Languages::TextType, signalTypeOptions, signalType));
		mainContent.AddChildTag(client.HtmlTagControlAccessory(controlId, "signal", signalID));
		mainContent.AddChildTag(HtmlTag("div").AddId("select_protocol").AddChildTag(client.HtmlTagMatchKeyProtocolAccessory(controlId, matchKey, protocol)));
		mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("address", Languages::TextBaseAddress, address, 1, 2044));
		mainContent.AddChildTag(WebClientStatic::HtmlTagDuration(duration));
		mainContent.AddChildTag(HtmlTagInputCheckboxWithLabel("inverted", Languages::TextInverted, "true", inverted));
		if (manager.IsServerEnabled())
		{
			mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("serveraddress", Languages::TextServerAddress, serverAddress, 0, 2044));
		}
		formContent.AddChildTag(mainContent);

		HtmlTag addressContent("div");
		addressContent.AddId("tab_address");
		addressContent.AddClass("tab_content");
		addressContent.AddClass("hidden");
		addressContent.AddChildTag(HtmlTag("div").AddId("addresses"));
		formContent.AddChildTag(addressContent);

		formContent.AddChildTag(client.HtmlTagTabPosition(posx, posy, posz, rotation));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(HtmlTag("form").AddId("editform").AddChildTag(formContent)));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientSignal::HandleSignalAddresses(const map<string, string>& arguments)
	{
		Signal signalDummy(&manager, SignalNone);
		AccessoryType type = static_cast<AccessoryType>(Utils::Utils::GetIntegerMapEntry(arguments, "type"));
		SignalID signalId = Utils::Utils::GetIntegerMapEntry(arguments, "signal", SignalNone);
		Signal* signal = manager.GetSignal(signalId);
		if (signal == nullptr || signal->GetAccessoryType() != type)
		{
			signalDummy.SetAccessoryType(type);
			signal = &signalDummy;
		}

		Address address = Utils::Utils::GetIntegerMapEntry(arguments, "address", AddressNone);

		const std::map<DataModel::AccessoryState,DataModel::Signal::StateOption> stateOptions = signal->GetStateOptions();

		map<AddressOffset,string> selectAddressOptions;
		selectAddressOptions[-1] = "-";
		for (AddressOffset i = 0; i < static_cast<AddressOffset>(stateOptions.size()); ++i)
		{
			selectAddressOptions[i] = to_string(address + (i >> 1)) + " " + Languages::GetText(i & 0x01 ? Languages::TextGreen : Languages::TextRed);
		}

		HtmlTag addressContent;
		for (auto& stateOption : stateOptions)
		{
			AccessoryState state = stateOption.first;
			addressContent.AddChildTag(HtmlTagSelectWithLabel("address" + to_string(state),
				HtmlTagSignal::GetSignalImage(state, signal) + Languages::GetText(stateOption.second),
				selectAddressOptions,
				signal->GetStateAddressOffset(state)
			));
		}
		client.ReplyHtmlWithHeader(addressContent);
	}

	void WebClientSignal::HandleSignalSave(const map<string, string>& arguments)
	{
		const SignalID signalID = Utils::Utils::GetIntegerMapEntry(arguments, "signal", SignalNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		const string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		const Protocol protocol = static_cast<Protocol>(Utils::Utils::GetIntegerMapEntry(arguments, "protocol", ProtocolNone));
		const Address address = Utils::Utils::GetIntegerMapEntry(arguments, "address", AddressDefault);
		const Address serverAddress = Utils::Utils::GetIntegerMapEntry(arguments, "serveraddress", AddressNone);
		const LayoutPosition posX = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posY = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posZ = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		const LayoutItemSize height = Utils::Utils::GetIntegerMapEntry(arguments, "length", 1);
		const LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		const DataModel::AccessoryType signalType = static_cast<DataModel::AccessoryType>(Utils::Utils::GetIntegerMapEntry(arguments, "signaltype", DataModel::SignalTypeSimpleLeft));
		std::map<AccessoryState,AddressOffset> offsets;
		for (AddressOffset offset = 0; offset <= SignalStateMax; ++offset)
		{
			const AddressOffset address = Utils::Utils::GetIntegerMapEntry(arguments, "address" + to_string(offset), -1);
			if (address >= 0)
			{
				offsets[static_cast<AccessoryState>(offset)] = address;
			}
		}
		const DataModel::AccessoryPulseDuration duration = Utils::Utils::GetIntegerMapEntry(arguments, "duration", manager.GetDefaultAccessoryDuration());
		const bool inverted = Utils::Utils::GetBoolMapEntry(arguments, "inverted");
		string result;
		if (!manager.SignalSave(signalID,
			name,
			posX,
			posY,
			posZ,
			height,
			rotation,
			controlId,
			matchKey,
			protocol,
			address,
			serverAddress,
			signalType,
			offsets,
			duration,
			inverted,
			result))
		{
			client.ReplyResponse(WebClient::ResponseError, result);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextSignalSaved, name);
	}

	void WebClientSignal::HandleSignalState(const map<string, string>& arguments)
	{
		SignalID signalID = Utils::Utils::GetIntegerMapEntry(arguments, "signal", SignalNone);
		string signalStateText = Utils::Utils::GetStringMapEntry(arguments, "state", "stop");
		DataModel::AccessoryState signalState = DataModel::SignalStateStop;
		if (signalStateText.compare("clear") == 0)
		{
			signalState = DataModel::SignalStateClear;
		}
		else if (signalStateText.compare("aspect2") == 0)
		{
			signalState = DataModel::SignalStateAspect2;
		}
		else if (signalStateText.compare("aspect3") == 0)
		{
			signalState = DataModel::SignalStateAspect3;
		}
		else if (signalStateText.compare("aspect4") == 0)
		{
			signalState = DataModel::SignalStateAspect4;
		}
		else if (signalStateText.compare("aspect5") == 0)
		{
			signalState = DataModel::SignalStateAspect5;
		}
		else if (signalStateText.compare("aspect6") == 0)
		{
			signalState = DataModel::SignalStateAspect6;
		}
		else if (signalStateText.compare("aspect7") == 0)
		{
			signalState = DataModel::SignalStateAspect7;
		}
		else if (signalStateText.compare("aspect8") == 0)
		{
			signalState = DataModel::SignalStateAspect8;
		}
		else if (signalStateText.compare("aspect9") == 0)
		{
			signalState = DataModel::SignalStateAspect9;
		}
		else if (signalStateText.compare("aspect10") == 0)
		{
			signalState = DataModel::SignalStateAspect10;
		}
		else if (signalStateText.compare("dark") == 0)
		{
			signalState = DataModel::SignalStateDark;
		}
		else if (signalStateText.compare("stopexpected") == 0)
		{
			signalState = DataModel::SignalStateStopExpected;
		}
		else if (signalStateText.compare("clearexpected") == 0)
		{
			signalState = DataModel::SignalStateClearExpected;
		}
		else if (signalStateText.compare("aspect2expected") == 0)
		{
			signalState = DataModel::SignalStateAspect2Expected;
		}
		else if (signalStateText.compare("aspect3expected") == 0)
		{
			signalState = DataModel::SignalStateAspect3Expected;
		}
		else if (signalStateText.compare("aspect4expected") == 0)
		{
			signalState = DataModel::SignalStateAspect4Expected;
		}
		else if (signalStateText.compare("aspect5expected") == 0)
		{
			signalState = DataModel::SignalStateAspect5Expected;
		}
		else if (signalStateText.compare("aspect6expected") == 0)
		{
			signalState = DataModel::SignalStateAspect6Expected;
		}
		else if (signalStateText.compare("aspect7expected") == 0)
		{
			signalState = DataModel::SignalStateAspect7Expected;
		}
		else if (signalStateText.compare("aspect8expected") == 0)
		{
			signalState = DataModel::SignalStateAspect8Expected;
		}
		else if (signalStateText.compare("aspect9expected") == 0)
		{
			signalState = DataModel::SignalStateAspect9Expected;
		}
		else if (signalStateText.compare("aspect10expected") == 0)
		{
			signalState = DataModel::SignalStateAspect10Expected;
		}

		manager.SignalState(ControlTypeWebServer, signalID, signalState, false);

		client.ReplyHtmlWithHeaderAndParagraph(signalState ? Languages::TextSignalStateIsClear : Languages::TextSignalStateIsStop, manager.GetSignalName(signalID));
	}

	void WebClientSignal::HandleSignalList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextSignals));
		HtmlTag table("table");
		const map<string,DataModel::AccessoryConfig> signalList = manager.SignalConfigByName();
		map<string,string> signalArgument;
		for (auto& signal : signalList)
		{
			const AccessoryConfig& signalConfig = signal.second;
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(signal.first));
			row.AddChildTag(HtmlTag("td").AddContent(client.ProtocolName(signalConfig.GetProtocol())));
			row.AddChildTag(HtmlTag("td").AddContent(to_string(signalConfig.GetAddress())));
			const SignalID signalId = signalConfig.GetObjectIdentifier().GetObjectID();
			const string& signalIdString = to_string(signalId);
			signalArgument["signal"] = signalIdString;
			if (signalId == SwitchNone)
			{
				signalArgument["control"] = to_string(signalConfig.GetControlId());
				signalArgument["matchkey"] = signalConfig.GetMatchKey();
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextImport, "signaledit_list_" + signalIdString, signalArgument)));
			}
			else
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "signaledit_list_" + signalIdString, signalArgument)));
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "signalaskdelete_" + signalIdString, signalArgument)));
				if (signalConfig.IsInUse())
				{
					row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonCommandWide(Languages::TextRelease, "signalrelease_" + signalIdString, signalArgument, "hideElement('b_signalrelease_" + signalIdString + "');")));
				}
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "signaledit_0"));
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientSignal::HandleSignalAskDelete(const map<string, string>& arguments)
	{
		SignalID signalID = Utils::Utils::GetIntegerMapEntry(arguments, "signal", SignalNone);

		if (signalID == SignalNone)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextSignalDoesNotExist);
			return;
		}

		const DataModel::Signal* signal = manager.GetSignal(signalID);
		if (signal == nullptr)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextSignalDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& signalName = signal->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteSignal));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, signalName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "signaldelete"))
			.AddContent(HtmlTagInputHidden("signal", to_string(signalID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientSignal::HandleSignalDelete(const map<string, string>& arguments)
	{
		SignalID signalID = Utils::Utils::GetIntegerMapEntry(arguments, "signal", SignalNone);
		const DataModel::Signal* signal = manager.GetSignal(signalID);
		if (signal == nullptr)
		{
			client.ReplyResponse(WebClient::ResponseError, Languages::TextSignalDoesNotExist);
			return;
		}

		string name = signal->GetName();
		string result;
		if (!manager.SignalDelete(signalID, result))
		{
			client.ReplyResponse(WebClient::ResponseError, result);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextSignalDeleted, name);
	}

	void WebClientSignal::HandleSignalGet(const map<string, string>& arguments)
	{
		SignalID signalID = Utils::Utils::GetIntegerMapEntry(arguments, "signal");
		const DataModel::Signal* signal = manager.GetSignal(signalID);
		if (signal == nullptr)
		{
			client.ReplyHtmlWithHeader(HtmlTag());
			return;
		}
		client.ReplyHtmlWithHeader(HtmlTagSignal(manager, signal));
	}

	void WebClientSignal::HandleSignalRelease(const map<string, string>& arguments)
	{
		const SignalID signalID = static_cast<SignalID>(Utils::Utils::GetIntegerMapEntry(arguments, "signal"));
		const bool ret = manager.SignalRelease(signalID);
		client.ReplyHtmlWithHeaderAndParagraph(ret ? "Signal released" : "Signal not released");
	}

	void WebClientSignal::HandleSignalStates(const map<string, string>& arguments)
	{
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const SignalID signalId = static_cast<SignalID>(Utils::Utils::GetIntegerMapEntry(arguments, "signal"));
		client.ReplyHtmlWithHeader(client.HtmlTagRelationSignalState(name, signalId));
	}
}} // namespace Server::Web
