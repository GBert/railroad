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

#include <sstream>

#include "DataModel/Counter.h"
#include "Server/Web/HtmlTagCounter.h"

using std::string;
using std::to_string;

namespace Server { namespace Web
{
	HtmlTagCounter::HtmlTagCounter(const DataModel::Counter* counter)
	:	HtmlTagLayoutItem(dynamic_cast<const DataModel::LayoutItem*>(counter))
	{
		const string counterIdString = to_string(counter->GetID());
		image = "<polygon class=\"track\" points=\"15,0 21,0 21,36 15,36\"/>"
			"<text class=\"counter\" id=\"c_" + counterIdString + "_text\" x=\"-28\" y=\"11\" transform=\"rotate(270 0,0)\">" + to_string(counter->GetCounter()) + "</text>";

		imageDiv.AddClass("counter_item");
		imageDiv.AddAttribute("onclick", "return onClickCounter(" + counterIdString + ");");

		const string& counterName = counter->GetName();
		AddToolTip(counterName);
		AddContextMenuEntry(counterName);
		AddContextMenuEntry(Languages::TextCounterIncrement, "fireRequestAndForget('/?cmd=counterincrement&counter=" + counterIdString + "');");
		AddContextMenuEntry(Languages::TextCounterDecrement, "fireRequestAndForget('/?cmd=counterdecrement&counter=" + counterIdString + "');");
		AddContextMenuEntry(Languages::TextEditCounter, "loadPopup('/?cmd=counteredit&counter=" + counterIdString + "');");
		AddContextMenuEntry(Languages::TextDeleteCounter, "loadPopup('/?cmd=counteraskdelete&counter=" + counterIdString + "');");
		FinishInit();
	}
}} // namespace Server::Web
