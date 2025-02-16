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

#include <map>
#include <string>

#include "Server/Web/HtmlTag.h"

namespace Server { namespace Web
{
	class HtmlTagButton : public HtmlTag
	{
		public:
			HtmlTagButton() = delete;
			HtmlTagButton(const HtmlTagButton&) = delete;
			HtmlTagButton& operator=(const HtmlTagButton&) = delete;

			HtmlTagButton(const std::string& value,
				const std::string& command,
				const std::string& tooltipText = "");

			inline HtmlTagButton(const Languages::TextSelector value,
				const std::string& command,
				const std::string& tooltipText = "")
			:	HtmlTagButton(Languages::GetText(value), command, tooltipText)
			{
			}

			inline HtmlTagButton(const std::string& value,
				const std::string& command,
				const Languages::TextSelector tooltipText)
			:	HtmlTagButton(value, command, Languages::GetText(tooltipText))
			{
			}

			virtual ~HtmlTagButton()
			{
			}

			virtual inline HtmlTag AddAttribute(const std::string& name, const std::string& value = "") override
			{
				childTags[0].AddAttribute(name, value);
				return *this;
			}

			virtual inline bool IsAttributeSet(const std::string& name) override
			{
				return childTags[0].IsAttributeSet(name);
			}

			virtual inline HtmlTag AddClass(const std::string& value) override
			{
				childTags[0].AddClass(value);
				return *this;
			}

		protected:
			const std::string commandID;
	};
}} // namespace Server::Web
