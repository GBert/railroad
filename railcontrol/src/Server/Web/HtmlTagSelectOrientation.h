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

#include "Languages.h"
#include "Server/Web/HtmlTag.h"
#include "Server/Web/HtmlTagSelect.h"

namespace Server { namespace Web
{
	class HtmlTagSelectOrientation : public HtmlTag
	{
		public:
			HtmlTagSelectOrientation() = delete;

			HtmlTagSelectOrientation(const std::string& name, const Orientation defaultValue)
			{
				std::map<Orientation,Languages::TextSelector> orientations;
				orientations[OrientationLeft] = Languages::TextLeft;
				orientations[OrientationRight] = Languages::TextRight;
				AddChildTag(HtmlTagSelect(name, orientations, defaultValue));
			}

			virtual ~HtmlTagSelectOrientation() {}

			virtual inline HtmlTag AddAttribute(const std::string& name, const std::string& value) override
			{
				childTags[0].AddAttribute(name, value);
				return *this;
			}

			virtual inline bool IsAttributeSet(const std::string& name) override
			{
				return childTags[0].IsAttributeSet(name);
			}

			virtual inline HtmlTag AddClass(const std::string& className) override
			{
				childTags[0].AddClass(className);
				return *this;
			}
	};
}} // namespace Server::Web

