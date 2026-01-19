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

#pragma once

#include <map>
#include <string>

#include "DataModel/ObjectIdentifier.h"
#include "Languages.h"
#include "Server/Web/HtmlTag.h"
#include "Server/Web/HtmlTagInputHidden.h"
#include "Server/Web/HtmlTagInputText.h"

namespace Server { namespace Web
{
	class HtmlTagSelect : public HtmlTag
	{
		public:
			HtmlTagSelect() = delete;

			HtmlTagSelect(const std::string& name,
				const std::map<std::string,std::string>& options,
				const std::string& defaultValue = "");

			HtmlTagSelect(const std::string& name,
				const std::map<std::string,DataModel::ObjectIdentifier>& options,
				const DataModel::ObjectIdentifier& defaultValue = DataModel::ObjectIdentifier());

			template<typename T>
			HtmlTagSelect(const std::string& name,
				const std::map<std::string,T>& options,
				const int defaultValue = 0) // This can not be Type T, because it would be ambiguous with previous declaration
			:	HtmlTagSelect(name)
			{
				for (auto& option : options)
				{
					AddOption(std::to_string(option.second), option.first, option.second == defaultValue);
				}
				CheckDefaultKeyValue();
			}

			// T2 must be implicitly convertible to Languages::TextSelector
			template<typename T1, typename T2>
			HtmlTagSelect(const std::string& name,
				const std::map<T1,T2>& options,
				const T1 defaultValue = 0)
			:	HtmlTagSelect(name)
			{
				for (auto& option : options)
				{
					AddOption(std::to_string(option.first), Languages::GetText(option.second), option.first == defaultValue);
				}
				CheckDefaultKeyValue();
			}

			template<typename T>
			HtmlTagSelect(const std::string& name,
				const std::map<T,std::string>& options,
				const T defaultValue = 0)
			:	HtmlTagSelect(name)
			{
				for (auto& option : options)
				{
					AddOption(std::to_string(option.first), option.second, option.first == defaultValue);
				}
				CheckDefaultKeyValue();
			}

			virtual HtmlTag AddAttribute(const std::string& name, const std::string& value = "") override;

		private:
			HtmlTagSelect(const std::string& name);

			void AddOption(const std::string& key,
				const std::string& value,
				const bool defaultKeyValue);

			void CheckDefaultKeyValue();

			inline void SetDefaultKeyValue(const std::string& key,
				const std::string& value)
			{
				childTags[0].AddAttribute("value", key);
				childTags[1].AddAttribute("value", value);
			}

			const std::string commandID;
			bool defaultKeyValueFound;
			std::string defaultKey;
			std::string defaultValue;
	};
}} // namespace Server::Web

