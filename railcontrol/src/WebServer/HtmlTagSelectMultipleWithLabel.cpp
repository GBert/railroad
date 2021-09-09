/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2021 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "WebServer/HtmlTagSelectMultipleWithLabel.h"

namespace WebServer
{
	HtmlTag HtmlTagSelectMultipleWithLabel::AddAttribute(const std::string& name, const std::string& value)
	{
		HtmlTagSelectMultiple& select = reinterpret_cast<HtmlTagSelectMultiple&>(childTags.at(1));
		select.HtmlTagSelectMultiple::AddAttribute(name, value);
		return *this;
	}

	bool HtmlTagSelectMultipleWithLabel::IsAttributeSet(const std::string& name)
	{
		HtmlTagSelectMultiple& select = reinterpret_cast<HtmlTagSelectMultiple&>(childTags.at(1));
		return select.HtmlTagSelectMultiple::IsAttributeSet(name);
	}

	HtmlTag HtmlTagSelectMultipleWithLabel::AddClass(const std::string& _class)
	{
		HtmlTagSelectMultiple& select = reinterpret_cast<HtmlTagSelectMultiple&>(childTags.at(1));
		select.HtmlTagSelectMultiple::AddClass(_class);
		return *this;
	}
} // namespace WebServer

