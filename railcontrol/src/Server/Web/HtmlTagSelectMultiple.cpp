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

#include "Server/Web/HtmlTagSelectMultiple.h"

namespace Server { namespace Web
{
	HtmlTagSelectMultiple::HtmlTagSelectMultiple(const std::string& name, const std::string& defaultValue)
	:	HtmlTag("div"),
		commandID("s_" + name),
		key(defaultValue)
	{
		AddId(commandID + "_container");
		AddClass("dropdowncontainer");

		HtmlTagInputHidden hidden(name, "");
		hidden.AddId(commandID);
		AddChildTag(hidden);

		HtmlTagInputText text("skip_" + commandID);
		text.AddId("skip_" + commandID);
		text.AddAttribute("onclick", "toggleClass('d_" + commandID + "', 'show'); return false;");
		text.AddAttribute("readonly");
		AddChildTag(text);

		HtmlTag dropDown("div");
		dropDown.AddClass("dropdown");
		dropDown.AddId("d_" + commandID);
		AddChildTag(dropDown);
	}

	HtmlTag HtmlTagSelectMultiple::AddAttribute(const std::string& name, const std::string& value)
	{
		childTags[0].AddAttribute(name, value);
		return *this;
	}

	void HtmlTagSelectMultiple::AddOption(const std::string& key,
		const std::string& value,
		const bool isSet,
		const std::string& none,
		const std::string& several)
	{
		HtmlTag optionTag("div");
		optionTag.AddAttribute("key", key);
		optionTag.AddClass("dropdownentry");
		optionTag.AddContent(value);
		if (isSet)
		{
			this->value = (this->value.size() == 0 ? value : several);
			optionTag.AddClass("selected_option");
		}
		optionTag.AddAttribute("onclick", "selectMultipleValue('" + key + "', '" + commandID + "', '" + none + "', '" + several + "');");
		childTags[2].AddChildTag(optionTag);
	}
}} // namespace Server::Web
