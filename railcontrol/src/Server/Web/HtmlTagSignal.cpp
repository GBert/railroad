/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2023 Dominik (Teddy) Mahrer - www.railcontrol.org

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

#include "Server/Web/HtmlTagSignal.h"

using std::string;
using std::to_string;

namespace Server { namespace Web
{
	HtmlTagSignal::HtmlTagSignal(__attribute__((unused)) const Manager& manager, const DataModel::Signal* const signal)
	:	HtmlTagLayoutItem(dynamic_cast<const DataModel::LayoutItem*>(signal)),
	 	signal(signal)
	{
		image += "<polygon points=\"15,0 21,0 21,36 15,36\" fill=\"white\"/>";
		image += GetSignalImagePlain(signal);
		const string idText = to_string(signal->GetID());

		imageDiv.AddClass("signal_item");

		const DataModel::AccessoryState signalState = signal->GetAccessoryState();
		string stateClassText = GetStateClassText(signalState);
		imageDiv.AddClass(stateClassText);
		onClickMenuDiv.AddClass(stateClassText);

		const DataModel::AccessoryType type = signal->GetType();
		switch (type)
		{
			case DataModel::SignalTypeDeCombined:
				MenuEntry(Languages::TextSignalStateStop, idText, DataModel::SignalStateStop, "stop");
				MenuEntry(Languages::TextSignalStateClear, idText, DataModel::SignalStateClear, "clear");
				MenuEntry(Languages::TextSignalStateStopExpected, idText, DataModel::SignalStateStopExpected, "stopexpected");
				MenuEntry(Languages::TextSignalStateShunting, idText, DataModel::SignalStateAspect4, "aspect4");
				MenuEntry(Languages::TextSignalStateZs7, idText, DataModel::SignalStateAspect7, "aspect7");
				MenuEntry(Languages::TextSignalStateDark, idText, DataModel::SignalStateDark, "dark");
				imageDiv.AddAttribute("onclick", "return onClickWithMenu(event, '" + identifier + "');");
				break;

			case DataModel::SignalTypeChLDistant:
				break;

			case DataModel::SignalTypeChLMain:
				MenuEntry(Languages::TextSignalStateStop, idText, DataModel::SignalStateStop, "stop");
				MenuEntry(Languages::TextSignalStateClear, idText, DataModel::SignalStateClear, "clear");
				MenuEntry(Languages::TextSignalStateClear40, idText, DataModel::SignalStateAspect2, "aspect2");
				MenuEntry(Languages::TextSignalStateClear60, idText, DataModel::SignalStateAspect3, "aspect3");
				MenuEntry(Languages::TextSignalStateClear90, idText, DataModel::SignalStateAspect5, "aspect5");
				MenuEntry(Languages::TextSignalStateShortClear, idText, DataModel::SignalStateAspect6, "aspect6");
				imageDiv.AddAttribute("onclick", "return onClickWithMenu(event, '" + identifier + "');");
				break;

			case DataModel::SignalTypeChDwarf:
				MenuEntry(Languages::TextSignalStateStop, idText, DataModel::SignalStateStop, "stop");
				MenuEntry(Languages::TextSignalStateClear, idText, DataModel::SignalStateClear, "clear");
				MenuEntry(Languages::TextSignalStateCaution, idText, DataModel::SignalStateStopExpected, "stopexpected");
				imageDiv.AddAttribute("onclick", "return onClickWithMenu(event, '" + identifier + "');");
				break;

			case DataModel::SignalTypeSimpleRight:
			case DataModel::SignalTypeSimpleLeft:
				imageDiv.AddAttribute("onclick", "return onClickSignal(" + idText + ");");
				break;

			default:
				break;
		}
		AddToolTip(signal->GetName() + " (addr=" + to_string(signal->GetAddress()) + ")");

		AddContextMenuEntry(Languages::TextEditSignal, "loadPopup('/?cmd=signaledit&signal=" + idText + "');");
		AddContextMenuEntry(Languages::TextDeleteSignal, "loadPopup('/?cmd=signalaskdelete&signal=" + idText + "');");
		FinishInit();
	}

	void HtmlTagSignal::MenuEntry(const Languages::TextSelector text,
		const string& id,
		const DataModel::AccessoryState state,
		const string& aspect)
	{
		const DataModel::AddressOffset offset = signal->GetStateAddressOffset(state);
		if (offset < 0)
		{
			return;
		}

		AddOnClickMenuEntry(text,
			"fireRequestAndForget('/?cmd=signalstate&signal=" + id + "&state=" + aspect + "');",
			"menu_" + aspect);
	}

	string HtmlTagSignal::GetSignalImage(const DataModel::AccessoryState state,
		const DataModel::Signal* signal)
	{
		string out = "<div class=\"inline-block " + GetStateClassText(state) + "\">";
		out += "<svg height=\"36\" width=\"36\">" + GetSignalImagePlain(signal) + "</svg>";
		out += "</div>";
		return out;
	}

	string HtmlTagSignal::GetSignalImagePlain(const DataModel::Signal* const signal)
	{
		switch (signal->GetType())
		{
			case DataModel::SignalTypeDeCombined:
				return "<polygon points=\"0,0 14,0 14,31 0,31\" fill=\"white\"/>"
					"<polygon points=\"1,1 13,1 13,30 1,30\" fill=\"black\"/>"
					"<polyline points=\"7,31 7,34\" style=\"stroke:gray;stroke-width:2\"/>"
					"<polyline points=\"4,34 10,34\" style=\"stroke:gray;stroke-width:2\"/>"
					"<circle class=\"stop aspect4\" cx=\"7\" cy=\"10\" r=\"2.5\" fill=\"red\" opacity=\"0\"/>"
					"<circle class=\"clear\" cx=\"5\" cy=\"15\" r=\"2.5\" fill=\"lightgreen\" opacity=\"0\"/>"
					"<circle class=\"stopexpected\" cx=\"9\" cy=\"15\" r=\"2.5\" fill=\"orange\" opacity=\"0\"/>"
					"<circle class=\"aspect4\" cx=\"7\" cy=\"19\" r=\"1.5\" fill=\"white\" opacity=\"0\"/>"
					"<circle class=\"aspect4\" cx=\"3\" cy=\"23\" r=\"1.5\" fill=\"white\" opacity=\"0\"/>"
					"<circle class=\"aspect7\" cx=\"5\" cy=\"19\" r=\"1.5\" fill=\"yellow\" opacity=\"0\"/>"
					"<circle class=\"aspect7\" cx=\"9\" cy=\"19\" r=\"1.5\" fill=\"yellow\" opacity=\"0\"/>"
					"<circle class=\"aspect7\" cx=\"7\" cy=\"23\" r=\"1.5\" fill=\"yellow\" opacity=\"0\"/>";

			case DataModel::SignalTypeChLDistant:
				return "<polygon points=\"0,13.5 2.5,11 11.5,11 14,13.5 14,25.5 11.5,28 2.5,28 0,25.5\" fill=\"black\"/>"
					"<polyline points=\"7,31 7,34\" style=\"stroke:gray;stroke-width:2\"/>"
					"<polyline points=\"4,34 10,34\" style=\"stroke:gray;stroke-width:2\"/>"
					"<circle class=\"stop aspect2 aspect3 aspect6\" cx=\"3.5\" cy=\"14.5\" r=\"2.5\" fill=\"orange\" opacity=\"0\"/>"
					"<circle class=\"stop\" cx=\"10.5\" cy=\"14.5\" r=\"2.5\" fill=\"orange\" opacity=\"0\"/>"
					"<circle class=\"clear aspect3 aspect5\" cx=\"3.5\" cy=\"24.5\" r=\"2.5\" fill=\"lightgreen\" opacity=\"0\"/>"
					"<circle class=\"clear aspect2 aspect3 aspect5 aspect6\" cx=\"10.5\" cy=\"18.5\" r=\"2.5\" fill=\"lightgreen\" opacity=\"0\"/>"
					"<circle class=\"aspect5\" cx=\"10.5\" cy=\"24.5\" r=\"2.5\" fill=\"orange\" opacity=\"0\"/>";

			case DataModel::SignalTypeChLMain:
				return "<polygon points=\"2,2.5 4.5,0 9.5,0 12,2.5 12,28.5 9.5,31 4.5,31 2,28.5\" fill=\"white\"/>"
					"<polygon points=\"3,3.5 5.5,1 8.5,1 11,3.5 11,27.5 8.5,30 5.5,30 3,27.5\" fill=\"black\"/>"
					"<polyline points=\"7,31 7,34\" style=\"stroke:gray;stroke-width:2\"/>"
					"<polyline points=\"4,34 10,34\" style=\"stroke:gray;stroke-width:2\"/>"
					"<circle class=\"clear aspect2 aspect3 aspect5\" cx=\"7\" cy=\"4.5\" r=\"2.5\" fill=\"lightgreen\" opacity=\"0\"/>"
					"<circle class=\"stop\" cx=\"7\" cy=\"10\" r=\"2.5\" fill=\"red\" opacity=\"0\"/>"
					"<circle class=\"aspect6\" cx=\"7\" cy=\"10\" r=\"2.5\" fill=\"orange\" opacity=\"0\"/>"
					"<circle class=\"aspect3 aspect5\" cx=\"7\" cy=\"15.5\" r=\"2.5\" fill=\"lightgreen\" opacity=\"0\"/>"
					"<circle class=\"aspect2 aspect6\" cx=\"7\" cy=\"21\" r=\"2.5\" fill=\"orange\" opacity=\"0\"/>"
					"<circle class=\"aspect5\" cx=\"7\" cy=\"26.5\" r=\"2.5\" fill=\"lightgreen\" opacity=\"0\"/>";

			case DataModel::SignalTypeChDwarf:
				return "<polygon points=\"0,13 6,13 14,21 14,27 0,27\" fill=\"white\"/>"
					"<polygon points=\"1,14 5,14 13,22 13,26 1,26\" fill=\"black\"/>"
					"<polyline points=\"7,27 7,30\" style=\"stroke:gray;stroke-width:2\"/>"
					"<polyline points=\"4,30 10,30\" style=\"stroke:gray;stroke-width:2\"/>"
					"<circle class=\"clear stopexpected\" cx=\"4\" cy=\"17\" r=\"2\" fill=\"white\" opacity=\"0\"/>"
					"<circle class=\"stop clear\" cx=\"4\" cy=\"23\" r=\"2\" fill=\"white\" opacity=\"0\"/>"
					"<circle class=\"stop stopexpected\" cx=\"10\" cy=\"23\" r=\"2\" fill=\"white\" opacity=\"0\"/>";

			case DataModel::SignalTypeSimpleRight:
				return "<polygon points=\"22,4 26,0 32,0 36,4 36,19 32,23 26,23 22,19\" fill=\"white\"/>"
					"<polygon points=\"23,5 27,1 31,1 35,5 35,18 31,22 27,22 23,18\" fill=\"black\"/>"
					"<polyline points=\"29,23 29,30\" style=\"stroke:gray;stroke-width:2\"/>"
					"<polyline points=\"26,30 32,30\" style=\"stroke:gray;stroke-width:2\"/>"
					"<circle class=\"stop\" cx=\"29\" cy=\"7\" r=\"4\" fill=\"red\" opacity=\"0\"/>"
					"<circle class=\"clear\" cx=\"29\" cy=\"16\" r=\"4\" fill=\"lightgreen\" opacity=\"0\"/>";

			case DataModel::SignalTypeSimpleLeft:
				return "<polygon points=\"0,4 4,0 10,0 14,4 14,19 10,23 4,23 0,19\" fill=\"white\"/>"
					"<polygon points=\"1,5 5,1 9,1 13,5 13,18 9,22 5,22 1,18\" fill=\"black\"/>"
					"<polyline points=\"7,23 7,30\" style=\"stroke:gray;stroke-width:2\"/>"
					"<polyline points=\"4,30 10,30\" style=\"stroke:gray;stroke-width:2\"/>"
					"<circle class=\"stop\" cx=\"7\" cy=\"7\" r=\"4\" fill=\"red\" opacity=\"0\"/>"
					"<circle class=\"clear\" cx=\"7\" cy=\"16\" r=\"4\" fill=\"lightgreen\" opacity=\"0\"/>";

			default:
				return "";
		}
	}

	string HtmlTagSignal::GetStateClassText(const DataModel::AccessoryState state)
	{
		switch (state)
		{
			case DataModel::SignalStateStop:
				return "signal_stop";

			case DataModel::SignalStateClear:
				return "signal_clear";

			case DataModel::SignalStateAspect2:
				return "signal_aspect2";

			case DataModel::SignalStateAspect3:
				return "signal_aspect3";

			case DataModel::SignalStateAspect4:
				return "signal_aspect4";

			case DataModel::SignalStateAspect5:
				return "signal_aspect5";

			case DataModel::SignalStateAspect6:
				return "signal_aspect6";

			case DataModel::SignalStateAspect7:
				return "signal_aspect7";

			case DataModel::SignalStateStopExpected:
				return "signal_stopexpected";

			case DataModel::SignalStateDark:
			default:
				return "signal_dark";
		}
	}
}} // namespace Server::Web
