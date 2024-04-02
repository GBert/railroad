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

#include "Server/Web/HtmlTagSelect.h"

namespace Server { namespace Web
{
	HtmlTagSelect::HtmlTagSelect(const std::string& name,
		const std::map<std::string,std::string>& options,
		const std::string& defaultValue)
	:	HtmlTagSelect(name)
	{
		for (auto& option : options)
		{
			AddOption(option.first, option.second, option.first == defaultValue);
		}
		CheckDefaultKeyValue();
	}

	HtmlTagSelect::HtmlTagSelect(const std::string& name,
		const std::map<std::string,DataModel::ObjectIdentifier>& options,
		const DataModel::ObjectIdentifier& defaultValue)
	:	HtmlTagSelect(name)
	{
		for (auto& option : options)
		{
			AddOption(option.second, option.first, option.second == defaultValue);
		}
		CheckDefaultKeyValue();
	}

	HtmlTagSelect::HtmlTagSelect(const std::string& name)
	:	HtmlTag("div"),
		commandID("s_" + name),
		defaultKeyValueFound(false)
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

	HtmlTag HtmlTagSelect::AddAttribute(const std::string& name, const std::string& value)
	{
		childTags[0].AddAttribute(name, value);
		return *this;
	}

	void HtmlTagSelect::AddOption(const std::string& key,
		const std::string& value,
		const bool defaultKeyValue)
	{
		defaultKeyValueFound |= defaultKeyValue;
		if (defaultKeyValue || defaultKey.length() == 0)
		{
			defaultKey = key;
			defaultValue = value;
		}

		HtmlTag optionTag("div");
		optionTag.AddAttribute("key", key);
		optionTag.AddClass("dropdownentry");
		optionTag.AddContent(value);
		if (defaultKeyValue)
		{
			SetDefaultKeyValue(key, value);
			optionTag.AddClass("selected_option");
		}
		optionTag.AddAttribute("onclick", "selectValue('" + key + "', '" + value + "', '" + commandID + "');");
		childTags[2].AddChildTag(optionTag);
	}

	void HtmlTagSelect::CheckDefaultKeyValue()
	{
		if (defaultKeyValueFound)
		{
			return;
		}

		SetDefaultKeyValue(defaultKey, defaultValue);
	}
}} // namespace Server::Web
