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

#include "WebServer/HtmlTagSelectWithLabel.h"

namespace WebServer
{
	HtmlTagSelectWithLabel::HtmlTagSelectWithLabel(const std::string& name,
		const Languages::TextSelector label,
		const std::map<std::string,DataModel::ObjectIdentifier>& options,
		const DataModel::ObjectIdentifier& defaultValue)
	:	HtmlTag("div")
	{
		HtmlTag::AddClass("input_select_with_label");
		AddChildTag(HtmlTagLabel(label, "s_" + name));
		AddChildTag(HtmlTagSelect(name, options, defaultValue));
	}

	HtmlTag HtmlTagSelectWithLabel::AddAttribute(const std::string& name, const std::string& value)
	{
		HtmlTagSelect& select = reinterpret_cast<HtmlTagSelect&>(childTags.at(1));
		select.HtmlTagSelect::AddAttribute(name, value);
		return *this;
	}

	bool HtmlTagSelectWithLabel::IsAttributeSet(const std::string& name)
	{
		HtmlTagSelect& select = reinterpret_cast<HtmlTagSelect&>(childTags.at(1));
		return select.HtmlTagSelect::IsAttributeSet(name);
	}

	HtmlTag HtmlTagSelectWithLabel::AddClass(const std::string& _class)
	{
		HtmlTagSelect& select = reinterpret_cast<HtmlTagSelect&>(childTags.at(1));
		select.HtmlTagSelect::AddClass(_class);
		return *this;
	}
} // namespace WebServer

