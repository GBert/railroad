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

#include "DataModel/Feedback.h"
#include "DataModel/LayoutItem.h"
#include "Server/Web/HtmlTagFeedback.h"

using std::string;
using std::to_string;

namespace Server { namespace Web
{
	HtmlTagFeedback::HtmlTagFeedback(const DataModel::Feedback* feedback,
		const DataModel::LayoutItem::LayoutPosition posX,
		const DataModel::LayoutItem::LayoutPosition posY,
		const DataModel::FeedbackType feedbackType)
	:	HtmlTagLayoutItem(dynamic_cast<const DataModel::LayoutItem*>(feedback), posX, posY)
	{
		switch (feedbackType)
		{
			case DataModel::FeedbackTypeStraight:
				image = "<polygon class=\"track\" points=\"15,0 21,0 21,36 15,36\"/>";
				break;

			case DataModel::FeedbackTypeTurn:
				image = "<polygon class=\"track\" points=\"0,21 0,15 21,36 15,36\"/>";
				break;

			case DataModel::FeedbackTypeDefault:
			default:
				break;
		}
		image += "<circle r=\"12\" cx=\"18\" cy=\"18\" stroke=\"white\" stroke-width=\"2\" class=\"feedback\"/>";

		const DataModel::Feedback::FeedbackState state = feedback->GetState();

		const string& feedbackName = feedback->GetName();

		string feedbackIdString = to_string(feedback->GetID());
		imageDiv.AddClass("feedback_item");
		imageDiv.AddClass(state == DataModel::Feedback::FeedbackStateOccupied ? "feedback_occupied" : "feedback_free");
		imageDiv.AddAttribute("onclick", "return onClickFeedback(" + feedbackIdString + ");");

		AddToolTip(feedbackName + " (pin=" + to_string(feedback->GetPin()) + ")");
		AddContextMenuEntry(feedbackName);
		AddContextMenuEntry(Languages::TextEditFeedback, "loadPopup('/?cmd=feedbackedit&feedback=" + feedbackIdString + "');");
		AddContextMenuEntry(Languages::TextDeleteFeedback, "loadPopup('/?cmd=feedbackaskdelete&feedback=" + feedbackIdString + "');");
		FinishInit();
	}
}} // namespace Server::Web
