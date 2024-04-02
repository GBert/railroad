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

#include <string>

#include "Server/Web/HtmlTagButtonMinus.h"
#include "Server/Web/HtmlTagButtonPlus.h"
#include "Server/Web/HtmlTagInput.h"
#include "Server/Web/HtmlTagInputInteger.h"

namespace Server { namespace Web
{
	HtmlTagInputInteger::HtmlTagInputInteger(const std::string& name, const int value, const int min, const int max)
	:	HtmlTag("div")
	{
		std::string minString = std::to_string(min);
		std::string maxString = std::to_string(max);
		AddId("d_" + name);
		AddClass("div_integer");
		HtmlTagInput input("text", name, std::to_string(value));
		input.AddClass("integer");
		input.AddAttribute("oninput", "checkIntegerValue('" + name + "', " + minString + ", " + maxString + ");");
		AddChildTag(input);

		AddChildTag(HtmlTagButtonMinus(name, min));
		AddChildTag(HtmlTagButtonPlus(name, max));
	}
}} // namespace Server::Web
