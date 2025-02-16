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

#include "Server/Web/HtmlTagButton.h"

namespace Server { namespace Web
{
	class HtmlTagButtonSwapRelation : public HtmlTagButton
	{
		public:
			HtmlTagButtonSwapRelation() = delete;
			HtmlTagButtonSwapRelation(HtmlTagButtonSwapRelation&) = delete;
			HtmlTagButtonSwapRelation& operator=(HtmlTagButtonSwapRelation&) = delete;

			inline HtmlTagButtonSwapRelation(const std::string& atlock, const std::string& priority, const bool up)
			:	HtmlTagButton(HtmlTag("span").AddContent(up ? "&uarr;" : "&darr;"), "swap_relation_" + priority + "_" + std::string(up ? "up" : "down"))
			{
				AddAttribute("onclick", "swapRelations('" + atlock + "', " + priority + ", '" + std::string(up ? "up" : "down") + "'); return false;");
				AddClass("small_button");
			}
	};
}} // namespace Server::Web
