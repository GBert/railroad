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

#pragma once

#include <string>

#include "Languages.h"
#include "Server/Web/HtmlTag.h"
#include "Server/Web/HtmlTagLabel.h"
#include "Server/Web/HtmlTagInputInteger.h"

namespace Server { namespace Web
{
	class HtmlTagInputIntegerWithLabel : public HtmlTag
	{
		public:
			HtmlTagInputIntegerWithLabel() = delete;

			HtmlTagInputIntegerWithLabel(const std::string& name, const Languages::TextSelector label, const int min, const int max)
			:	HtmlTagInputIntegerWithLabel(name, label, 0, min, max)
			{}

			template<typename... TextArgs>
			HtmlTagInputIntegerWithLabel(const std::string& name, const Languages::TextSelector label, const int value, const int min, const int max, TextArgs... textArgs)
			:	HtmlTag("div")
			{
				HtmlTag::AddClass("input_integer_with_label");
				AddChildTag(HtmlTagLabel(label, name, textArgs...));
				AddChildTag(HtmlTagInputInteger(name, value, min, max));
			}

			virtual ~HtmlTagInputIntegerWithLabel()
			{
			}

			inline virtual HtmlTag AddAttribute(const std::string& name, const std::string& value) override
			{
				childTags[1].AddAttribute(name, value);
				return *this;
			}

			inline virtual HtmlTag AddClass(const std::string& _class) override
			{
				childTags[1].AddClass(_class);
				return *this;
			}
	};
}} // namespace Server::Web

