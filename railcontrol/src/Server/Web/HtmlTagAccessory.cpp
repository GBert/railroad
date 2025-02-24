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

#include <sstream>

#include "DataModel/Accessory.h"
#include "Server/Web/HtmlTagAccessory.h"

using std::string;
using std::to_string;

namespace Server { namespace Web
{
	HtmlTagAccessory::HtmlTagAccessory(const DataModel::Accessory* accessory)
	:	HtmlTagLayoutItem(dynamic_cast<const DataModel::LayoutItem*>(accessory))
	{
		const DataModel::AccessoryType accessoryType = static_cast<DataModel::AccessoryType>(accessory->GetAccessoryType() & DataModel::AccessoryTypeMask);
		switch (accessoryType)
		{
			case DataModel::AccessoryTypeStraight:
			case DataModel::AccessoryTypeDecoupler:
				image = "<polygon class=\"track\" points=\"15,0 21,0 21,36 15,36\"/>";
				break;

			case DataModel::AccessoryTypeTurn:
				image = "<polygon class=\"track\" points=\"0,21 0,15 21,36 15,36\"/>";
				break;

			case DataModel::AccessoryTypeDefault:
			case DataModel::AccessoryTypeLight:
			default:
				break;
		}
		switch (accessoryType)
		{
			case DataModel::AccessoryTypeDefault:
			case DataModel::AccessoryTypeStraight:
			case DataModel::AccessoryTypeTurn:
				image += "<polygon class=\"accessory\" points=\"10,10 26,10 26,26 10,26\" />";
				break;

			case DataModel::AccessoryTypeDecoupler:
				image += "<polygon class=\"accessory\" points=\"13,6 13,30 11,28 11,8\" />";
				image += "<polygon class=\"accessory\" points=\"23,6 25,8 25,28 23,30\" />";
				break;

			case DataModel::AccessoryTypeLight:
				image =	"<polyline points=\"15.5,22.3 14.8,21.8 14.2,21.2 13.7,20.5 13.3,19.7 13.1,18.9 13,18 13.1,17.1 13.3,16.3 13.7,15.5 14.2,14.8 14.8,14.2 15.5,13.7 16.3,13.3 17.1,13.1 18,13 18.9,13.1 19.7,13.3 20.5,13.7 21.2,14.2 21.8,14.8 22.3,15.5 22.7,16.3 22.9,17.1 23,18 22.9,18.9 22.7,19.7 22.3,20.5 21.8,21.2 21.2,21.8 20.5,22.3\" stroke=\"white\" stroke-width=\"0\" fill=\"white\"/>"
					"<polyline points=\"15,23 21,23 21,30 18,32 15,30\" stroke=\"white\" stroke-width=\"0\" fill=\"white\"/>"
					"<polyline points=\"10.2,22 5,25.5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"9,18 3,18\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"10.2,13.5 5,10.5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"13.5,10.2 10.5,5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"18,9 18,3\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"22.5,10.2 25.5,5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"25.8,13.5 31,10.5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"27,18 33,18\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"25.8,22.5 31,25.5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>";
				break;

			case DataModel::AccessoryTypeLightInhouse:
				image = //"<polyline points=\"3,3 1.5,3.5 1,5 1,33 1.5,34.5 3,35 33,35 34.5,34.5 35,33 35,5 34.5,3.5 33,3 3,3\" stroke=\"white\" stroke-width=\"1\" fill=\"none\"/>"
					"<polyline points=\"0,10.5 18,0.5 36,10.5\" stroke=\"white\" stroke-width=\"1\" fill=\"none\"/>"
					"<polyline points=\"3.5,35.5 3.5,8.5 32.5,8.5 32.5,35.5 3.5,35.5\" stroke=\"white\" stroke-width=\"1\" fill=\"none\"/>"
					"<polyline points=\"23,10 22.9,10.9 22.7,11.7 22.3,12.5 21.8,13.2 21.2,13.8 20.5,14.3 19.7,14.7 18.9,14.9 18,15 17.1,14.9 16.3,14.7 15.5,14.3 14.8,13.8 14.2,13.2 13.7,12.5 13.3,11.7 13.1,10.9 13,10\" stroke=\"white\" stroke-width=\"0\" fill=\"white\"/>"
					"<polyline points=\"25.8,14.5 31,17.5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"22.5,17.8 25.5,23\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"18,19 18,25\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"13.5,17.8 10.5,23\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"10.2,14.5 5,17.5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>";
				break;

			case DataModel::AccessoryTypeLightStreet:
				image = "<polyline points=\"4.5,36 4.5,9.5 6.5,7.5 23,3.5\" stroke=\"white\" stroke-width=\"1\" fill=\"none\"/>"
					"<polyline points=\"23,5 22.9,5.9 22.7,6.7 22.3,7.5 21.8,8.2 21.2,8.8 20.5,9.3 19.7,9.7 18.9,9.9 18,10 17.1,9.9 16.3,9.7 15.5,9.3 14.8,8.8 14.2,8.2 13.7,7.5\" stroke=\"white\" stroke-width=\"0\" fill=\"white\"/>"
					"<polyline points=\"25.8,9.5 31,12.5\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"22.5,12.8 25.5,18\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"18,14 18,20\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>"
					"<polyline points=\"13.5,12.8 10.5,18\" stroke=\"white\" stroke-width=\"2\" class=\"accessory_on\"/>";
				break;

			default:
				break;
		}

		const DataModel::AccessoryState state = accessory->GetAccessoryState();

		string accessoryIdString = to_string(accessory->GetID());
		imageDiv.AddClass("accessory_item");
		switch (accessory->GetAccessoryType() & DataModel::AccessoryTypeConnectionMask)
		{
			case DataModel::AccessoryTypeOnOn:
			case DataModel::AccessoryTypeOnOff:
				imageDiv.AddClass(state == DataModel::AccessoryStateOn ? "accessory_on" : "accessory_off");
				imageDiv.AddAttribute("onclick", "return onClickAccessory(" + accessoryIdString + ");");
				break;

			case DataModel::AccessoryTypeOnPush:
				imageDiv.AddClass("accessory_off");
				imageDiv.AddAttribute("onpointerdown", "return onPointerDownAccessory(" + accessoryIdString + ");");
				imageDiv.AddAttribute("onpointerup", "return onPointerUpAccessory(" + accessoryIdString + ");");
				break;

			default:
				imageDiv.AddClass("accessory_off");
				break;
		}

		const string& accessoryName = accessory->GetName();
		AddToolTip(accessoryName + " (addr=" + to_string(accessory->GetAddress()) + ")");
		AddContextMenuEntry(accessoryName);
		AddContextMenuEntry(Languages::TextReleaseAccessory, "fireRequestAndForget('/?cmd=accessoryrelease&accessory=" + accessoryIdString + "');");
		AddContextMenuEntry(Languages::TextEditAccessory, "loadPopup('/?cmd=accessoryedit&accessory=" + accessoryIdString + "');");
		AddContextMenuEntry(Languages::TextDeleteAccessory, "loadPopup('/?cmd=accessoryaskdelete&accessory=" + accessoryIdString + "');");
		FinishInit();
	}
}} // namespace Server::Web
