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

#pragma once

#include <map>
#include <string>

#include "DataModel/ObjectIdentifier.h"
#include "Languages.h"
#include "WebServer/HtmlTag.h"
#include "WebServer/HtmlTagInputHidden.h"
#include "WebServer/HtmlTagInputText.h"

namespace WebServer
{
	class HtmlTagSelectMultiple : public HtmlTag
	{
		public:
			HtmlTagSelectMultiple() = delete;

			// T1 must be an integer type
			// T2 must be implicitly convertible to Languages::TextSelector
			template<typename T1, typename T2>
			HtmlTagSelectMultiple(const std::string& name,
				const std::map<T1,T2>& options,
				const T1 defaultValue = 0)
			:	HtmlTagSelectMultiple(name, std::to_string(defaultValue))
			{
				const std::string none = Languages::GetText(Languages::TextNone);
				const std::string several = Languages::GetText(Languages::TextSeveral);
				for (auto& option : options)
				{
					AddOption(std::to_string(option.first),
						Languages::GetText(option.second),
						static_cast<bool>((defaultValue & option.first) == option.first),
						none,
						several);
				}
				childTags[0].AddAttribute("value", key);
				if (value.size() == 0)
				{
					value = none;
				}
				childTags[1].AddAttribute("value", value);
			}

			virtual HtmlTag AddAttribute(const std::string& name, const std::string& value = "") override;

		private:
			HtmlTagSelectMultiple(const std::string& name, const std::string& defaultValue);

			void AddOption(const std::string& key,
				const std::string& value,
				const bool isSet,
				const std::string& none,
				const std::string& several);

			const std::string commandID;
			std::string key;
			std::string value;
	};
} // namespace WebServer

