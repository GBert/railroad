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

#include "Utils/Utils.h"
#include "Server/Web/HtmlTagInput.h"

using std::string;

namespace Server { namespace Web
{
	HtmlTagInput::HtmlTagInput(const string& type,
		const string& name,
		const string& value,
		const Style style)
	:	HtmlTag("input")
	{
		AddAttribute("type", type);
		AddAttribute("name", name);
		AddId(name);
		if (value.size() > 0)
		{
			AddAttribute("value", Utils::Utils::HtmlEncode(value));
		}
		switch(style)
		{
			case StyleDisabled:
				AddAttribute("disabled");
				break;

			case StyleReadOnly:
				AddAttribute("readonly");
				break;

			default:
				break;
		}
	}
}} // namespace Server::Web
