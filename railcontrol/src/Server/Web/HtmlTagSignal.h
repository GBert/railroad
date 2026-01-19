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

#include <string>

#include "DataModel/Signal.h"
#include "Manager.h"
#include "Server/Web/HtmlTagLayoutItem.h"

class Manager;

namespace DataModel
{
	class Signal;
}

namespace Server { namespace Web
{
	class HtmlTagSignal : public HtmlTagLayoutItem
	{
		public:
			HtmlTagSignal() = delete;
			HtmlTagSignal(const HtmlTagSignal&) = delete;
			HtmlTagSignal& operator=(const HtmlTagSignal&) = delete;

			HtmlTagSignal(const Manager& manager, const DataModel::Signal* const signal);

			virtual ~HtmlTagSignal()
			{
			}

			static std::string GetSignalImage(const DataModel::AccessoryState state,
				const DataModel::Signal* signal);

		private:
			void MenuEntry(const Languages::TextSelector text,
				const string& id,
				const DataModel::AccessoryState state,
				const string& aspect);

			static std::string GetSignalImagePlain(const DataModel::Signal* const signal);
			static std::string GetStateClassText(const DataModel::AccessoryState state);

			const DataModel::Signal* const signal;
	};
}} // namespace Server::Web

