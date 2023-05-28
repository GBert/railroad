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

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "WebServer/HtmlTag.h"
#include "WebServer/HtmlTagLabel.h"
#include "WebServer/HtmlTagSelectMultiple.h"

namespace WebServer
{
	class HtmlTagSelectMultipleWithLabel : public HtmlTag
	{
		public:
			HtmlTagSelectMultipleWithLabel() = delete;

			template<typename T>
			HtmlTagSelectMultipleWithLabel(const std::string& name,
				const Languages::TextSelector label,
				const std::vector<std::pair<T,Languages::TextSelector>>& options,
				const T defaultValue = 0)
			:	HtmlTag("div")
			{
				HtmlTag::AddClass("input_select_with_label");
				AddChildTag(HtmlTagLabel(label, "s_" + name));
				AddChildTag(HtmlTagSelectMultiple(name, options, defaultValue));
			}

			virtual ~HtmlTagSelectMultipleWithLabel()
			{
			}

			virtual HtmlTag AddAttribute(const std::string& name, const std::string& value = "") override;

			virtual bool IsAttributeSet(const std::string& name) override;

			virtual HtmlTag AddClass(const std::string& _class) override;
	};
} // namespace WebServer

