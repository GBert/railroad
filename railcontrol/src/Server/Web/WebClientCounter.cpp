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

#include "DataModel/Counter.h"
#include "Utils/Utils.h"
#include "Server/Web/HtmlTag.h"
#include "Server/Web/HtmlTagButtonCancel.h"
#include "Server/Web/HtmlTagButtonCommandWide.h"
#include "Server/Web/HtmlTagButtonOK.h"
#include "Server/Web/HtmlTagButtonPopupWide.h"
#include "Server/Web/HtmlTagInputHidden.h"
#include "Server/Web/HtmlTagInputIntegerWithLabel.h"
#include "Server/Web/HtmlTagInputTextWithLabel.h"
#include "Server/Web/HtmlTagCounter.h"
#include "Server/Web/WebClient.h"
#include "Server/Web/WebClientCounter.h"

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
	void WebClientCounter::HandleCounterEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		const CounterID counterID = Utils::Utils::GetIntegerMapEntry(arguments, "counter", CounterNone);
		string name = Languages::GetText(Languages::TextNew);
		int max = Utils::Utils::GetIntegerMapEntry(arguments, "max", 1);
		int min = Utils::Utils::GetIntegerMapEntry(arguments, "min", 0);
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", LayerUndeletable);
		LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);

		if (counterID > CounterNone)
		{
			const DataModel::Counter* counter = manager.GetCounter(counterID);
			if (counter != nullptr)
			{
				name = counter->GetName();
				max = counter->GetMax();
				min = counter->GetMin();
				posx = counter->GetPosX();
				posy = counter->GetPosY();
				posz = counter->GetPosZ();
				rotation = counter->GetRotation();
			}
		}

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("main", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("position", Languages::TextPosition));

		content.AddChildTag(tabMenu);

		HtmlTag formContent;
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "countersave"));
		formContent.AddChildTag(HtmlTagInputHidden("counter", to_string(counterID)));

		HtmlTag mainContent("div");
		mainContent.AddId("tab_main");
		mainContent.AddClass("tab_content");
		mainContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("max", Languages::TextMax, max, 0, 255));
		mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("min", Languages::TextMin, min, -255, 0));
		formContent.AddChildTag(mainContent);

		formContent.AddChildTag(client.HtmlTagTabPosition(posx, posy, posz, rotation));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(HtmlTag("form").AddId("editform").AddChildTag(formContent)));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientCounter::HandleCounterSave(const map<string, string>& arguments)
	{
		const CounterID counterID = Utils::Utils::GetIntegerMapEntry(arguments, "counter", CounterNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const int max = Utils::Utils::GetIntegerMapEntry(arguments, "max", 1);
		const int min = Utils::Utils::GetIntegerMapEntry(arguments, "min", 0);
		const LayoutPosition posX = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posY = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posZ = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		const LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		string result;
		if (!manager.CounterSave(counterID,
			name,
			max,
			min,
			posX,
			posY,
			posZ,
			rotation,
			result))
		{
			client.ReplyResponse(WebClient::ResponseError, result);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextCounterSaved, name);
	}

	void WebClientCounter::HandleCounterList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextCounters));
		HtmlTag table("table");
		const map<string,DataModel::Counter*> counterList = manager.CounterListByName();
		map<string,string> counterArgument;
		for (auto& counter : counterList)
		{
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(counter.first));
			const string& counterIdString = to_string(counter.second->GetID());
			counterArgument["counter"] = counterIdString;
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "counteredit_list_" + counterIdString, counterArgument)));
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "counteraskdelete_" + counterIdString, counterArgument)));
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "counteredit_0"));
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientCounter::HandleCounterAskDelete(const map<string, string>& arguments)
	{
		const CounterID counterID = Utils::Utils::GetIntegerMapEntry(arguments, "counter", CounterNone);
		if (counterID == CounterNone)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextCounterDoesNotExist);
			return;
		}

		const DataModel::Counter* counter = manager.GetCounter(counterID);
		if (!counter)
		{
			client.ReplyHtmlWithHeaderAndParagraph(Languages::TextCounterDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& counterName = counter->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteCounter));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, counterName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "counterdelete"))
			.AddContent(HtmlTagInputHidden("counter", to_string(counterID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		client.ReplyHtmlWithHeader(content);
	}

	void WebClientCounter::HandleCounterDelete(const map<string, string>& arguments)
	{
		const CounterID counterID = Utils::Utils::GetIntegerMapEntry(arguments, "counter", CounterNone);
		const DataModel::Counter* counter = manager.GetCounter(counterID);
		if (!counter)
		{
			client.ReplyResponse(WebClient::ResponseError, Languages::TextCounterDoesNotExist);
			return;
		}

		string name = counter->GetName();
		string result;
		if (!manager.CounterDelete(counterID, result))
		{
			client.ReplyResponse(WebClient::ResponseError, result);
			return;
		}

		client.ReplyResponse(WebClient::ResponseInfo, Languages::TextCounterDeleted, name);
	}

	void WebClientCounter::HandleCounterGet(const map<string, string>& arguments)
	{
		const CounterID counterID = Utils::Utils::GetIntegerMapEntry(arguments, "counter");
		const DataModel::Counter* counter = manager.GetCounter(counterID);
		if (!counter)
		{
			client.ReplyHtmlWithHeader(HtmlTag());
			return;
		}
		client.ReplyHtmlWithHeader(HtmlTagCounter(counter));
	}

	void WebClientCounter::HandleCounterIncrement(const map<string, string>& arguments)
	{
		const CounterID counterID = Utils::Utils::GetIntegerMapEntry(arguments, "counter");
		const bool result = manager.Count(counterID, CounterTypeIncrement);
		if (result)
		{
			client.ReplyResponse(WebClient::ResponseInfo, Languages::TextCounterIncremented);
		}
		else
		{
			client.ReplyResponse(WebClient::ResponseWarning, Languages::TextCounterReachedLimit);
		}
	}

	void WebClientCounter::HandleCounterDecrement(const map<string, string>& arguments)
	{
		const CounterID counterID = Utils::Utils::GetIntegerMapEntry(arguments, "counter");
		const bool result = manager.Count(counterID, CounterTypeDecrement);
		if (result)
		{
			client.ReplyResponse(WebClient::ResponseInfo, Languages::TextCounterDecremented);
		}
		else
		{
			client.ReplyResponse(WebClient::ResponseWarning, Languages::TextCounterReachedLimit);
		}
	}
}} // namespace Server::Web
