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

#pragma once

#include <string>

#include "Server/Web/HtmlTagInput.h"

namespace Server { namespace Web
{
	class HtmlTagInputCheckbox : public HtmlTagInput
	{
		public:
			HtmlTagInputCheckbox() = delete;

			inline HtmlTagInputCheckbox(const std::string& name, const std::string& value, const bool checked = false)
			:	HtmlTagInput("checkbox", name, value)
			{
				if (checked)
				{
					AddAttribute("checked");
				}
				AddClass("checkbox");
			};
	};
}} // namespace Server::Web

