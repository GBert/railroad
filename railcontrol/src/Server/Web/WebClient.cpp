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

#include <algorithm>
#include <cstring>		//memset
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "DataModel/DataModel.h"
#include "DataModel/AccessoryConfig.h"
#include "DataModel/LocoConfig.h"
#include "DataModel/ObjectIdentifier.h"
#include "Hardware/HardwareHandler.h"
#include "RailControl.h"
#include "Utils/Utils.h"
#include "Version.h"
#include "Server/Web/HtmlTagAccessory.h"
#include "Server/Web/HtmlTagButtonCancel.h"
#include "Server/Web/HtmlTagButtonCommand.h"
#include "Server/Web/HtmlTagButtonCommandFullScreen.h"
#include "Server/Web/HtmlTagButtonCommandPressRelease.h"
#include "Server/Web/HtmlTagButtonCommandToggle.h"
#include "Server/Web/HtmlTagButtonCommandWide.h"
#include "Server/Web/HtmlTagButtonOK.h"
#include "Server/Web/HtmlTagButtonPopup.h"
#include "Server/Web/HtmlTagButtonPopupWide.h"
#include "Server/Web/HtmlTagFeedback.h"
#include "Server/Web/HtmlTagInputCheckbox.h"
#include "Server/Web/HtmlTagInputCheckboxWithLabel.h"
#include "Server/Web/HtmlTagInputHidden.h"
#include "Server/Web/HtmlTagInputIntegerWithLabel.h"
#include "Server/Web/HtmlTagInputSliderLocoSpeed.h"
#include "Server/Web/HtmlTagInputTextWithLabel.h"
#include "Server/Web/HtmlTagRoute.h"
#include "Server/Web/HtmlTagSelectOrientation.h"
#include "Server/Web/HtmlTagSelectWithLabel.h"
#include "Server/Web/HtmlTagSignal.h"
#include "Server/Web/HtmlTagSwitch.h"
#include "Server/Web/HtmlTagText.h"
#include "Server/Web/HtmlTagTrack.h"
#include "Server/Web/ResponseCsv.h"
#include "Server/Web/ResponseHtml.h"
#include "Server/Web/ResponseHtmlFull.h"
#include "Server/Web/ResponseHtmlNotFound.h"
#include "Server/Web/ResponseHtmlNotImplemented.h"
#include "Server/Web/WebClient.h"
#include "Server/Web/WebClientStatic.h"
#include "Server/Web/WebServer.h"
#include "Utils/Integer.h"

using namespace DataModel;
using LayoutPosition = DataModel::LayoutItem::LayoutPosition;
using LayoutItemSize = DataModel::LayoutItem::LayoutItemSize;
using LayoutRotation = DataModel::LayoutItem::LayoutRotation;
using Visible = DataModel::LayoutItem::Visible;
using std::deque;
using std::map;
using std::string;
using std::thread;
using std::to_string;
using std::vector;

namespace Server { namespace Web
{
	WebClient::~WebClient()
	{
		run = false;
		clientThread.join();
		delete connection;
	}

	// worker is the thread that handles client requests
	void WebClient::Worker()
	{
		Utils::Utils::SetThreadName("WebClient");
		logger->Debug(Languages::TextTcpConnectionEstablished, connection->AddressAsString());
		WorkerImpl();
		logger->Debug(Languages::TextTcpConnectionClosed, connection->AddressAsString());
		terminated = true;
	}

	void WebClient::WorkerImpl()
	{
		run = true;
		bool keepalive = true;

		while (run && keepalive)
		{
			const int BufferSize = 8192;
			char buffer[BufferSize];
			memset(buffer, 0, BufferSize);

			int pos = 0;
			string s;
			while (pos < BufferSize - 1 && s.find("\n\n") == string::npos && run)
			{
				int ret = connection->Receive(buffer + pos, BufferSize - 1 - pos, 0);
				if (ret == -1)
				{
					if (errno != ETIMEDOUT)
					{
						logger->Debug(Languages::TextErrorReadingData, strerror(errno));
						return;
					}
					if (run == false)
					{
						return;
					}
					continue;
				}
				pos += ret;
				s = string(buffer);
				Utils::Utils::ReplaceString(s, string("\r\n"), string("\n"));
				Utils::Utils::ReplaceString(s, string("\r"), string("\n"));
			}

			deque<string> lines;
			Utils::Utils::SplitString(s, string("\n"), lines);

			if (lines.size() <= 1)
			{
				return;
			}

			string method;
			string uri;
			string protocol;
			map<string, string> arguments;
			map<string, string> headers;
			InterpretClientRequest(lines, method, uri, protocol, arguments, headers);
			keepalive = (Utils::Utils::GetStringMapEntry(headers, "Connection", "close").compare("keep-alive") == 0);
			logger->Info(Languages::TextHttpRequest, method, uri);

			// if method is not implemented
			if ((method.compare("GET") != 0) && (method.compare("HEAD") != 0))
			{
				logger->Info(Languages::TextMethodNotImplemented, id, method);
				ResponseHtmlNotImplemented response(method);
				connection->Send(response);
				return;
			}

			// handle requests
			if (uri.compare("/") == 0)
			{
				PrintMainHTML();
				if (server.UpdateAvailable())
				{
					server.AddUpdate("warning", Languages::TextRailControlUpdateAvailable);
				}
			}
			else if (arguments["cmd"].compare("quit") == 0)
			{
				ReplyHtmlWithHeaderAndParagraph(Languages::TextStoppingRailControl);
				stopRailControlWebserver();
			}
			else if (arguments["cmd"].compare("booster") == 0)
			{
				bool on = Utils::Utils::GetBoolMapEntry(arguments, "on");
				if (on)
				{
					ReplyHtmlWithHeaderAndParagraph(Languages::TextTurningBoosterOn);
					manager.Booster(ControlTypeWebServer, BoosterStateGo);
				}
				else
				{
					ReplyHtmlWithHeaderAndParagraph(Languages::TextTurningBoosterOff);
					manager.Booster(ControlTypeWebServer, BoosterStateStop);
				}
			}
			else if (arguments["cmd"].compare("layeredit") == 0)
			{
				HandleLayerEdit(arguments);
			}
			else if (arguments["cmd"].compare("layersave") == 0)
			{
				HandleLayerSave(arguments);
			}
			else if (arguments["cmd"].compare("layerlist") == 0)
			{
				HandleLayerList();
			}
			else if (arguments["cmd"].compare("layeraskdelete") == 0)
			{
				HandleLayerAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("layerdelete") == 0)
			{
				HandleLayerDelete(arguments);
			}
			else if (arguments["cmd"].compare("controledit") == 0)
			{
				HandleControlEdit(arguments);
			}
			else if (arguments["cmd"].compare("controlsave") == 0)
			{
				HandleControlSave(arguments);
			}
			else if (arguments["cmd"].compare("controllist") == 0)
			{
				HandleControlList();
			}
			else if (arguments["cmd"].compare("controlaskdelete") == 0)
			{
				HandleControlAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("controldelete") == 0)
			{
				HandleControlDelete(arguments);
			}
			else if (arguments["cmd"].compare("loco") == 0)
			{
				HandleLoco(arguments);
			}
			else if (arguments["cmd"].compare("locospeed") == 0)
			{
				HandleLocoBaseSpeed(arguments);
			}
			else if (arguments["cmd"].compare("locoorientation") == 0)
			{
				HandleLocoBaseOrientation(arguments);
			}
			else if (arguments["cmd"].compare("locofunction") == 0)
			{
				HandleLocoFunction(arguments);
			}
			else if (arguments["cmd"].compare("locoedit") == 0)
			{
				HandleLocoEdit(arguments);
			}
			else if (arguments["cmd"].compare("locosave") == 0)
			{
				HandleLocoSave(arguments);
			}
			else if (arguments["cmd"].compare("locolist") == 0)
			{
				HandleLocoList();
			}
			else if (arguments["cmd"].compare("locoaskdelete") == 0)
			{
				HandleLocoAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("locodelete") == 0)
			{
				HandleLocoDelete(arguments);
			}
			else if (arguments["cmd"].compare("locorelease") == 0)
			{
				HandleLocoRelease(arguments);
			}
			else if (arguments["cmd"].compare("locoaddtimetable") == 0)
			{
				HandleLocoAddTimeTable(arguments);
			}
			else if (arguments["cmd"].compare("multipleunitedit") == 0)
			{
				HandleMultipleUnitEdit(arguments);
			}
			else if (arguments["cmd"].compare("multipleunitsave") == 0)
			{
				HandleMultipleUnitSave(arguments);
			}
			else if (arguments["cmd"].compare("multipleunitlist") == 0)
			{
				HandleMultipleUnitList();
			}
			else if (arguments["cmd"].compare("multipleunitaskdelete") == 0)
			{
				HandleMultipleUnitAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("multipleunitdelete") == 0)
			{
				HandleMultipleUnitDelete(arguments);
			}
			else if (arguments["cmd"].compare("multipleunitrelease") == 0)
			{
				HandleMultipleUnitRelease(arguments);
			}
			else if (arguments["cmd"].compare("accessoryedit") == 0)
			{
				HandleAccessoryEdit(arguments);
			}
			else if (arguments["cmd"].compare("accessorysave") == 0)
			{
				HandleAccessorySave(arguments);
			}
			else if (arguments["cmd"].compare("accessorystate") == 0)
			{
				HandleAccessoryState(arguments);
			}
			else if (arguments["cmd"].compare("accessorylist") == 0)
			{
				HandleAccessoryList();
			}
			else if (arguments["cmd"].compare("accessoryaskdelete") == 0)
			{
				HandleAccessoryAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("accessorydelete") == 0)
			{
				HandleAccessoryDelete(arguments);
			}
			else if (arguments["cmd"].compare("accessoryget") == 0)
			{
				HandleAccessoryGet(arguments);
			}
			else if (arguments["cmd"].compare("accessoryrelease") == 0)
			{
				HandleAccessoryRelease(arguments);
			}
			else if (arguments["cmd"].compare("switchedit") == 0)
			{
				HandleSwitchEdit(arguments);
			}
			else if (arguments["cmd"].compare("switchsave") == 0)
			{
				HandleSwitchSave(arguments);
			}
			else if (arguments["cmd"].compare("switchstate") == 0)
			{
				HandleSwitchState(arguments);
			}
			else if (arguments["cmd"].compare("switchstates") == 0)
			{
				route.HandleRelationSwitchStates(arguments);
			}
			else if (arguments["cmd"].compare("switchlist") == 0)
			{
				HandleSwitchList();
			}
			else if (arguments["cmd"].compare("switchaskdelete") == 0)
			{
				HandleSwitchAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("switchdelete") == 0)
			{
				HandleSwitchDelete(arguments);
			}
			else if (arguments["cmd"].compare("switchget") == 0)
			{
				HandleSwitchGet(arguments);
			}
			else if (arguments["cmd"].compare("switchrelease") == 0)
			{
				HandleSwitchRelease(arguments);
			}
			else if (arguments["cmd"].compare("signaladdresses") == 0)
			{
				signal.HandleSignalAddresses(arguments);
			}
			else if (arguments["cmd"].compare("signaledit") == 0)
			{
				signal.HandleSignalEdit(arguments);
			}
			else if (arguments["cmd"].compare("signalsave") == 0)
			{
				signal.HandleSignalSave(arguments);
			}
			else if (arguments["cmd"].compare("signalstate") == 0)
			{
				signal.HandleSignalState(arguments);
			}
			else if (arguments["cmd"].compare("signalstates") == 0)
			{
				signal.HandleSignalStates(arguments);
			}
			else if (arguments["cmd"].compare("signallist") == 0)
			{
				signal.HandleSignalList();
			}
			else if (arguments["cmd"].compare("signalaskdelete") == 0)
			{
				signal.HandleSignalAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("signaldelete") == 0)
			{
				signal.HandleSignalDelete(arguments);
			}
			else if (arguments["cmd"].compare("signalget") == 0)
			{
				signal.HandleSignalGet(arguments);
			}
			else if (arguments["cmd"].compare("signalrelease") == 0)
			{
				signal.HandleSignalRelease(arguments);
			}
			else if (arguments["cmd"].compare("routeedit") == 0)
			{
				route.HandleRouteEdit(arguments);
			}
			else if (arguments["cmd"].compare("routesave") == 0)
			{
				route.HandleRouteSave(arguments);
			}
			else if (arguments["cmd"].compare("routelist") == 0)
			{
				route.HandleRouteList();
			}
			else if (arguments["cmd"].compare("routeaskdelete") == 0)
			{
				route.HandleRouteAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("routedelete") == 0)
			{
				route.HandleRouteDelete(arguments);
			}
			else if (arguments["cmd"].compare("routeget") == 0)
			{
				route.HandleRouteGet(arguments);
			}
			else if (arguments["cmd"].compare("routeexecute") == 0)
			{
				route.HandleRouteExecute(arguments);
			}
			else if (arguments["cmd"].compare("routerelease") == 0)
			{
				route.HandleRouteRelease(arguments);
			}
			else if (arguments["cmd"].compare("textedit") == 0)
			{
				text.HandleTextEdit(arguments);
			}
			else if (arguments["cmd"].compare("textsave") == 0)
			{
				text.HandleTextSave(arguments);
			}
			else if (arguments["cmd"].compare("textlist") == 0)
			{
				text.HandleTextList();
			}
			else if (arguments["cmd"].compare("textaskdelete") == 0)
			{
				text.HandleTextAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("textdelete") == 0)
			{
				text.HandleTextDelete(arguments);
			}
			else if (arguments["cmd"].compare("textget") == 0)
			{
				text.HandleTextGet(arguments);
			}
			else if (arguments["cmd"].compare("trackedit") == 0)
			{
				track.HandleTrackEdit(arguments);
			}
			else if (arguments["cmd"].compare("tracksave") == 0)
			{
				track.HandleTrackSave(arguments);
			}
			else if (arguments["cmd"].compare("tracklist") == 0)
			{
				track.HandleTrackList();
			}
			else if (arguments["cmd"].compare("trackaskdelete") == 0)
			{
				track.HandleTrackAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("trackdelete") == 0)
			{
				track.HandleTrackDelete(arguments);
			}
			else if (arguments["cmd"].compare("trackget") == 0)
			{
				track.HandleTrackGet(arguments);
			}
			else if (arguments["cmd"].compare("tracksetloco") == 0)
			{
				track.HandleTrackSetLoco(arguments);
			}
			else if (arguments["cmd"].compare("trackrelease") == 0)
			{
				track.HandleTrackRelease(arguments);
			}
			else if (arguments["cmd"].compare("trackstartloco") == 0)
			{
				track.HandleTrackStartLoco(arguments);
			}
			else if (arguments["cmd"].compare("trackstoploco") == 0)
			{
				track.HandleTrackStopLoco(arguments);
			}
			else if (arguments["cmd"].compare("trackblock") == 0)
			{
				track.HandleTrackBlock(arguments);
			}
			else if (arguments["cmd"].compare("trackorientation") == 0)
			{
				track.HandleTrackOrientation(arguments);
			}
			else if (arguments["cmd"].compare("feedbackedit") == 0)
			{
				HandleFeedbackEdit(arguments);
			}
			else if (arguments["cmd"].compare("feedbacksave") == 0)
			{
				HandleFeedbackSave(arguments);
			}
			else if (arguments["cmd"].compare("feedbackstate") == 0)
			{
				HandleFeedbackState(arguments);
			}
			else if (arguments["cmd"].compare("feedbacklist") == 0)
			{
				HandleFeedbackList();
			}
			else if (arguments["cmd"].compare("feedbackaskdelete") == 0)
			{
				HandleFeedbackAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("feedbackdelete") == 0)
			{
				HandleFeedbackDelete(arguments);
			}
			else if (arguments["cmd"].compare("feedbackget") == 0)
			{
				HandleFeedbackGet(arguments);
			}
			else if (arguments["cmd"].compare("feedbacksoftrack") == 0)
			{
				route.HandleFeedbacksOfTrack(arguments);
			}
			else if (arguments["cmd"].compare("protocol") == 0)
			{
				HandleProtocol(arguments);
			}
			else if (arguments["cmd"].compare("feedbackadd") == 0)
			{
				HandleFeedbackAdd(arguments);
			}
			else if (arguments["cmd"].compare("relationadd") == 0)
			{
				route.HandleRelationAdd(arguments);
			}
			else if (arguments["cmd"].compare("relationobject") == 0)
			{
				route.HandleRelationObject(arguments);
			}
			else if (arguments["cmd"].compare("layout") == 0)
			{
				HandleLayout(arguments);
			}
			else if (arguments["cmd"].compare("locoselector") == 0)
			{
				HandleLocoSelector(arguments);
			}
			else if (arguments["cmd"].compare("layerselector") == 0)
			{
				HandleLayerSelector(arguments);
			}
			else if (arguments["cmd"].compare("stopallimmediately") == 0)
			{
				manager.LocoBaseStopAllImmediately(ControlTypeWebServer);
			}
			else if (arguments["cmd"].compare("startall") == 0)
			{
				manager.LocoBaseStartAll();
			}
			else if (arguments["cmd"].compare("stopall") == 0)
			{
				manager.LocoBaseStopAll();
			}
			else if (arguments["cmd"].compare("settingsedit") == 0)
			{
				HandleSettingsEdit();
			}
			else if (arguments["cmd"].compare("settingssave") == 0)
			{
				HandleSettingsSave(arguments);
			}
			else if (arguments["cmd"].compare("slaveadd") == 0)
			{
				HandleSlaveAdd(arguments);
			}
			else if (arguments["cmd"].compare("timestamp") == 0)
			{
				HandleTimestamp(arguments);
			}
			else if (arguments["cmd"].compare("controlarguments") == 0)
			{
				HandleControlArguments(arguments);
			}
			else if (arguments["cmd"].compare("program") == 0)
			{
				HandleProgram();
			}
			else if (arguments["cmd"].compare("programmodeselector") == 0)
			{
				HandleProgramModeSelector(arguments);
			}
			else if (arguments["cmd"].compare("programread") == 0)
			{
				HandleProgramRead(arguments);
			}
			else if (arguments["cmd"].compare("programwrite") == 0)
			{
				HandleProgramWrite(arguments);
			}
			else if (arguments["cmd"].compare("getcvfields") == 0)
			{
				HandleCvFields(arguments);
			}
			else if (arguments["cmd"].compare("clusterlist") == 0)
			{
				cluster.HandleClusterList();
			}
			else if (arguments["cmd"].compare("clusteredit") == 0)
			{
				cluster.HandleClusterEdit(arguments);
			}
			else if (arguments["cmd"].compare("clustersave") == 0)
			{
				cluster.HandleClusterSave(arguments);
			}
			else if (arguments["cmd"].compare("clusteraskdelete") == 0)
			{
				cluster.HandleClusterAskDelete(arguments);
			}
			else if (arguments["cmd"].compare("clusterdelete") == 0)
			{
				cluster.HandleClusterDelete(arguments);
			}
			else if (arguments["cmd"].compare("newposition") == 0)
			{
				HandleNewPosition(arguments);
			}
			else if (arguments["cmd"].compare("rotate") == 0)
			{
				HandleRotate(arguments);
			}
			else if (arguments["cmd"].compare("getlocolist") == 0)
			{
				string s = manager.GetLocoList();
				connection->Send(ResponseCsv(s));
			}
			else if (arguments["cmd"].compare("getroutelist") == 0)
			{
				string s = manager.GetRouteList();
				connection->Send(ResponseCsv(s));
			}
			else if (arguments["cmd"].compare("updater") == 0)
			{
				HandleUpdater(headers);
			}
			else
			{
				DeliverFile(uri);
			}
		}
	}

	void WebClient::InterpretClientRequest(const deque<string>& lines, string& method, string& uri, string& protocol, map<string,string>& arguments, map<string,string>& headers)
	{
		if (lines.size() == 0)
		{
			return;
		}

		for (auto& line : lines)
		{
			if (line.find("HTTP/1.") == string::npos)
			{
				deque<string> list;
				Utils::Utils::SplitString(line, string(": "), list);
				if (list.size() == 2)
				{
					headers[list[0]] = list[1];
				}
				continue;
			}

			deque<string> list;
			Utils::Utils::SplitString(line, string(" "), list);
			if (list.size() != 3)
			{
				continue;
			}

			method = list[0];
			// transform method to uppercase
			std::transform(method.begin(), method.end(), method.begin(), ::toupper);

			// if method == HEAD set membervariable
			headOnly = method.compare("HEAD") == 0;

			// set uri and protocol
			uri = list[1];
			protocol = list[2];

			// read GET-arguments from uri
			deque<string> uriParts;
			Utils::Utils::SplitString(uri, "?", uriParts);
			if (uriParts.size() != 2)
			{
				continue;
			}

			deque<string> argumentStrings;
			Utils::Utils::SplitString(uriParts[1], "&", argumentStrings);
			for (auto& argument : argumentStrings)
			{
				if (argument.length() == 0)
				{
					continue;
				}
				string key;
				string value;
				Utils::Utils::SplitString(argument, "=", key, value);
				arguments[key] = Utils::Utils::UrlDecode(value);
			}
		}
	}

	void WebClient::DeliverFile(const string& virtualFile)
	{
		std::stringstream ss;
		char workingDir[128];
		if (getcwd(workingDir, sizeof(workingDir)))
		{
			ss << workingDir << "/html" << virtualFile;
		}
		string sFile = ss.str();
		const char* realFile = sFile.c_str();
		FILE* f = fopen(realFile, "r");
		if (f == nullptr)
		{
			ResponseHtmlNotFound response(virtualFile);
			connection->Send(response);
			logger->Info(Languages::TextFileNotFound, virtualFile);
			return;
		}

		DeliverFileInternal(f, realFile, virtualFile);
		fclose(f);
	}

	void WebClient::DeliverFileInternal(FILE* f, const char* realFile, const string& virtualFile)
	{
		struct stat s;
		int rc = stat(realFile, &s);
		if (rc != 0)
		{
			return;
		}

		size_t length = virtualFile.length();
		const char* contentType = nullptr;
		if (length > 4 && virtualFile[length - 4] == '.')
		{
			if (virtualFile[length - 3] == 'i' && virtualFile[length - 2] == 'c' && virtualFile[length - 1] == 'o')
			{
				contentType = "image/x-icon";
			}
			else if (virtualFile[length - 3] == 'c' && virtualFile[length - 2] == 's' && virtualFile[length - 1] == 's')
			{
				contentType = "text/css";
			}
			else if (virtualFile[length - 3] == 'p' && virtualFile[length - 2] == 'n' && virtualFile[length - 1] == 'g')
			{
				contentType = "image/png";
			}
			else if (virtualFile[length - 3] == 't' && virtualFile[length - 2] == 't' && virtualFile[length - 1] == 'f')
			{
				contentType = "application/x-font-ttf";
			}
		}
		else if (length > 3 && virtualFile[length - 3] == '.' && virtualFile[length - 2] == 'j' && virtualFile[length - 1] == 's')
		{
			contentType = "application/javascript";
		}

		Response response;
		response.AddHeader("Cache-Control", "no-cache, must-revalidate");
		response.AddHeader("Pragma", "no-cache");
		response.AddHeader("Expires", "Sun, 12 Feb 2016 00:00:00 GMT");
		response.AddHeader("Content-Length", to_string(s.st_size));
		if (contentType != nullptr)
		{
			response.AddHeader("Content-Type", contentType);
		}
		connection->Send(response);

		if (headOnly == true)
		{
			return;
		}

		char* buffer = static_cast<char*>(malloc(s.st_size));
		if (buffer == nullptr)
		{
			return;
		}

		size_t r = fread(buffer, 1, s.st_size, f);
		connection->Send(buffer, r, 0);
		free(buffer);
	}

	void WebClient::HandleLayerEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		LayerID layerID = Utils::Utils::GetIntegerMapEntry(arguments, "layer", LayerNone);
		string name = Languages::GetText(Languages::TextNew);

		if (layerID != LayerNone)
		{
			Layer* layer = manager.GetLayer(layerID);
			if (layer != nullptr)
			{
				name = layer->GetName();
			}
		}

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag form("form");
		form.AddId("editform");
		form.AddChildTag(HtmlTagInputHidden("cmd", "layersave"));
		form.AddChildTag(HtmlTagInputHidden("layer", to_string(layerID)));
		form.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(form));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleLayerSave(const map<string, string>& arguments)
	{
		LayerID layerID = Utils::Utils::GetIntegerMapEntry(arguments, "layer", LayerNone);
		string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		string result;

		if (!manager.LayerSave(layerID, name, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextLayerSaved, name);
	}

	void WebClient::HandleLayerAskDelete(const map<string, string>& arguments)
	{
		LayerID layerID = Utils::Utils::GetIntegerMapEntry(arguments, "layer", LayerNone);

		if (layerID == LayerNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextLayerDoesNotExist);
			return;
		}

		if (layerID == LayerUndeletable)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextLayer1IsUndeletable);
			return;
		}

		const Layer* layer = manager.GetLayer(layerID);
		if (layer == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::GetText(Languages::TextLayerDoesNotExist));
			return;
		}

		HtmlTag content;
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteLayer));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, layer->GetName()));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "layerdelete"))
			.AddContent(HtmlTagInputHidden("layer", to_string(layerID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleLayerDelete(const map<string, string>& arguments)
	{
		LayerID layerID = Utils::Utils::GetIntegerMapEntry(arguments, "layer", LayerNone);

		if (layerID == LayerNone)
		{
			ReplyResponse(ResponseError, Languages::TextLayerDoesNotExist);
			return;
		}

		if (layerID == LayerUndeletable)
		{
			ReplyResponse(ResponseError, Languages::TextLayer1IsUndeletable);
			return;
		}

		const Layer* layer = manager.GetLayer(layerID);
		if (layer == nullptr)
		{
			ReplyResponse(ResponseError, Languages::TextLayerDoesNotExist);
			return;
		}

		string name = layer->GetName();
		string result;
		if (!manager.LayerDelete(layerID, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextLayerDeleted, name);
	}

	void WebClient::HandleLayerList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextLayers));
		HtmlTag table("table");
		const map<string,LayerID> layerList = manager.LayerListByName();
		map<string,string> layerArgument;
		for (auto& layer : layerList)
		{
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(layer.first));
			string layerIdString = to_string(layer.second);
			layerArgument["layer"] = layerIdString;
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "layeredit_list_" + layerIdString, layerArgument)));
			if (layer.second != LayerUndeletable)
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "layeraskdelete_" + layerIdString, layerArgument)));
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "layeredit_0"));
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleControlEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		ControlID controlID = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		HardwareType hardwareType = HardwareTypeNone;
		string name = Languages::GetText(Languages::TextNew);
		string arg1;
		string arg2;
		string arg3;
		string arg4;
		string arg5;

		if (controlID != ControlIdNone)
		{
			Hardware::HardwareParams* params = manager.GetHardware(controlID);
			if (params != nullptr)
			{
				hardwareType = params->GetHardwareType();
				name = params->GetName();
				arg1 = params->GetArg1();
				arg2 = params->GetArg2();
				arg3 = params->GetArg3();
				arg4 = params->GetArg4();
				arg5 = params->GetArg5();
			}
		}

		const std::map<string,HardwareType> hardwareOptions = WebClientStatic::ListHardwareNames();

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag form("form");
		form.AddId("editform");
		form.AddChildTag(HtmlTagInputHidden("cmd", "controlsave"));
		form.AddChildTag(HtmlTagInputHidden("control", to_string(controlID)));
		form.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));

		HtmlTagSelectWithLabel selectHardwareType("hardwaretype", Languages::TextType, hardwareOptions, hardwareType);
		selectHardwareType.AddAttribute("onchange", "getArgumentsOfHardwareType();");
		form.AddChildTag(selectHardwareType);

		HtmlTag controlArguments("div");
		controlArguments.AddId("controlarguments");
		controlArguments.AddChildTag(WebClientStatic::HtmlTagControlArguments(hardwareType, arg1, arg2, arg3, arg4, arg5));
		form.AddChildTag(controlArguments);

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(form));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleControlSave(const map<string, string>& arguments)
	{
		ControlID controlID = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		HardwareType hardwareType = static_cast<HardwareType>(Utils::Utils::GetIntegerMapEntry(arguments, "hardwaretype", HardwareTypeNone));
		string arg1 = Utils::Utils::GetStringMapEntry(arguments, "arg1");
		string arg2 = Utils::Utils::GetStringMapEntry(arguments, "arg2");
		string arg3 = Utils::Utils::GetStringMapEntry(arguments, "arg3");
		string arg4 = Utils::Utils::GetStringMapEntry(arguments, "arg4");
		string arg5 = Utils::Utils::GetStringMapEntry(arguments, "arg5");
		string result;

		if (!manager.ControlSave(controlID, hardwareType, name, arg1, arg2, arg3, arg4, arg5, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextControlSaved, name);
	}

	void WebClient::HandleControlAskDelete(const map<string, string>& arguments)
	{
		ControlID controlID = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlNone);

		if (controlID == ControlNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextControlDoesNotExist);
			return;
		}

		const Hardware::HardwareParams* control = manager.GetHardware(controlID);
		if (control == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextControlDoesNotExist);
			return;
		}

		HtmlTag content;
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteControl));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, control->GetName()));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "controldelete"))
			.AddContent(HtmlTagInputHidden("control", to_string(controlID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleControlDelete(const map<string, string>& arguments)
	{
		ControlID controlID = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlNone);
		const Hardware::HardwareParams* control = manager.GetHardware(controlID);
		if (control == nullptr)
		{
			ReplyResponse(ResponseError, Languages::TextControlDoesNotExist);
			return;
		}

		string name = control->GetName();

		if (!manager.ControlDelete(controlID))
		{
			ReplyResponse(ResponseError, Languages::TextControlDoesNotExist);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextControlDeleted, name);
	}

	void WebClient::HandleControlList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextControls));
		HtmlTag table("table");
		const map<string,Hardware::HardwareParams*> hardwareList = manager.ControlListByName();
		map<string,string> hardwareArgument;
		for (auto& hardware : hardwareList)
		{
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(hardware.first));
			string controlIdString = to_string(hardware.second->GetControlID());
			hardwareArgument["control"] = controlIdString;
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "controledit_list_" + controlIdString, hardwareArgument)));
			row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "controlaskdelete_" + controlIdString, hardwareArgument)));
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "controledit_0"));
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleLocoBaseSpeed(const map<string, string>& arguments)
	{
		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		const Speed speed = Utils::Utils::GetIntegerMapEntry(arguments, "speed", MinSpeed);

		const ObjectIdentifier locoBaseIdentifier(WebClientStatic::LocoIdToObjectIdentifier(locoID));

		manager.LocoBaseSpeed(ControlTypeWebServer, locoBaseIdentifier, speed);

		ReplyHtmlWithHeaderAndParagraph(Languages::TextLocoSpeedIs, manager.GetLocoBaseName(locoBaseIdentifier), speed);
	}

	void WebClient::HandleLocoBaseOrientation(const map<string, string>& arguments)
	{
		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		const Orientation orientation = (Utils::Utils::GetBoolMapEntry(arguments, "on") ? OrientationRight : OrientationLeft);

		const ObjectIdentifier locoBaseIdentifier(WebClientStatic::LocoIdToObjectIdentifier(locoID));

		manager.LocoBaseOrientation(ControlTypeWebServer, locoBaseIdentifier, orientation);

		ReplyHtmlWithHeaderAndParagraph(orientation == OrientationLeft ? Languages::TextLocoDirectionOfTravelIsLeft : Languages::TextLocoDirectionOfTravelIsRight, manager.GetLocoBaseName(locoBaseIdentifier));
	}

	void WebClient::HandleLocoFunction(const map<string, string>& arguments)
	{
		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		const DataModel::LocoFunctionNr function = Utils::Utils::GetIntegerMapEntry(arguments, "function", 0);
		const DataModel::LocoFunctionState state = static_cast<DataModel::LocoFunctionState>(Utils::Utils::GetBoolMapEntry(arguments, "on"));

		const ObjectIdentifier locoBaseIdentifier(WebClientStatic::LocoIdToObjectIdentifier(locoID));

		manager.LocoBaseFunctionState(ControlTypeWebServer, locoBaseIdentifier, function, state);

		ReplyHtmlWithHeaderAndParagraph(state ? Languages::TextLocoFunctionIsOn : Languages::TextLocoFunctionIsOff, manager.GetLocoBaseName(locoBaseIdentifier), function);
	}

	void WebClient::HandleLocoRelease(const map<string, string>& arguments)
	{
		bool ret = false;
		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		if (locoID != LocoNone)
		{
			ret = manager.LocoRelease(locoID);
		}
		else
		{
			TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
			ret = manager.LocoBaseReleaseOnTrack(trackID);
		}
		ReplyHtmlWithHeaderAndParagraph(ret ? "Loco released" : "Loco not released");
	}

	void WebClient::HandleMultipleUnitRelease(const map<string, string>& arguments)
	{
		bool ret = false;
		MultipleUnitID multipleUnitID = Utils::Utils::GetIntegerMapEntry(arguments, "multipleunit", MultipleUnitNone);
		if (multipleUnitID != LocoNone)
		{
			ret = manager.MultipleUnitRelease(multipleUnitID);
		}
		else
		{
			TrackID trackID = static_cast<TrackID>(Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone));
			ret = manager.LocoBaseReleaseOnTrack(trackID);
		}
		ReplyHtmlWithHeaderAndParagraph(ret ? "Loco released" : "Loco not released");
	}

	void WebClient::HandleLocoAddTimeTable(const map<string, string>& arguments)
	{
		const TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone);
		const ObjectIdentifier locoBaseIdentifier = manager.GetLocoBaseIdentifierOfTrack(trackID);
		const RouteID routeID = static_cast<RouteID>(Utils::Utils::GetIntegerMapEntry(arguments, "route"));
		const bool automode = Utils::Utils::GetStringMapEntry(arguments, "followup", "manual").compare("automode") == 0;
		bool ret = manager.LocoBaseAddTimeTable(locoBaseIdentifier, routeID, automode);
		manager.TrackStartLocoBase(trackID);
		ReplyHtmlWithHeaderAndParagraph(ret ? "Route added" : "Route not added");
	}

	HtmlTag WebClient::HtmlTagMatchKeyProtocolLoco(const ControlID controlId,
		const string& selectedMatchKey,
		const Protocol selectedProtocol) const
	{
		HtmlTag content;
		map<string,LocoConfig> matchKeyMap = manager.GetUnmatchedLocosOfControl(controlId, selectedMatchKey);
		content.AddChildTag(WebClientStatic::HtmlTagMatchKey(matchKeyMap, selectedMatchKey));
		map<string,Protocol> protocolMap = manager.LocoProtocolsOfControl(controlId);
		content.AddChildTag(WebClientStatic::HtmlTagProtocol(protocolMap, selectedProtocol));
		return content;
	}

	HtmlTag WebClient::HtmlTagMatchKeyProtocolAccessory(const ControlID controlId,
		const string& selectedMatchKey,
		const Protocol selectedProtocol) const
	{
		HtmlTag content;
		map<string,AccessoryConfig> matchKeyMap = manager.GetUnmatchedAccessoriesOfControl(controlId, selectedMatchKey);
		content.AddChildTag(WebClientStatic::HtmlTagMatchKey(matchKeyMap, selectedMatchKey));
		map<string,Protocol> protocolMap = manager.AccessoryProtocolsOfControl(controlId);
		content.AddChildTag(WebClientStatic::HtmlTagProtocol(protocolMap, selectedProtocol));
		return content;
	}

	HtmlTag WebClient::HtmlTagMatchKeyFeedback(const ControlID controlId,
		const string& selectedMatchKey) const
	{
		HtmlTag content;
		map<string,FeedbackConfig> matchKeyMap = manager.GetUnmatchedFeedbacksOfControl(controlId, selectedMatchKey);
		content.AddChildTag(WebClientStatic::HtmlTagMatchKey(matchKeyMap, selectedMatchKey));
		return content;
	}

	HtmlTag WebClient::HtmlTagProtocolAccessory(const ControlID controlID, const Protocol selectedProtocol)
	{
		map<string,Protocol> protocolMap = manager.AccessoryProtocolsOfControl(controlID);
		return WebClientStatic::HtmlTagProtocol(protocolMap, selectedProtocol);
	}

	void WebClient::HandleProtocol(const map<string, string>& arguments)
	{
		ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		if (controlId == ControlIdNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextControlDoesNotExist);
			return;
		}

		if (Utils::Utils::IsMapEntrySet(arguments, "loco"))
		{
			LocoID locoId = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
			string matchKey;
			Protocol protocol = ProtocolNone;
			Loco* loco = manager.GetLoco(locoId);
			if (loco != nullptr)
			{
				matchKey = loco->GetMatchKey();
				protocol = loco->GetProtocol();
			}
			ReplyHtmlWithHeader(HtmlTagMatchKeyProtocolLoco(controlId, matchKey, protocol));
			return;
		}
		AccessoryID accessoryId = Utils::Utils::GetIntegerMapEntry(arguments, "accessory", AccessoryNone);
		if (accessoryId != AccessoryNone)
		{
			Accessory *accessory = manager.GetAccessory(accessoryId);
			ReplyHtmlWithHeader(HtmlTagProtocolAccessory(controlId, accessory == nullptr ? ProtocolNone : accessory->GetProtocol()));
			return;
		}
		SwitchID switchId = Utils::Utils::GetIntegerMapEntry(arguments, "switch", SwitchNone);
		if (switchId != SwitchNone)
		{
			Switch *mySwitch = manager.GetSwitch(switchId);
			ReplyHtmlWithHeader(HtmlTagProtocolAccessory(controlId, mySwitch == nullptr ? ProtocolNone : mySwitch->GetProtocol()));
			return;
		}
		SignalID signalId = Utils::Utils::GetIntegerMapEntry(arguments, "signal", SignalNone);
		if (signalId != SignalNone)
		{
			Signal *signal = manager.GetSignal(signalId);
			ReplyHtmlWithHeader(HtmlTagProtocolAccessory(controlId, signal == nullptr ? ProtocolNone : signal->GetProtocol()));
			return;
		}

		ReplyHtmlWithHeader(HtmlTagProtocolAccessory(controlId, ProtocolNone));
	}

	HtmlTag WebClient::HtmlTagPosition(const LayoutPosition posx,
		const LayoutPosition posy,
		const LayoutPosition posz) const
	{
		HtmlTag content("div");
		content.AddId("position");;
		content.AddChildTag(HtmlTagInputIntegerWithLabel("posx", Languages::TextPosX, posx, 0, 255));
		content.AddChildTag(HtmlTagInputIntegerWithLabel("posy", Languages::TextPosY, posy, 0, 255));
		map<string,LayerID> layerList = manager.LayerListByName();
		content.AddChildTag(HtmlTagSelectWithLabel("posz", Languages::TextPosZ, layerList, posz));
		content.AddChildTag(HtmlTag("p").AddContent(Languages::GetText(Languages::TextHint)).AddContent(HtmlTag("br")).AddContent(Languages::GetText(Languages::TextHintPositionMove)));
		return content;
	}

	HtmlTag WebClient::HtmlTagPosition(const LayoutPosition posx,
		const LayoutPosition posy,
		const LayoutPosition posz,
		const Visible visible) const
	{
		HtmlTag content;
		HtmlTagInputCheckboxWithLabel checkboxVisible("visible", Languages::TextVisible, "visible", static_cast<bool>(visible));
		checkboxVisible.AddId("visible");
		checkboxVisible.AddAttribute("onchange", "onChangeCheckboxShowHide('visible', 'position');");
		content.AddChildTag(checkboxVisible);
		HtmlTag posDiv = HtmlTagPosition(posx, posy, posz);
		if (visible == DataModel::LayoutItem::VisibleNo)
		{
			posDiv.AddAttribute("hidden");
		}
		content.AddChildTag(posDiv);
		return content;
	}

	HtmlTag WebClient::HtmlTagSlaveSelect(const string& prefix,
		const vector<Relation*>& relations,
		const map<string,ObjectID>& options,
		const bool allowNew) const
	{
		HtmlTag content("div");
		content.AddId("tab_" + prefix + "s");
		content.AddClass("tab_content");
		content.AddClass("hidden");

		HtmlTag div("div");
		div.AddChildTag(HtmlTagInputHidden(prefix + "counter", to_string(relations.size())));
		div.AddId(prefix + "s");

		unsigned int counter = 1;
		for (auto relation : relations)
		{
			ObjectID objectID = relation->ObjectID2();
			div.AddChildTag(WebClientStatic::HtmlTagSlaveEntry(prefix, to_string(counter), objectID, options));
			++counter;
		}
		div.AddChildTag(HtmlTag("div").AddId(prefix + "_new_" + to_string(counter)));

		content.AddChildTag(div);
		if (allowNew)
		{
			HtmlTagButton newTrackButton(Languages::TextNew, "new" + prefix);
			newTrackButton.AddAttribute("onclick", "addSlave('" + prefix + "');return false;");
			newTrackButton.AddClass("wide_button");
			content.AddChildTag(newTrackButton);
		}
		content.AddChildTag(HtmlTag("br"));
		return content;
	}

	HtmlTag WebClient::HtmlTagSelectFeedbackForTrack(const unsigned int counter,
		const TrackID trackID,
		const FeedbackID feedbackID) const
	{
		string counterString = to_string(counter);
		HtmlTag content("div");
		content.AddId("feedback_container_" + counterString);
		HtmlTagButton deleteButton(Languages::TextDelete, "delete_feedback_" + counterString);
		deleteButton.AddAttribute("onclick", "deleteElement('feedback_container_" + counterString + "');return false;");
		deleteButton.AddClass("wide_button");
		content.AddChildTag(deleteButton);

		map<string, Feedback*> feedbacks = manager.FeedbackListByName();
		map<string, FeedbackID> feedbackOptions;
		for (auto& feedback : feedbacks)
		{
			const Track* track = feedback.second->GetTrack();
			if ((!track) || (track->GetID() != trackID))
			{
				continue;
			}
			feedbackOptions[feedback.first] = feedback.second->GetID();
		}
		content.AddChildTag(HtmlTagSelect("feedback_" + counterString, feedbackOptions, feedbackID));
		content.AddChildTag(HtmlTag("div").AddId("div_feedback_" + to_string(counter + 1)));
		return content;
	}

	void WebClient::HandleSlaveAdd(const map<string, string>& arguments)
	{
		string priorityString = Utils::Utils::GetStringMapEntry(arguments, "priority", "1");
		Priority priority = Utils::Integer::StringToInteger(priorityString, 1);
		string prefix = Utils::Utils::GetStringMapEntry(arguments, "prefix");
		HtmlTag container;
		std::map<std::string,ObjectID> options;
		if (prefix.compare("track") == 0)
		{
			options = cluster.GetTrackOptions();
		}
		else if (prefix.compare("feedback") == 0)
		{
			options = track.GetFeedbackOptions();
		}
		else if (prefix.compare("signal") == 0)
		{
			options = track.GetSignalOptions();
		}
		else if (prefix.compare("slave") == 0)
		{
			options = GetMultipleUnitSlaveOptions();
		}
		container.AddChildTag(WebClientStatic::HtmlTagSlaveEntry(prefix, priorityString, ObjectNone, options));
		container.AddChildTag(HtmlTag("div").AddId(prefix + "_new_" + to_string(priority + 1)));
		ReplyHtmlWithHeader(container);
	}

	void WebClient::HandleFeedbackAdd(const map<string, string>& arguments)
	{
		const unsigned int counter = Utils::Utils::GetIntegerMapEntry(arguments, "counter", 1);
		const TrackID trackID = Utils::Utils::GetIntegerMapEntry(arguments, "track", TrackNone);
		ReplyHtmlWithHeader(HtmlTagSelectFeedbackForTrack(counter, trackID));
	}

	map<string,ObjectID> WebClient::GetMultipleUnitSlaveOptions() const
	{
		map<string, ObjectID> options;

		map<string, LocoConfig> allLocos = manager.LocoConfigByName();
		for (auto& loco : allLocos)
		{
			options[loco.first] = loco.second.GetLocoId();
		}
		return options;
	}

	void WebClient::HandleLocoEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		const LocoID locoId = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlNone);
		if (controlId == ControlNone)
		{
			controlId = manager.GetPossibleControlForLoco();
		}
		string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		Protocol protocol = ProtocolNone;
		Address address = AddressDefault;
		Address serverAddress = AddressNone;
		string name = Languages::GetText(Languages::TextNew);
		bool pushpull = false;
		Length length = 0;
		Speed maxSpeed = MaxSpeed;
		Speed travelSpeed = DefaultTravelSpeed;
		Speed reducedSpeed = DefaultReducedSpeed;
		Speed creepingSpeed = DefaultCreepingSpeed;
		Propulsion propulsion = PropulsionOther;
		TrainType trainType = TrainTypeOther;
		LocoFunctionEntry locoFunctions[NumberOfLocoFunctions];
		vector<Relation*> slaves;

		if (locoId > LocoNone)
		{
			// existing loco
			const DataModel::Loco* loco = manager.GetLoco(locoId);
			if (loco != nullptr)
			{
				controlId = loco->GetControlID();
				matchKey = loco->GetMatchKey();
				protocol = loco->GetProtocol();
				address = loco->GetAddress();
				serverAddress = loco->GetServerAddress();
				name = loco->GetName();
				pushpull = loco->GetPushpull();
				length = loco->GetLength();
				maxSpeed = loco->GetMaxSpeed();
				travelSpeed = loco->GetTravelSpeed();
				reducedSpeed = loco->GetReducedSpeed();
				creepingSpeed = loco->GetCreepingSpeed();
				propulsion = loco->GetPropulsion();
				trainType = loco->GetTrainType();
				loco->GetFunctions(locoFunctions);
			}
		}
		else if (controlId > ControlNone)
		{
			// loco from hardware database
			DataModel::LocoConfig loco = manager.GetLocoOfConfigByMatchKey(controlId, matchKey);

			if ((loco.GetControlId() == controlId) && (loco.GetMatchKey() == matchKey))
			{
				protocol = loco.GetProtocol();
				address = loco.GetAddress();
				name = loco.GetName();
				loco.GetFunctions(locoFunctions);
			}
		}
		// else new loco

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("basic", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("functions", Languages::TextFunctions));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("automode", Languages::TextAutomode));
		content.AddChildTag(tabMenu);

		HtmlTag formContent("form");
		formContent.AddId("editform");
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "locosave"));
		formContent.AddChildTag(HtmlTagInputHidden("loco", to_string(locoId)));

		HtmlTag basicContent("div");
		basicContent.AddId("tab_basic");
		basicContent.AddClass("tab_content");
		basicContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		basicContent.AddChildTag(HtmlTagControlLoco(controlId, "loco", locoId));
		basicContent.AddChildTag(HtmlTag("div").AddId("select_protocol").AddChildTag(HtmlTagMatchKeyProtocolLoco(controlId, matchKey, protocol)));
		basicContent.AddChildTag(HtmlTagInputIntegerWithLabel("address", Languages::TextAddress, address, 1, 9999));
		if (manager.IsServerEnabled())
		{
			basicContent.AddChildTag(HtmlTagInputIntegerWithLabel("serveraddress", Languages::TextServerAddress, serverAddress, 0, 9999));
		}
		basicContent.AddChildTag(HtmlTagInputIntegerWithLabel("length", Languages::TextTrainLength, length, 0, 99999));
		basicContent.AddChildTag(WebClientStatic::HtmlTagSelectPropulsion(propulsion));
		basicContent.AddChildTag(WebClientStatic::HtmlTagSelectTrainType(trainType));
		formContent.AddChildTag(basicContent);

		formContent.AddChildTag(WebClientStatic::HtmlTagTabFunctions(locoFunctions));

		formContent.AddChildTag(WebClientStatic::HtmlTagTabAutomode(pushpull, maxSpeed, travelSpeed, reducedSpeed, creepingSpeed));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(formContent));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleMultipleUnitEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		const MultipleUnitID multipleUnitId = Utils::Utils::GetIntegerMapEntry(arguments, "multipleunit", MultipleUnitNone);
		ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlNone);
		string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		string name = Languages::GetText(Languages::TextNew);
		bool pushpull = false;
		Address serverAddress = AddressNone;
		Length length = 0;
		Speed maxSpeed = MaxSpeed;
		Speed travelSpeed = DefaultTravelSpeed;
		Speed reducedSpeed = DefaultReducedSpeed;
		Speed creepingSpeed = DefaultCreepingSpeed;
		TrainType trainType = TrainTypeOther;
		LocoFunctionEntry locoFunctions[NumberOfLocoFunctions];
		vector<Relation*> slaves;

		if (multipleUnitId > MultipleUnitNone)
		{
			// existing multiple unit
			const DataModel::MultipleUnit* multipleUnit = manager.GetMultipleUnit(multipleUnitId);
			if (multipleUnit != nullptr)
			{
				controlId = multipleUnit->GetControlID();
				matchKey = multipleUnit->GetMatchKey();
				name = multipleUnit->GetName();
				pushpull = multipleUnit->GetPushpull();
				serverAddress = multipleUnit->GetServerAddress();
				length = multipleUnit->GetLength();
				maxSpeed = multipleUnit->GetMaxSpeed();
				travelSpeed = multipleUnit->GetTravelSpeed();
				reducedSpeed = multipleUnit->GetReducedSpeed();
				creepingSpeed = multipleUnit->GetCreepingSpeed();
				trainType = multipleUnit->GetTrainType();
				multipleUnit->GetFunctions(locoFunctions);
				slaves = multipleUnit->GetSlaves();
			}
		}
		else if (controlId > ControlNone)
		{
			// multiple unit from hardware database
			DataModel::LocoConfig multipleUnit = manager.GetMultipleUnitOfConfigByMatchKey(controlId, matchKey);

			if ((multipleUnit.GetControlId() == controlId) && (multipleUnit.GetMatchKey() == matchKey))
			{
				name = multipleUnit.GetName();
				multipleUnit.GetFunctions(locoFunctions);
				slaves = WebClientStatic::ConvertSlaveIDVectorToRelation(manager, multipleUnitId, multipleUnit.GetSlaves());
			}
		}
		// else new multiple unit

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("basic", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("functions", Languages::TextFunctions));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("slaves", Languages::TextMultipleUnit));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("automode", Languages::TextAutomode));
		content.AddChildTag(tabMenu);

		HtmlTag formContent("form");
		formContent.AddId("editform");
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "multipleunitsave"));
		formContent.AddChildTag(HtmlTagInputHidden("multipleunit", to_string(multipleUnitId)));

		HtmlTag basicContent("div");
		basicContent.AddId("tab_basic");
		basicContent.AddClass("tab_content");
		basicContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		basicContent.AddChildTag(HtmlTagControlMultipleUnit(controlId, "multipleunit", multipleUnitId));
		if (manager.IsServerEnabled())
		{
			basicContent.AddChildTag(HtmlTagInputIntegerWithLabel("serveraddress", Languages::TextServerAddress, serverAddress, 0, 9999));
		}
		basicContent.AddChildTag(HtmlTagInputIntegerWithLabel("length", Languages::TextTrainLength, length, 0, 99999));

		basicContent.AddChildTag(WebClientStatic::HtmlTagSelectTrainType(trainType));
		formContent.AddChildTag(basicContent);

		formContent.AddChildTag(WebClientStatic::HtmlTagTabFunctions(locoFunctions));

		formContent.AddChildTag(HtmlTagSlaveSelect("slave", slaves, GetMultipleUnitSlaveOptions()));

		formContent.AddChildTag(WebClientStatic::HtmlTagTabAutomode(pushpull, maxSpeed, travelSpeed, reducedSpeed, creepingSpeed));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(formContent));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleLocoSave(const map<string, string>& arguments)
	{
		const LocoID locoId = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name", Languages::GetText(Languages::TextLoco));
		const ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		const string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		const Protocol protocol = static_cast<Protocol>(Utils::Utils::GetIntegerMapEntry(arguments, "protocol", ProtocolNone));
		const Address address = Utils::Utils::GetIntegerMapEntry(arguments, "address", AddressDefault);
		const Address serverAddress = Utils::Utils::GetIntegerMapEntry(arguments, "serveraddress", AddressNone);
		const Length length = Utils::Utils::GetIntegerMapEntry(arguments, "length", 0);
		const bool pushpull = Utils::Utils::GetBoolMapEntry(arguments, "pushpull", false);
		const Speed maxSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "maxspeed", MaxSpeed);
		Speed travelSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "travelspeed", DefaultTravelSpeed);
		if (travelSpeed > maxSpeed)
		{
			travelSpeed = maxSpeed;
		}
		Speed reducedSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "reducedspeed", DefaultReducedSpeed);
		if (reducedSpeed > travelSpeed)
		{
			reducedSpeed = travelSpeed;
		}
		Speed creepingSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "creepingspeed", DefaultCreepingSpeed);
		if (creepingSpeed > reducedSpeed)
		{
			creepingSpeed = reducedSpeed;
		}
		const Propulsion propulsion = static_cast<Propulsion>(Utils::Utils::GetIntegerMapEntry(arguments, "propulsion", PropulsionOther));
		const TrainType type = static_cast<TrainType>(Utils::Utils::GetIntegerMapEntry(arguments, "type", TrainTypeOther));

		vector<DataModel::LocoFunctionEntry> locoFunctions;
		DataModel::LocoFunctionEntry locoFunctionEntry;
		for (DataModel::LocoFunctionNr nr = 0; nr < NumberOfLocoFunctions; ++nr)
		{
			string nrString = "f" + to_string(nr) + "_";
			locoFunctionEntry.nr = nr;
			locoFunctionEntry.type = static_cast<DataModel::LocoFunctionType>(Utils::Utils::GetIntegerMapEntry(arguments, nrString + "type", DataModel::LocoFunctionTypeNone));
			if (locoFunctionEntry.type == DataModel::LocoFunctionTypeNone)
			{
				continue;
			}
			locoFunctionEntry.icon = static_cast<DataModel::LocoFunctionIcon>(Utils::Utils::GetIntegerMapEntry(arguments, nrString + "icon", DataModel::LocoFunctionIconNone));
			if (locoFunctionEntry.type == DataModel::LocoFunctionTypeTimer)
			{
				locoFunctionEntry.timer = Utils::Utils::GetIntegerMapEntry(arguments, nrString + "timer", 1);
				if (locoFunctionEntry.timer == 0)
				{
					locoFunctionEntry.timer = 1;
				}
			}
			else
			{
				locoFunctionEntry.timer = 0;
			}
			locoFunctions.push_back(locoFunctionEntry);
		}

		string result;

			if (!manager.LocoSave(locoId,
				name,
				controlId,
				matchKey,
				protocol,
				address,
				serverAddress,
				length,
				pushpull,
				maxSpeed,
				travelSpeed,
				reducedSpeed,
				creepingSpeed,
				propulsion,
				type,
				locoFunctions,
				result))
			{
				ReplyResponse(ResponseError, result);
				return;
			}

		ReplyResponse(ResponseInfo, Languages::TextLocoSaved, name);
	}

	void WebClient::HandleMultipleUnitSave(__attribute__((unused)) const map<string, string>& arguments)
	{
		const MultipleUnitID multipleUnitId = Utils::Utils::GetIntegerMapEntry(arguments, "multipleunit", MultipleUnitNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		const string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		const Address address = Utils::Utils::GetIntegerMapEntry(arguments, "address", AddressDefault);
		const Address serverAddress = Utils::Utils::GetIntegerMapEntry(arguments, "serveraddress", AddressNone);
		const Length length = Utils::Utils::GetIntegerMapEntry(arguments, "length", 0);
		const bool pushpull = Utils::Utils::GetBoolMapEntry(arguments, "pushpull", false);
		const Speed maxSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "maxspeed", MaxSpeed);
		Speed travelSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "travelspeed", DefaultTravelSpeed);
		if (travelSpeed > maxSpeed)
		{
			travelSpeed = maxSpeed;
		}
		Speed reducedSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "reducedspeed", DefaultReducedSpeed);
		if (reducedSpeed > travelSpeed)
		{
			reducedSpeed = travelSpeed;
		}
		Speed creepingSpeed = Utils::Utils::GetIntegerMapEntry(arguments, "creepingspeed", DefaultCreepingSpeed);
		if (creepingSpeed > reducedSpeed)
		{
			creepingSpeed = reducedSpeed;
		}
		const TrainType type = static_cast<TrainType>(Utils::Utils::GetIntegerMapEntry(arguments, "type", TrainTypeOther));

		vector<DataModel::LocoFunctionEntry> locoFunctions;
		DataModel::LocoFunctionEntry locoFunctionEntry;
		for (DataModel::LocoFunctionNr nr = 0; nr < NumberOfLocoFunctions; ++nr)
		{
			string nrString = "f" + to_string(nr) + "_";
			locoFunctionEntry.nr = nr;
			locoFunctionEntry.type = static_cast<DataModel::LocoFunctionType>(Utils::Utils::GetIntegerMapEntry(arguments, nrString + "type", DataModel::LocoFunctionTypeNone));
			if (locoFunctionEntry.type == DataModel::LocoFunctionTypeNone)
			{
				continue;
			}
			locoFunctionEntry.icon = static_cast<DataModel::LocoFunctionIcon>(Utils::Utils::GetIntegerMapEntry(arguments, nrString + "icon", DataModel::LocoFunctionIconNone));
			if (locoFunctionEntry.type == DataModel::LocoFunctionTypeTimer)
			{
				locoFunctionEntry.timer = Utils::Utils::GetIntegerMapEntry(arguments, nrString + "timer", 1);
				if (locoFunctionEntry.timer == 0)
				{
					locoFunctionEntry.timer = 1;
				}
			}
			else
			{
				locoFunctionEntry.timer = 0;
			}
			locoFunctions.push_back(locoFunctionEntry);
		}

		vector<LocoID> slaveIDs = WebClientStatic::InterpretSlaveData("slave", arguments);
		vector<Relation*> slaves = WebClientStatic::ConvertSlaveIDVectorToRelation(manager, multipleUnitId, slaveIDs);

		string result;

		if (!manager.MultipleUnitSave(multipleUnitId,
			name,
			controlId,
			matchKey,
			address,
			serverAddress,
			length,
			pushpull,
			maxSpeed,
			travelSpeed,
			reducedSpeed,
			creepingSpeed,
			type,
			locoFunctions,
			slaves,
			result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextMultipleUnitSaved, name);
	}

	void WebClient::HandleLocoList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextLocos));
		HtmlTag table("table");
		const map<string,LocoConfig> locoList = manager.LocoConfigByName();
		map<string,string> locoArgument;
		for (auto& loco : locoList)
		{
			const LocoConfig& locoConfig = loco.second;
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(loco.first));
			row.AddChildTag(HtmlTag("td").AddContent(ProtocolName(locoConfig.GetProtocol())));
			row.AddChildTag(HtmlTag("td").AddContent(to_string(locoConfig.GetAddress())));
			const LocoID locoId = locoConfig.GetLocoId();
			const string& locoIdString = to_string(locoId);
			locoArgument["loco"] = locoIdString;
			if (locoId == LocoNone)
			{
				locoArgument["control"] = to_string(locoConfig.GetControlId());
				locoArgument["matchkey"] = locoConfig.GetMatchKey();
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextImport, "locoedit_list_0", locoArgument)));
				row.AddChildTag(HtmlTag("td").AddContent("&nbsp;"));
				row.AddChildTag(HtmlTag("td").AddContent("&nbsp;"));
			}
			else
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "locoedit_list_" + locoIdString, locoArgument)));
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "locoaskdelete_" + locoIdString, locoArgument)));
				if (loco.second.IsInUse())
				{
					row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonCommandWide(Languages::TextRelease, "locorelease_" + locoIdString, locoArgument, "hideElement('b_locorelease_" + locoIdString + "');")));
				}
				else
				{
					row.AddChildTag(HtmlTag("td").AddContent("&nbsp;"));
				}
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "locoedit_0"));
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleMultipleUnitList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextMultipleUnits));
		HtmlTag table("table");
		const map<string,LocoConfig> multipleUnitList = manager.MultipleUnitConfigByName();
		map<string,string> multipleUnitArgument;
		for (auto& multipleUnit : multipleUnitList)
		{
			const LocoConfig& locoConfig = multipleUnit.second;
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(multipleUnit.first));
			const MultipleUnitID multipleUnitId = locoConfig.GetLocoId();
			const string& multipleUnitIdString = to_string(multipleUnitId);
			multipleUnitArgument["multipleunit"] = multipleUnitIdString;
			if (multipleUnitId == LocoNone)
			{
				multipleUnitArgument["control"] = to_string(locoConfig.GetControlId());
				multipleUnitArgument["matchkey"] = locoConfig.GetMatchKey();
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextImport, "multipleunitedit_list_" + multipleUnitIdString, multipleUnitArgument)));
				row.AddChildTag(HtmlTag("td").AddContent("&nbsp;"));
				row.AddChildTag(HtmlTag("td").AddContent("&nbsp;"));
			}
			else
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "multipleunitedit_list_" + multipleUnitIdString, multipleUnitArgument)));
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "multipleunitaskdelete_" + multipleUnitIdString, multipleUnitArgument)));
				if (multipleUnit.second.IsInUse())
				{
					row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonCommandWide(Languages::TextRelease, "multipleunitrelease_" + multipleUnitIdString, multipleUnitArgument, "hideElement('b_locorelease_" + multipleUnitIdString + "');")));
				}
				else
				{
					row.AddChildTag(HtmlTag("td").AddContent("&nbsp;"));
				}
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "multipleunitedit_0"));
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleLocoAskDelete(const map<string, string>& arguments)
	{
		LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);

		if (locoID == LocoNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextLocoDoesNotExist);
			return;
		}

		const DataModel::Loco* loco = manager.GetLoco(locoID);
		if (loco == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextLocoDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& locoName = loco->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteLoco));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, locoName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "locodelete"))
			.AddContent(HtmlTagInputHidden("loco", to_string(locoID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleMultipleUnitAskDelete(const map<string, string>& arguments)
	{
		MultipleUnitID multipleUnitID = Utils::Utils::GetIntegerMapEntry(arguments, "multipleunit", MultipleUnitNone);

		if (multipleUnitID == MultipleUnitNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextMultipleUnitDoesNotExist);
			return;
		}

		const DataModel::MultipleUnit* multipleUnit = manager.GetMultipleUnit(multipleUnitID);
		if (multipleUnit == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextMultipleUnitDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& multipleUnitName = multipleUnit->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteMultipleUnit));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, multipleUnitName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "multipleunitdelete"))
			.AddContent(HtmlTagInputHidden("multipleunit", to_string(multipleUnitID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleLocoDelete(const map<string, string>& arguments)
	{
		LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		const DataModel::Loco* loco = manager.GetLoco(locoID);
		if (loco == nullptr)
		{
			ReplyResponse(ResponseError, Languages::TextLocoDoesNotExist);
			return;
		}

		string name = loco->GetName();
		string result;
		if (!manager.LocoDelete(locoID, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextLocoDeleted, name);
	}

	void WebClient::HandleMultipleUnitDelete(const map<string, string>& arguments)
	{
		MultipleUnitID multipleUnitID = Utils::Utils::GetIntegerMapEntry(arguments, "multipleunit", MultipleUnitNone);
		const DataModel::MultipleUnit* multipleUnit = manager.GetMultipleUnit(multipleUnitID);
		if (multipleUnit == nullptr)
		{
			ReplyResponse(ResponseError, Languages::TextMultipleUnitDoesNotExist);
			return;
		}

		string name = multipleUnit->GetName();
		string result;
		if (!manager.MultipleUnitDelete(multipleUnitID, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextMultipleUnitDeleted, name);
	}

	HtmlTag WebClient::HtmlTagLayerSelector(const LayerID layerID) const
	{
		map<string,LayerID> options = manager.LayerListByNameWithFeedback();
		return HtmlTagSelect("layer", options, layerID).AddAttribute("onchange", "loadLayout();");
	}

	void WebClient::HandleLayout(const map<string, string>& arguments)
	{
		LayerID layer = static_cast<LayerID>(Utils::Utils::GetIntegerMapEntry(arguments, "layer", CHAR_MIN));
		HtmlTag content;

		if (layer < LayerUndeletable)
		{
			const map<FeedbackID,Feedback*>& feedbacks = manager.FeedbackList();
			for (auto& feedback : feedbacks)
			{
				if (feedback.second->GetControlID() != -layer)
				{
					continue;
				}

				content.AddChildTag(HtmlTagFeedback(feedback.second, true));
			}
			ReplyHtmlWithHeader(content);
			return;
		}

		const map<TextID,DataModel::Text*>& texts = manager.TextList();
		for (auto& text : texts)
		{
			if (text.second->IsVisibleOnLayer(layer) == false)
			{
				continue;
			}
			content.AddChildTag(HtmlTagText(text.second));
		}

		const map<AccessoryID,DataModel::Accessory*>& accessories = manager.AccessoryList();
		for (auto& accessory : accessories)
		{
			if (accessory.second->IsVisibleOnLayer(layer) == false)
			{
				continue;
			}
			content.AddChildTag(HtmlTagAccessory(accessory.second));
		}

		const map<SwitchID,DataModel::Switch*>& switches = manager.SwitchList();
		for (auto& mySwitch : switches)
		{
			if (mySwitch.second->IsVisibleOnLayer(layer) == false)
			{
				continue;
			}
			content.AddChildTag(HtmlTagSwitch(mySwitch.second));
		}

		const map<SwitchID,DataModel::Track*>& tracks = manager.TrackList();
		for (auto& track : tracks)
		{
			if (track.second->IsVisibleOnLayer(layer) == false)
			{
				continue;
			}
			content.AddChildTag(HtmlTagTrack(manager, track.second));
		}

		const map<RouteID,DataModel::Route*>& routes = manager.RouteList();
		for (auto& route : routes)
		{
			if (route.second->IsVisibleOnLayer(layer) == false)
			{
				continue;
			}
			content.AddChildTag(HtmlTagRoute(route.second));
		}

		const map<FeedbackID,Feedback*>& feedbacks = manager.FeedbackList();
		for (auto& feedback : feedbacks)
		{
			if (feedback.second->IsVisibleOnLayer(layer) == false)
			{
				continue;
			}
			content.AddChildTag(HtmlTagFeedback(feedback.second));
		}

		const map<SignalID,DataModel::Signal*>& signals = manager.SignalList();
		for (auto& signal : signals)
		{
			if (signal.second->IsVisibleOnLayer(layer) == false)
			{
				continue;
			}
			content.AddChildTag(HtmlTagSignal(manager, signal.second));
		}

		ReplyHtmlWithHeader(content);
	}

	HtmlTag WebClient::HtmlTagControlLoco(ControlID& controlId, const string& objectType, const ObjectID objectID) const
	{
		std::map<ControlID,string> controls = manager.ControlListNames(Hardware::CapabilityLoco);
		return WebClientStatic::HtmlTagControl(controls, controlId, objectType, objectID);
	}

	HtmlTag WebClient::HtmlTagControlMultipleUnit(ControlID& controlId, const string& objectType, const ObjectID objectID) const
	{
		std::map<ControlID,string> controls = manager.ControlListNames(Hardware::CapabilityMultipleUnit);
		controls[ControlNone] = Languages::GetText(Languages::TextIndependentOfControl);
		return WebClientStatic::HtmlTagControl(controls, controlId, objectType, objectID);
	}

	HtmlTag WebClient::HtmlTagControlAccessory(ControlID& controlID, const string& objectType, const ObjectID objectID) const
	{
		std::map<ControlID,string> controls = manager.ControlListNames(Hardware::CapabilityAccessory);
		return WebClientStatic::HtmlTagControl(controls, controlID, objectType, objectID);
	}

	HtmlTag WebClient::HtmlTagControlFeedback(ControlID& controlId, const string& objectType, const ObjectID objectID) const
	{
		std::map<ControlID,string> controls = manager.ControlListNames(Hardware::CapabilityFeedback);
		return WebClientStatic::HtmlTagControl(controls, controlId, objectType, objectID);
	}

	void WebClient::HandleAccessoryEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		AccessoryID accessoryID = Utils::Utils::GetIntegerMapEntry(arguments, "accessory", AccessoryNone);
		string name = Languages::GetText(Languages::TextNew);
		ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlNone);
		if (controlId == ControlNone)
		{
			controlId = manager.GetPossibleControlForAccessory();
		}
		string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		Protocol protocol = ProtocolNone;
		Address address = AddressDefault;
		Address serverAddress = AddressNone;
		DataModel::AccessoryType accessoryType = static_cast<DataModel::AccessoryType>(Utils::Utils::GetIntegerMapEntry(arguments, "accessorytype", AccessoryTypeDefault));
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", LayerUndeletable);
		LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		DataModel::AccessoryPulseDuration duration = manager.GetDefaultAccessoryDuration();
		bool inverted = false;
		if (accessoryID > AccessoryNone)
		{
			const DataModel::Accessory* accessory = manager.GetAccessory(accessoryID);
			if (accessory != nullptr)
			{
				name = accessory->GetName();
				controlId = accessory->GetControlID();
				matchKey = accessory->GetMatchKey();
				protocol = accessory->GetProtocol();
				address = accessory->GetAddress();
				serverAddress = accessory->GetServerAddress();
				accessoryType = accessory->GetAccessoryType();
				posx = accessory->GetPosX();
				posy = accessory->GetPosY();
				posz = accessory->GetPosZ();
				rotation = accessory->GetRotation();
				duration = accessory->GetAccessoryPulseDuration();
				inverted = accessory->GetInverted();
			}
		}
		else if (controlId > ControlNone)
		{
			// accessory from hardware database
			const DataModel::AccessoryConfig accessory = manager.GetAccessoryOfConfigByMatchKey(controlId, matchKey);
			if (accessory.GetControlId() == controlId && accessory.GetMatchKey() == matchKey)
			{
				protocol = accessory.GetProtocol();
				address = accessory.GetAddress();
				name = accessory.GetName();
			}
		}
		// else new accessory

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("main", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("position", Languages::TextPosition));
		content.AddChildTag(tabMenu);

		HtmlTag formContent;
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "accessorysave"));
		formContent.AddChildTag(HtmlTagInputHidden("accessory", to_string(accessoryID)));

		std::map<DataModel::AccessoryType, Languages::TextSelector> typeOptions;
		typeOptions[DataModel::AccessoryTypeDefault] = Languages::TextDefault;
		typeOptions[DataModel::AccessoryTypeStraight] = Languages::TextStraight;
		typeOptions[DataModel::AccessoryTypeTurn] = Languages::TextTurn;

		HtmlTag mainContent("div");
		mainContent.AddId("tab_main");
		mainContent.AddClass("tab_content");
		mainContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		mainContent.AddChildTag(HtmlTagControlAccessory(controlId, "accessory", accessoryID));
		mainContent.AddChildTag(HtmlTag("div").AddId("select_protocol").AddChildTag(HtmlTagMatchKeyProtocolAccessory(controlId, matchKey, protocol)));
		mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("address", Languages::TextAddress, address, 1, 2044));
		mainContent.AddChildTag(WebClientStatic::HtmlTagDuration(duration));
		mainContent.AddChildTag(HtmlTagInputCheckboxWithLabel("inverted", Languages::TextInverted, "true", inverted));
		if (manager.IsServerEnabled())
		{
			mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("serveraddress", Languages::TextServerAddress, serverAddress, 0, 2044));
		}
		mainContent.AddChildTag(HtmlTagSelectWithLabel("accessorytype", Languages::TextType, typeOptions, accessoryType));
		formContent.AddChildTag(mainContent);

		formContent.AddChildTag(HtmlTagTabPosition(posx, posy, posz, rotation));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(HtmlTag("form").AddId("editform").AddChildTag(formContent)));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleAccessoryGet(const map<string, string>& arguments)
	{
		AccessoryID accessoryID = Utils::Utils::GetIntegerMapEntry(arguments, "accessory");
		const DataModel::Accessory* accessory = manager.GetAccessory(accessoryID);
		if (accessory == nullptr)
		{
			ReplyHtmlWithHeader(HtmlTag());
			return;
		}
		ReplyHtmlWithHeader(HtmlTagAccessory(accessory));
	}

	void WebClient::HandleAccessorySave(const map<string, string>& arguments)
	{
		const AccessoryID accessoryID = Utils::Utils::GetIntegerMapEntry(arguments, "accessory", AccessoryNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		const string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		const Protocol protocol = static_cast<Protocol>(Utils::Utils::GetIntegerMapEntry(arguments, "protocol", ProtocolNone));
		const Address address = Utils::Utils::GetIntegerMapEntry(arguments, "address", AddressDefault);
		const Address serverAddress = Utils::Utils::GetIntegerMapEntry(arguments, "serveraddress", AddressNone);
		const DataModel::AccessoryType accessoryType = static_cast<DataModel::AccessoryType>(Utils::Utils::GetIntegerMapEntry(arguments, "accessorytype", AccessoryTypeDefault));
		const LayoutPosition posX = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posY = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posZ = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		const LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		const DataModel::AccessoryPulseDuration duration = Utils::Utils::GetIntegerMapEntry(arguments, "duration", manager.GetDefaultAccessoryDuration());
		const bool inverted = Utils::Utils::GetBoolMapEntry(arguments, "inverted");
		string result;
		if (!manager.AccessorySave(accessoryID,
			name,
			posX,
			posY,
			posZ,
			rotation,
			controlId,
			matchKey,
			protocol,
			address,
			serverAddress,
			accessoryType,
			duration,
			inverted,
			result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextAccessorySaved, name);
	}

	void WebClient::HandleAccessoryState(const map<string, string>& arguments)
	{
		AccessoryID accessoryID = Utils::Utils::GetIntegerMapEntry(arguments, "accessory", AccessoryNone);
		DataModel::AccessoryState accessoryState = (Utils::Utils::GetStringMapEntry(arguments, "state", "off").compare("off") == 0 ? DataModel::AccessoryStateOff : DataModel::AccessoryStateOn);

		manager.AccessoryState(ControlTypeWebServer, accessoryID, accessoryState, false);

		ReplyHtmlWithHeaderAndParagraph(accessoryState ? Languages::TextAccessoryStateIsGreen : Languages::TextAccessoryStateIsRed, manager.GetAccessoryName(accessoryID));
	}

	void WebClient::HandleAccessoryList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextAccessories));
		HtmlTag table("table");
		const map<string,DataModel::AccessoryConfig> accessoryList = manager.AccessoryConfigByName();
		map<string,string> accessoryArgument;
		for (auto& accessory : accessoryList)
		{
			const AccessoryConfig& accessoryConfig = accessory.second;
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(accessory.first));
			row.AddChildTag(HtmlTag("td").AddContent(ProtocolName(accessoryConfig.GetProtocol())));
			row.AddChildTag(HtmlTag("td").AddContent(to_string(accessoryConfig.GetAddress())));
			AccessoryID accessoryId = accessoryConfig.GetObjectIdentifier().GetObjectID();
			const string& accessoryIdString = to_string(accessoryId);
			accessoryArgument["accessory"] = accessoryIdString;
			if (accessoryId == AccessoryNone)
			{
				accessoryArgument["control"] = to_string(accessoryConfig.GetControlId());
				accessoryArgument["matchkey"] = accessoryConfig.GetMatchKey();
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextImport, "accessoryedit_list_" + accessoryIdString, accessoryArgument)));
			}
			else
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "accessoryedit_list_" + accessoryIdString, accessoryArgument)));
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "accessoryaskdelete_" + accessoryIdString, accessoryArgument)));
				if (accessoryConfig.IsInUse())
				{
					row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonCommandWide(Languages::TextRelease, "accessoryrelease_" + accessoryIdString, accessoryArgument, "hideElement('b_accessoryrelease_" + accessoryIdString + "');")));
				}
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "accessoryedit_0"));
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleAccessoryAskDelete(const map<string, string>& arguments)
	{
		AccessoryID accessoryID = Utils::Utils::GetIntegerMapEntry(arguments, "accessory", AccessoryNone);

		if (accessoryID == AccessoryNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextAccessoryDoesNotExist);
			return;
		}

		const DataModel::Accessory* accessory = manager.GetAccessory(accessoryID);
		if (accessory == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextAccessoryDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& accessoryName = accessory->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteAccessory));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, accessoryName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "accessorydelete"))
			.AddContent(HtmlTagInputHidden("accessory", to_string(accessoryID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleAccessoryDelete(const map<string, string>& arguments)
	{
		AccessoryID accessoryID = Utils::Utils::GetIntegerMapEntry(arguments, "accessory", AccessoryNone);
		const DataModel::Accessory* accessory = manager.GetAccessory(accessoryID);
		if (accessory == nullptr)
		{
			ReplyResponse(ResponseError, Languages::TextAccessoryDoesNotExist);
			return;
		}

		string name = accessory->GetName();
		string result;
		if (!manager.AccessoryDelete(accessoryID, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextAccessoryDeleted, name);
	}

	void WebClient::HandleAccessoryRelease(const map<string, string>& arguments)
	{
		AccessoryID accessoryID = Utils::Utils::GetIntegerMapEntry(arguments, "accessory");
		bool ret = manager.AccessoryRelease(accessoryID);
		ReplyHtmlWithHeaderAndParagraph(ret ? "Accessory released" : "Accessory not released");
	}

	void WebClient::HandleSwitchEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		SwitchID switchID = Utils::Utils::GetIntegerMapEntry(arguments, "switch", SwitchNone);
		ControlID controlId = manager.GetPossibleControlForAccessory();
		if (controlId == ControlNone)
		{
			controlId = manager.GetPossibleControlForAccessory();
		}
		string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		Protocol protocol = ProtocolNone;
		Address address = AddressDefault;
		Address serverAddress = AddressNone;
		string name = Languages::GetText(Languages::TextNew);
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", LayerUndeletable);
		LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		DataModel::AccessoryType type = DataModel::SwitchTypeLeft;
		DataModel::AccessoryPulseDuration duration = manager.GetDefaultAccessoryDuration();
		bool inverted = false;
		if (switchID > SwitchNone)
		{
			const DataModel::Switch* mySwitch = manager.GetSwitch(switchID);
			if (mySwitch != nullptr)
			{
				controlId = mySwitch->GetControlID();
				matchKey = mySwitch->GetMatchKey();
				protocol = mySwitch->GetProtocol();
				address = mySwitch->GetAddress();
				serverAddress = mySwitch->GetServerAddress();
				name = mySwitch->GetName();
				posx = mySwitch->GetPosX();
				posy = mySwitch->GetPosY();
				posz = mySwitch->GetPosZ();
				rotation = mySwitch->GetRotation();
				type = mySwitch->GetAccessoryType();
				duration = mySwitch->GetAccessoryPulseDuration();
				inverted = mySwitch->GetInverted();
			}
		}
		else if (controlId > ControlNone)
		{
			// switch from hardware database
			const DataModel::AccessoryConfig switchConfig = manager.GetAccessoryOfConfigByMatchKey(controlId, matchKey);
			if (switchConfig.GetControlId() == controlId && switchConfig.GetMatchKey() == matchKey)
			{
				protocol = switchConfig.GetProtocol();
				address = switchConfig.GetAddress();
				name = switchConfig.GetName();
			}
		}
		// else new switch

		std::map<DataModel::AccessoryType,Languages::TextSelector> typeOptions;
		typeOptions[DataModel::SwitchTypeLeft] = Languages::TextLeft;
		typeOptions[DataModel::SwitchTypeRight] = Languages::TextRight;
		typeOptions[DataModel::SwitchTypeThreeWay] = Languages::TextThreeWay;
		typeOptions[DataModel::SwitchTypeMaerklinLeft] = Languages::TextMaerklinLeft;
		typeOptions[DataModel::SwitchTypeMaerklinRight] = Languages::TextMaerklinRight;

		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("main", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("position", Languages::TextPosition));
		content.AddChildTag(tabMenu);

		HtmlTag formContent;
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "switchsave"));
		formContent.AddChildTag(HtmlTagInputHidden("switch", to_string(switchID)));

		HtmlTag mainContent("div");
		mainContent.AddId("tab_main");
		mainContent.AddClass("tab_content");
		mainContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		mainContent.AddChildTag(HtmlTagSelectWithLabel("type", Languages::TextType, typeOptions, type));
		mainContent.AddChildTag(HtmlTagControlAccessory(controlId, "switch", switchID));
		mainContent.AddChildTag(HtmlTag("div").AddId("select_protocol").AddChildTag(HtmlTagMatchKeyProtocolAccessory(controlId, matchKey, protocol)));
		mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("address", Languages::TextAddress, address, 1, 2044));
		mainContent.AddChildTag(WebClientStatic::HtmlTagDuration(duration));
		mainContent.AddChildTag(HtmlTagInputCheckboxWithLabel("inverted", Languages::TextInverted, "true", inverted));
		if (manager.IsServerEnabled())
		{
			mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("serveraddress", Languages::TextServerAddress, serverAddress, 0, 2044));
		}
		formContent.AddChildTag(mainContent);

		formContent.AddChildTag(HtmlTagTabPosition(posx, posy, posz, rotation));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(HtmlTag("form").AddId("editform").AddChildTag(formContent)));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleSwitchSave(const map<string, string>& arguments)
	{
		const SwitchID switchID = Utils::Utils::GetIntegerMapEntry(arguments, "switch", SwitchNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		const string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		const Protocol protocol = static_cast<Protocol>(Utils::Utils::GetIntegerMapEntry(arguments, "protocol", ProtocolNone));
		const Address address = Utils::Utils::GetIntegerMapEntry(arguments, "address", AddressDefault);
		const Address serverAddress = Utils::Utils::GetIntegerMapEntry(arguments, "serveraddress", AddressNone);
		const LayoutPosition posX = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posY = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posZ = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		const LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		const DataModel::AccessoryType type = static_cast<DataModel::AccessoryType>(Utils::Utils::GetIntegerMapEntry(arguments, "type", DataModel::SwitchTypeLeft));
		const DataModel::AccessoryPulseDuration duration = Utils::Utils::GetIntegerMapEntry(arguments, "duration", manager.GetDefaultAccessoryDuration());
		const bool inverted = Utils::Utils::GetBoolMapEntry(arguments, "inverted");
		string result;
		if (!manager.SwitchSave(switchID,
			name,
			posX,
			posY,
			posZ,
			rotation,
			controlId,
			matchKey,
			protocol,
			address,
			serverAddress,
			type,
			duration,
			inverted,
			result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextSwitchSaved, name);
	}

	void WebClient::HandleSwitchState(const map<string, string>& arguments)
	{
		SwitchID switchID = Utils::Utils::GetIntegerMapEntry(arguments, "switch", SwitchNone);
		string switchStateText = Utils::Utils::GetStringMapEntry(arguments, "state", "turnout");
		DataModel::AccessoryState switchState;
		if (switchStateText.compare("turnout") == 0)
		{
			switchState = DataModel::SwitchStateTurnout;
		}
		else if (switchStateText.compare("third") == 0)
		{
			switchState = DataModel::SwitchStateThird;
		}
		else
		{
			switchState = DataModel::SwitchStateStraight;
		}
		manager.SwitchState(ControlTypeWebServer, switchID, switchState, false);

		ReplyHtmlWithHeaderAndParagraph(switchState ? Languages::TextSwitchStateIsStraight : Languages::TextSwitchStateIsTurnout, manager.GetSwitchName(switchID));
	}

	void WebClient::HandleSwitchList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextSwitches));
		HtmlTag table("table");
		const map<string,AccessoryConfig> switchList = manager.SwitchConfigByName();
		map<string,string> switchArgument;
		for (auto& mySwitch : switchList)
		{
			const AccessoryConfig& switchConfig = mySwitch.second;
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(mySwitch.first));
			row.AddChildTag(HtmlTag("td").AddContent(ProtocolName(switchConfig.GetProtocol())));
			row.AddChildTag(HtmlTag("td").AddContent(to_string(switchConfig.GetAddress())));
			SwitchID switchId = switchConfig.GetObjectIdentifier().GetObjectID();
			const string& switchIdString = to_string(switchId);
			switchArgument["switch"] = switchIdString;
			if (switchId == SwitchNone)
			{
				switchArgument["control"] = to_string(switchConfig.GetControlId());
				switchArgument["matchkey"] = switchConfig.GetMatchKey();
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextImport, "switchedit_list_" + switchIdString, switchArgument)));
			}
			else
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "switchedit_list_" + switchIdString, switchArgument)));
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "switchaskdelete_" + switchIdString, switchArgument)));
				if (switchConfig.IsInUse())
				{
					row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonCommandWide(Languages::TextRelease, "switchrelease_" + switchIdString, switchArgument, "hideElement('b_switchrelease_" + switchIdString + "');")));
				}
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "switchedit_0"));
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleSwitchAskDelete(const map<string, string>& arguments)
	{
		SwitchID switchID = Utils::Utils::GetIntegerMapEntry(arguments, "switch", SwitchNone);

		if (switchID == SwitchNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextSwitchDoesNotExist);
			return;
		}

		const DataModel::Switch* mySwitch = manager.GetSwitch(switchID);
		if (mySwitch == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextSwitchDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& switchName = mySwitch->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteSwitch));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, switchName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "switchdelete"))
			.AddContent(HtmlTagInputHidden("switch", to_string(switchID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleSwitchDelete(const map<string, string>& arguments)
	{
		SwitchID switchID = Utils::Utils::GetIntegerMapEntry(arguments, "switch", SwitchNone);
		const DataModel::Switch* mySwitch = manager.GetSwitch(switchID);
		if (mySwitch == nullptr)
		{
			ReplyResponse(ResponseError, Languages::TextSwitchDoesNotExist);
			return;
		}

		string name = mySwitch->GetName();
		string result;
		if (!manager.SwitchDelete(switchID, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextSwitchDeleted, name);
	}

	void WebClient::HandleSwitchGet(const map<string, string>& arguments)
	{
		SwitchID switchID = Utils::Utils::GetIntegerMapEntry(arguments, "switch");
		const DataModel::Switch* mySwitch = manager.GetSwitch(switchID);
		if (mySwitch == nullptr)
		{
			ReplyHtmlWithHeader(HtmlTag());
			return;
		}
		ReplyHtmlWithHeader(HtmlTagSwitch(mySwitch));
	}

	void WebClient::HandleSwitchRelease(const map<string, string>& arguments)
	{
		SwitchID switchID = Utils::Utils::GetIntegerMapEntry(arguments, "switch");
		bool ret = manager.SwitchRelease(switchID);
		ReplyHtmlWithHeaderAndParagraph(ret ? "Switch released" : "Switch not released");
	}

	HtmlTag WebClient::HtmlTagTabPosition(const LayoutPosition posx,
		const LayoutPosition posy,
		const LayoutPosition posz,
		const LayoutRotation rotation,
		const Visible visible) const
	{
		HtmlTag positionContent("div");
		positionContent.AddId("tab_position");
		positionContent.AddClass("tab_content");
		positionContent.AddClass("hidden");
		if (visible == DataModel::LayoutItem::VisibleNotRelevant)
		{
			positionContent.AddChildTag(HtmlTagPosition(posx, posy, posz));
		}
		else
		{
			positionContent.AddChildTag(HtmlTagPosition(posx, posy, posz, visible));
		}
		if (rotation != DataModel::LayoutItem::RotationNotRelevant)
		{
			positionContent.AddChildTag(WebClientStatic::HtmlTagRotation(rotation));
		}
		return positionContent;
	}

	HtmlTag WebClient::HtmlTagRelationSignalState(const string& name,
		const SignalID signalId,
		const DataModel::Relation::Data data)
	{
		map<DataModel::AccessoryState,DataModel::Signal::StateOption> stateOptions;
		Signal* signal = manager.GetSignal(signalId);
		if (signal != nullptr)
		{
			stateOptions = signal->GetStateOptions();
		}

		return HtmlTagSelect(name + "_state", stateOptions, static_cast<DataModel::AccessoryState>(data)).AddClass("select_relation_state");
	}

	void WebClient::HandleFeedbackEdit(const map<string, string>& arguments)
	{
		HtmlTag content;
		FeedbackID feedbackID = Utils::Utils::GetIntegerMapEntry(arguments, "feedback", FeedbackNone);
		string name = Languages::GetText(Languages::TextNew);
		ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "controlid", manager.GetPossibleControlForFeedback());
		string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		FeedbackPin pin = Utils::Utils::GetIntegerMapEntry(arguments, "pin", FeedbackPinNone);
		DataModel::FeedbackType feedbackType = static_cast<DataModel::FeedbackType>(Utils::Utils::GetIntegerMapEntry(arguments, "feedbacktype", FeedbackTypeDefault));
		RouteID routeId = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);
		LayoutPosition posx = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		LayoutPosition posy = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		LayoutPosition posz = Utils::Utils::GetIntegerMapEntry(arguments, "posz", LayerUndeletable);
		LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		DataModel::LayoutItem::Visible visible = static_cast<Visible>(Utils::Utils::GetBoolMapEntry(arguments, "visible", feedbackID == FeedbackNone && ((posx || posy) && posz >= LayerUndeletable) ? DataModel::LayoutItem::VisibleYes : DataModel::LayoutItem::VisibleNo));
		if (posz < LayerUndeletable)
		{
			if (controlId == ControlNone)
			{
				controlId = -posz;
			}
			if (pin == 0)
			{
				pin = posy * 16 + posx + (posx > 8 ? 0 : 1);
			}
		}
		bool inverted = false;
		if (feedbackID > FeedbackNone)
		{
			// existing feedback
			const DataModel::Feedback* feedback = manager.GetFeedback(feedbackID);
			if (feedback != nullptr)
			{
				name = feedback->GetName();
				matchKey = feedback->GetMatchKey();
				controlId = feedback->GetControlID();
				pin = feedback->GetPin();
				inverted = feedback->GetInverted();
				feedbackType = feedback->GetFeedbackType();
				routeId = feedback->GetRouteId();
				visible = feedback->GetVisible();
				posx = feedback->GetPosX();
				posy = feedback->GetPosY();
				posz = feedback->GetPosZ();
				rotation = feedback->GetRotation();
			}
		}
		else if (controlId > ControlNone)
		{
			// feedback from hardware database
			const DataModel::FeedbackConfig feedback = manager.GetFeedbackOfConfigByMatchKey(controlId, matchKey);
			if (feedback.GetControlId() == controlId && feedback.GetMatchKey() == matchKey)
			{
				name = feedback.GetName();
				pin = feedback.GetPin();
			}
		}
		// else new feedback


		content.AddChildTag(HtmlTag("h1").AddContent(name).AddId("popup_title"));

		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("main", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("position", Languages::TextPosition));
		content.AddChildTag(tabMenu);

		HtmlTag formContent("form");
		formContent.AddId("editform");
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "feedbacksave"));
		formContent.AddChildTag(HtmlTagInputHidden("feedback", to_string(feedbackID)));

		std::map<DataModel::FeedbackType, Languages::TextSelector> typeOptions;
		typeOptions[DataModel::FeedbackTypeDefault] = Languages::TextDefault;
		typeOptions[DataModel::FeedbackTypeStraight] = Languages::TextStraight;
		typeOptions[DataModel::FeedbackTypeTurn] = Languages::TextTurn;

		const std::map<string, Route*> routes = manager.RouteListByName();
		map<string, RouteID> routeOptions;
		routeOptions["-"] = RouteNone;
		for (auto& track : routes)
		{
			routeOptions[track.first] = track.second->GetID();
		}

		HtmlTag mainContent("div");
		mainContent.AddId("tab_main");
		mainContent.AddClass("tab_content");
		mainContent.AddChildTag(HtmlTagInputTextWithLabel("name", Languages::TextName, name).AddAttribute("onkeyup", "updateName();"));
		mainContent.AddChildTag(HtmlTagControlFeedback(controlId, "feedback", feedbackID));
		mainContent.AddChildTag(HtmlTagMatchKeyFeedback(controlId, matchKey));
		mainContent.AddChildTag(HtmlTagInputIntegerWithLabel("pin", Languages::TextPin, pin, 1, 4096));
		mainContent.AddChildTag(HtmlTagInputCheckboxWithLabel("inverted", Languages::TextInverted, "true", inverted));
		mainContent.AddChildTag(HtmlTagSelectWithLabel("feedbacktype", Languages::TextType, typeOptions, feedbackType));
		mainContent.AddChildTag(HtmlTagSelectWithLabel("route", Languages::TextExecuteRoute, routeOptions, routeId));
		formContent.AddChildTag(mainContent);

		formContent.AddChildTag(HtmlTagTabPosition(posx, posy, posz, rotation, visible));

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(formContent));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleFeedbackSave(const map<string, string>& arguments)
	{
		const FeedbackID feedbackID = Utils::Utils::GetIntegerMapEntry(arguments, "feedback", FeedbackNone);
		const string name = Utils::Utils::GetStringMapEntry(arguments, "name");
		const ControlID controlId = Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlIdNone);
		const string matchKey = Utils::Utils::GetStringMapEntry(arguments, "matchkey");
		const FeedbackPin pin = static_cast<FeedbackPin>(Utils::Utils::GetIntegerMapEntry(arguments, "pin", FeedbackPinNone));
		const bool inverted = Utils::Utils::GetBoolMapEntry(arguments, "inverted");
		const DataModel::FeedbackType feedbackType = static_cast<DataModel::FeedbackType>(Utils::Utils::GetIntegerMapEntry(arguments, "feedbacktype", FeedbackTypeDefault));
		const RouteID routeId = Utils::Utils::GetIntegerMapEntry(arguments, "route", RouteNone);
		const DataModel::LayoutItem::Visible visible = static_cast<Visible>(Utils::Utils::GetBoolMapEntry(arguments, "visible", DataModel::LayoutItem::VisibleNo));
		const LayoutPosition posX = Utils::Utils::GetIntegerMapEntry(arguments, "posx", 0);
		const LayoutPosition posY = Utils::Utils::GetIntegerMapEntry(arguments, "posy", 0);
		const LayoutPosition posZ = Utils::Utils::GetIntegerMapEntry(arguments, "posz", 0);
		const LayoutRotation rotation = Utils::Utils::GetIntegerMapEntry(arguments, "rotation", DataModel::LayoutItem::Rotation0);
		string result;
		if (!manager.FeedbackSave(feedbackID,
			name,
			visible,
			posX,
			posY,
			posZ,
			rotation,
			controlId,
			matchKey,
			pin,
			inverted,
			feedbackType,
			routeId,
			result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextFeedbackSaved, name);
	}

	void WebClient::HandleFeedbackState(const map<string, string>& arguments)
	{
		FeedbackID feedbackID = Utils::Utils::GetIntegerMapEntry(arguments, "feedback", FeedbackNone);
		DataModel::Feedback::FeedbackState state = (Utils::Utils::GetStringMapEntry(arguments, "state", "occupied").compare("occupied") == 0 ? DataModel::Feedback::FeedbackStateOccupied : DataModel::Feedback::FeedbackStateFree);

		manager.FeedbackState(feedbackID, state);

		ReplyHtmlWithHeaderAndParagraph(state ? Languages::TextFeedbackStateIsOn : Languages::TextFeedbackStateIsOff, manager.GetFeedbackName(feedbackID));
	}

	void WebClient::HandleFeedbackList()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextFeedbacks));
		HtmlTag table("table");
		const map<string,DataModel::FeedbackConfig> feedbackList = manager.FeedbackConfigByName();
		map<string,string> feedbackArgument;
		for (auto& feedback : feedbackList)
		{
			const DataModel::FeedbackConfig& feedbackConfig = feedback.second;
			HtmlTag row("tr");
			row.AddChildTag(HtmlTag("td").AddContent(feedback.first));
			const FeedbackID feedbackId = feedbackConfig.GetFeedbackId();
			const string& feedbackIdString = to_string(feedbackId);
			feedbackArgument["feedback"] = feedbackIdString;
			if (feedbackId == FeedbackNone)
			{
				feedbackArgument["control"] = to_string(feedbackConfig.GetControlId());
				feedbackArgument["matchkey"] = feedbackConfig.GetMatchKey();
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextImport, "feedbackedit_list_" + feedbackIdString, feedbackArgument)));
			}
			else
			{
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextEdit, "feedbackedit_list_" + feedbackIdString, feedbackArgument)));
				row.AddChildTag(HtmlTag("td").AddChildTag(HtmlTagButtonPopupWide(Languages::TextDelete, "feedbackaskdelete_" + feedbackIdString, feedbackArgument)));
			}
			table.AddChildTag(row);
		}
		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(table));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonPopupWide(Languages::TextNew, "feedbackedit_0"));
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleFeedbackAskDelete(const map<string, string>& arguments)
	{
		FeedbackID feedbackID = Utils::Utils::GetIntegerMapEntry(arguments, "feedback", FeedbackNone);

		if (feedbackID == FeedbackNone)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextFeedbackDoesNotExist);
			return;
		}

		const DataModel::Feedback* feedback = manager.GetFeedback(feedbackID);
		if (feedback == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextFeedbackDoesNotExist);
			return;
		}

		HtmlTag content;
		const string& feedbackName = feedback->GetName();
		content.AddContent(HtmlTag("h1").AddContent(Languages::TextDeleteFeedback));
		content.AddContent(HtmlTag("p").AddContent(Languages::TextAreYouSureToDelete, feedbackName));
		content.AddContent(HtmlTag("form").AddId("editform")
			.AddContent(HtmlTagInputHidden("cmd", "feedbackdelete"))
			.AddContent(HtmlTagInputHidden("feedback", to_string(feedbackID))
			));
		content.AddContent(HtmlTagButtonCancel());
		content.AddContent(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleFeedbackDelete(const map<string, string>& arguments)
	{
		FeedbackID feedbackID = Utils::Utils::GetIntegerMapEntry(arguments, "feedback", FeedbackNone);
		const DataModel::Feedback* feedback = manager.GetFeedback(feedbackID);
		if (feedback == nullptr)
		{
			ReplyResponse(ResponseError, Languages::TextFeedbackDoesNotExist);
			return;
		}

		string name = feedback->GetName();
		string result;
		if (!manager.FeedbackDelete(feedbackID, result))
		{
			ReplyResponse(ResponseError, result);
			return;
		}

		ReplyResponse(ResponseInfo, Languages::TextFeedbackDeleted, name);
	}

	void WebClient::HandleFeedbackGet(const map<string, string>& arguments)
	{
		FeedbackID feedbackID = Utils::Utils::GetIntegerMapEntry(arguments, "feedback", FeedbackNone);
		const DataModel::Feedback* feedback = manager.GetFeedback(feedbackID);
		if (feedback == nullptr)
		{
			ReplyHtmlWithHeader(HtmlTag());
			return;
		}

		LayerID layer = Utils::Utils::GetIntegerMapEntry(arguments, "layer", LayerNone);
		if (feedback->GetControlID() == -layer)
		{
			ReplyHtmlWithHeader(HtmlTagFeedback(feedback, true));
			return;
		}

		if (layer < LayerNone || feedback->GetVisible() == DataModel::LayoutItem::VisibleNo)
		{
			ReplyHtmlWithHeader(HtmlTag());
			return;
		}

		ReplyHtmlWithHeader(HtmlTagFeedback(feedback));
	}

	void WebClient::HandleLocoSelector(const map<string,string>& arguments)
	{
		const unsigned int selector = Utils::Utils::GetIntegerMapEntry(arguments, "selector", 1);
		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco");
		ReplyHtmlWithHeader(HtmlTagLocoSelector(to_string(selector), locoID));
	}

	void WebClient::HandleLayerSelector(const map<string,string>& arguments)
	{
		const LayerID layerID = Utils::Utils::GetIntegerMapEntry(arguments, "layer");
		ReplyHtmlWithHeader(HtmlTagLayerSelector(layerID));
	}

	void WebClient::HandleSettingsEdit()
	{
		const StartupInitLocos startupInitLocos = manager.GetStartupInitLocos();
		const DataModel::AccessoryPulseDuration defaultAccessoryDuration = manager.GetDefaultAccessoryDuration();
		const bool executeAccessory = manager.GetExecuteAccessory();
		const bool autoAddFeedback = manager.GetAutoAddFeedback();
		const bool stopOnFeedbackInFreeTrack = manager.GetStopOnFeedbackInFreeTrack();
		const DataModel::SelectRouteApproach selectRouteApproach = manager.GetSelectRouteApproach();
		const DataModel::LocoBase::NrOfTracksToReserve nrOfTracksToReserve = manager.GetNrOfTracksToReserve();

		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextSettings).AddId("popup_title"));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("basic", Languages::TextBasic, true));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("startup", Languages::TextStartup));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("feedback", Languages::TextFeedback));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("accessory", Languages::TextAccessory));
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("automode", Languages::TextAutomode));
		content.AddChildTag(tabMenu);

		HtmlTag formContent("form");
		formContent.AddId("editform");
		formContent.AddChildTag(HtmlTagInputHidden("cmd", "settingssave"));

		HtmlTag basicContent("div");
		basicContent.AddId("tab_basic");
		basicContent.AddClass("tab_content");
		basicContent.AddChildTag(WebClientStatic::HtmlTagLanguage());
		basicContent.AddChildTag(WebClientStatic::HtmlTagLogLevel());
		formContent.AddChildTag(basicContent);

		HtmlTag startupContent("div");
		startupContent.AddId("tab_startup");
		startupContent.AddClass("tab_content");
		startupContent.AddClass("hidden");
		startupContent.AddChildTag(WebClientStatic::HtmlTagStartupLocos(startupInitLocos));
		formContent.AddChildTag(startupContent);

		HtmlTag accessoryContent("div");
		accessoryContent.AddId("tab_accessory");
		accessoryContent.AddClass("tab_content");
		accessoryContent.AddClass("hidden");
		accessoryContent.AddChildTag(WebClientStatic::HtmlTagDuration(defaultAccessoryDuration, Languages::TextDefaultSwitchingDuration));
		accessoryContent.AddChildTag(WebClientStatic::HtmlTagSelectExecuteAccessory(executeAccessory));
		formContent.AddChildTag(accessoryContent);

		HtmlTag feedbackContent("div");
		feedbackContent.AddId("tab_feedback");
		feedbackContent.AddClass("tab_content");
		feedbackContent.AddClass("hidden");
		feedbackContent.AddChildTag(HtmlTagInputCheckboxWithLabel("autoaddfeedback", Languages::TextAutomaticallyAddUnknownFeedbacks, "autoaddfeedback", autoAddFeedback));
		feedbackContent.AddChildTag(HtmlTagInputCheckboxWithLabel("stoponfeedbackinfreetrack", Languages::TextStopOnFeedbackInFreeTrack, "stoponfeedbackinfreetrack", stopOnFeedbackInFreeTrack));
		formContent.AddChildTag(feedbackContent);

		HtmlTag automodeContent("div");
		automodeContent.AddId("tab_automode");
		automodeContent.AddClass("tab_content");
		automodeContent.AddClass("hidden");
		automodeContent.AddChildTag(WebClientStatic::HtmlTagSelectSelectRouteApproach(selectRouteApproach, false));
		automodeContent.AddChildTag(WebClientStatic::HtmlTagNrOfTracksToReserve(nrOfTracksToReserve));
		formContent.AddChildTag(automodeContent);

		content.AddChildTag(HtmlTag("div").AddClass("popup_content").AddChildTag(formContent));
		content.AddChildTag(HtmlTagButtonCancel());
		content.AddChildTag(HtmlTagButtonOK());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleSettingsSave(const map<string, string>& arguments)
	{
		const Languages::Language language = static_cast<Languages::Language>(Utils::Utils::GetIntegerMapEntry(arguments, "language", Languages::EN));
		const Logger::Logger::Level logLevel = static_cast<Logger::Logger::Level>(Utils::Utils::GetIntegerMapEntry(arguments, "loglevel", Logger::Logger::LevelInfo));
		const StartupInitLocos startupInitLocos = static_cast<StartupInitLocos>(Utils::Utils::GetIntegerMapEntry(arguments, "startupinitlocos", StartupInitLocosAll));
		const DataModel::AccessoryPulseDuration defaultAccessoryDuration = Utils::Utils::GetIntegerMapEntry(arguments, "duration", manager.GetDefaultAccessoryDuration());
		const bool executeAccessory = Utils::Utils::GetBoolMapEntry(arguments, "executeaccessory", manager.GetExecuteAccessory());
		const bool autoAddFeedback = Utils::Utils::GetBoolMapEntry(arguments, "autoaddfeedback", manager.GetAutoAddFeedback());
		const bool stopOnFeedbackInFreeTrack = Utils::Utils::GetBoolMapEntry(arguments, "stoponfeedbackinfreetrack", manager.GetStopOnFeedbackInFreeTrack());
		const DataModel::SelectRouteApproach selectRouteApproach = static_cast<DataModel::SelectRouteApproach>(Utils::Utils::GetIntegerMapEntry(arguments, "selectrouteapproach", DataModel::SelectRouteRandom));
		const DataModel::Loco::NrOfTracksToReserve nrOfTracksToReserve = static_cast<DataModel::Loco::NrOfTracksToReserve>(Utils::Utils::GetIntegerMapEntry(arguments, "nroftrackstoreserve", DataModel::Loco::ReserveOne));
		manager.SettingsSave(language,
			startupInitLocos,
			defaultAccessoryDuration,
			autoAddFeedback,
			stopOnFeedbackInFreeTrack,
			executeAccessory,
			selectRouteApproach,
			nrOfTracksToReserve,
			logLevel);
		ReplyResponse(ResponseInfo, Languages::TextSettingsSaved);
	}

	void WebClient::HandleTimestamp(__attribute__((unused)) const map<string, string>& arguments)
	{
#ifdef __CYGWIN__
		ReplyHtmlWithHeaderAndParagraph(Languages::TextTimestampNotSet);
#else
		const time_t timestamp = Utils::Utils::GetIntegerMapEntry(arguments, "timestamp", 0);
		if (timestamp == 0)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextTimestampNotSet);
			return;
		}
		struct timeval tv;
		int ret = gettimeofday(&tv, nullptr);
		if (ret != 0 || tv.tv_sec > GetVersionInfoCompileTimestamp())
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextTimestampAlreadySet);
			return;
		}

		tv.tv_sec = timestamp;
		ret = settimeofday(&tv, nullptr);
		if (ret != 0)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextTimestampNotSet);
			return;
		}
		ReplyHtmlWithHeaderAndParagraph(Languages::TextTimestampSet);
#endif
	}

	void WebClient::HandleControlArguments(const map<string, string>& arguments)
	{
		HardwareType hardwareType = static_cast<HardwareType>(Utils::Utils::GetIntegerMapEntry(arguments, "hardwaretype"));
		ReplyHtmlWithHeader(WebClientStatic::HtmlTagControlArguments(hardwareType));
	}

	HtmlTag WebClient::HtmlTagProgramModeSelector(const ControlID controlID, ProgramMode& mode) const
	{
		Hardware::Capabilities capabilities = manager.GetCapabilities(controlID);
		map<ProgramMode, Languages::TextSelector> programModeOptions;
		if (capabilities & (Hardware::CapabilityProgramDccDirectRead | Hardware::CapabilityProgramDccDirectWrite))
		{
			programModeOptions[ProgramModeDccDirect] = Languages::TextProgramModeDccDirect;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeDccDirect;
			}
		}
		if (capabilities & (Hardware::CapabilityProgramDccPomLocoRead | Hardware::CapabilityProgramDccPomLocoWrite))
		{
			programModeOptions[ProgramModeDccPomLoco] = Languages::TextProgramModeDccPomLoco;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeDccPomLoco;
			}
		}
		if (capabilities & (Hardware::CapabilityProgramDccPomAccessoryRead | Hardware::CapabilityProgramDccPomAccessoryWrite))
		{
			programModeOptions[ProgramModeDccPomAccessory] = Languages::TextProgramModeDccPomAccessory;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeDccPomAccessory;
			}
		}
		if (capabilities & Hardware::CapabilityProgramMmWrite)
		{
			programModeOptions[ProgramModeMm] = Languages::TextProgramModeMm;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeMm;
			}
		}
		if (capabilities & Hardware::CapabilityProgramMmPomWrite)
		{
			programModeOptions[ProgramModeMmPom] = Languages::TextProgramModeMmPom;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeMmPom;
			}
		}
		if (capabilities & (Hardware::CapabilityProgramMfxRead | Hardware::CapabilityProgramMfxWrite))
		{
			programModeOptions[ProgramModeMfx] = Languages::TextProgramModeMfx;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeMfx;
			}
		}
		if (capabilities & (Hardware::CapabilityProgramDccPageRead | Hardware::CapabilityProgramDccPageWrite))
		{
			programModeOptions[ProgramModeDccPage] = Languages::TextProgramModeDccPage;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeDccPage;
			}
		}
		if (capabilities & (Hardware::CapabilityProgramDccRegisterRead | Hardware::CapabilityProgramDccRegisterWrite))
		{
			programModeOptions[ProgramModeDccRegister] = Languages::TextProgramModeDccRegister;
			if (mode == ProgramModeNone)
			{
				mode = ProgramModeDccRegister;
			}
		}
		return HtmlTagSelectWithLabel("moderaw", Languages::TextProgramMode, programModeOptions, mode).AddAttribute("onchange", "onChangeProgramModeSelector();");
	}

	HtmlTag WebClient::HtmlTagCvFields(const ControlID controlID, const ProgramMode programMode) const
	{
		HtmlTag content("div");
		content.AddId("cv_fields");
		switch (programMode)
		{
			case ProgramModeMmPom:
				content.AddChildTag(HtmlTagInputIntegerWithLabel("addressraw", Languages::TextAddress, 1, 1, 0xFF));
				break;

			case ProgramModeMfx:
			case ProgramModeDccPomLoco:
			case ProgramModeDccPomAccessory:
				content.AddChildTag(HtmlTagInputIntegerWithLabel("addressraw", Languages::TextAddress, 1, 1, 0x4000));
				break;

			default:
				content.AddChildTag(HtmlTagInputHidden("addressraw", "0"));
				break;
		}

		switch (programMode)
		{
			case ProgramModeMfx:
				content.AddChildTag(HtmlTagInputIntegerWithLabel("indexraw", Languages::TextIndex, 0, 0, 0x3F));
				break;

			default:
				content.AddChildTag(HtmlTagInputHidden("indexraw", "0"));
				break;
		}

		switch (programMode)
		{
			case ProgramModeMm:
			case ProgramModeMmPom:
				content.AddChildTag(HtmlTagInputIntegerWithLabel("cvraw", Languages::TextCV, 1, 1, 256));
				break;

			default:
				content.AddChildTag(HtmlTagInputIntegerWithLabel("cvraw", Languages::TextCV, 1, 1, 1024));
				break;
		}

		content.AddChildTag(HtmlTagInputIntegerWithLabel("valueraw", Languages::TextValue, 0, 0, 255));

		content.AddChildTag(HtmlTag("br"));
		content.AddChildTag(HtmlTagInput8BitValueWithLabel());
		content.AddChildTag(HtmlTag("br"));

		Hardware::Capabilities capabilities = manager.GetCapabilities(controlID);
		if (((programMode == ProgramModeMm) && (capabilities & Hardware::CapabilityProgramMmWrite))
			|| ((programMode == ProgramModeMmPom) && (capabilities & Hardware::CapabilityProgramMmPomWrite))
			|| ((programMode == ProgramModeMfx) && (capabilities & Hardware::CapabilityProgramMfxWrite))
			|| ((programMode == ProgramModeDccRegister) && (capabilities & Hardware::CapabilityProgramDccRegisterWrite))
			|| ((programMode == ProgramModeDccPage) && (capabilities & Hardware::CapabilityProgramDccPageWrite))
			|| ((programMode == ProgramModeDccDirect) && (capabilities & Hardware::CapabilityProgramDccDirectWrite))
			|| ((programMode == ProgramModeDccPomLoco) && (capabilities & Hardware::CapabilityProgramDccPomLocoWrite))
			|| ((programMode == ProgramModeDccPomAccessory) && (capabilities & Hardware::CapabilityProgramDccPomAccessoryWrite)))
		{
			HtmlTagButton writeButton(Languages::TextWrite, "programwrite");
			writeButton.AddAttribute("onclick", "onClickProgramWrite();return false;");
			writeButton.AddClass("wide_button");
			content.AddChildTag(writeButton);
		}

		if (((programMode == ProgramModeMfx) && (capabilities & Hardware::CapabilityProgramMfxRead))
			|| ((programMode == ProgramModeDccRegister) && (capabilities & Hardware::CapabilityProgramDccRegisterRead))
			|| ((programMode == ProgramModeDccPage) && (capabilities & Hardware::CapabilityProgramDccPageRead))
			|| ((programMode == ProgramModeDccDirect) && (capabilities & Hardware::CapabilityProgramDccDirectRead))
			|| ((programMode == ProgramModeDccPomLoco) && (capabilities & Hardware::CapabilityProgramDccPomLocoRead))
			|| ((programMode == ProgramModeDccPomAccessory) && (capabilities & Hardware::CapabilityProgramDccPomAccessoryRead)))
		{
			HtmlTagButton readButton(Languages::TextRead, "programread");
			readButton.AddAttribute("onclick", "onClickProgramRead();return false;");
			readButton.AddClass("wide_button");
			content.AddChildTag(readButton);
		}

		return content;
	}

	void WebClient::HandleCvFields(const map<string, string>& arguments)
	{
		ControlID controlID = static_cast<ControlID>(Utils::Utils::GetIntegerMapEntry(arguments, "control", ControlNone));
		ProgramMode programMode = static_cast<ProgramMode>(Utils::Utils::GetIntegerMapEntry(arguments, "mode", ProgramModeNone));
		ReplyHtmlWithHeader(HtmlTagCvFields(controlID, programMode));
	}

	HtmlTag WebClient::HtmlTagInput8BitValueWithLabel() const
	{
		HtmlTag content;
		content.AddChildTag(HtmlTagLabel(Languages::TextValue, "valueraw0"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw7"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw6"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw5"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw4"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw3"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw2"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw1"));
		content.AddChildTag(HtmlTagInputBitValue("valueraw0"));
		return content;
	}

	HtmlTag WebClient::HtmlTagInputBitValue(string name) const
	{
		HtmlTagInputCheckbox checkbox(name, name);
		checkbox.AddAttribute("onclick", "updateCvValue();");
		return checkbox;
	}

	void WebClient::HandleProgram()
	{
		HtmlTag content;
		content.AddChildTag(HtmlTag("h1").AddContent(Languages::TextProgrammer));
		HtmlTag tabMenu("div");
		tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("raw", Languages::TextDirect, true));
		std::map<ControlID,string> controls = manager.ControlListNames(Hardware::CapabilityProgram);
		if (controls.size() == 0)
		{
			ReplyHtmlWithHeader(HtmlTag("p").AddContent(Languages::TextNoControlSupportsProgramming));
			return;
		}
		/* FIXME: Tab MM and Tab DCC incomplete
		unsigned int controlCountMm = 0;
		unsigned int controlCountDcc = 0;
		for (auto control : controls)
		{
			Hardware::Capabilities capabilities = manager.GetCapabilities(control.first);
			if (capabilities & Hardware::CapabilityProgramMmWrite)
			{
				++controlCountMm;
			}
			if (capabilities & (Hardware::CapabilityProgramDccDirectRead | Hardware::CapabilityProgramDccDirectWrite))
			{
				++controlCountDcc;
			}
		}

		if (controlCountMm > 0)
		{
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("mm", Languages::TextMaerklinMotorola));
		}
		if (controlCountDcc > 0)
		{
			tabMenu.AddChildTag(WebClientStatic::HtmlTagTabMenuItem("dcc", Languages::TextDcc));
		}
		*/
		content.AddChildTag(tabMenu);

		HtmlTag programContent("div");
		programContent.AddClass("popup_content");

		HtmlTag rawContent("div");
		rawContent.AddId("tab_raw");
		rawContent.AddClass("tab_content");
		rawContent.AddClass("narrow_label");
		HtmlTag controlSelector = WebClientStatic::HtmlTagControl("controlraw", controls);
		rawContent.AddChildTag(controlSelector);

		const ControlID controlIdFirst = controls.begin()->first;
		HtmlTag programModeSelector("div");
		programModeSelector.AddId("program_mode_selector");
		ProgramMode programMode = ProgramModeNone;
		programModeSelector.AddChildTag(HtmlTagProgramModeSelector(controlIdFirst, programMode));
		rawContent.AddChildTag(programModeSelector);
		rawContent.AddChildTag(HtmlTagCvFields(controlIdFirst, programMode));
		programContent.AddChildTag(rawContent);

		/* FIXME: Tab MM incomplete
		HtmlTag mmContent("div");
		mmContent.AddId("tab_mm");
		mmContent.AddClass("tab_content");
		mmContent.AddClass("hidden");
		mmContent.AddContent("MM");
		programContent.AddChildTag(mmContent);
		*/

		/* FIXME: Tab DCC incomplete
		HtmlTag dccContent("div");
		dccContent.AddId("tab_dcc");
		dccContent.AddClass("tab_content");
		dccContent.AddClass("hidden");
		dccContent.AddContent("DCC");
		programContent.AddChildTag(dccContent);
		*/

		content.AddChildTag(programContent);
		content.AddChildTag(HtmlTagButtonCancel());
		ReplyHtmlWithHeader(content);
	}

	void WebClient::HandleProgramModeSelector(const map<string, string>& arguments)
	{
		ControlID controlID = static_cast<ControlID>(Utils::Utils::GetIntegerMapEntry(arguments, "control"));
		ProgramMode mode = static_cast<ProgramMode>(Utils::Utils::GetIntegerMapEntry(arguments, "mode"));
		return ReplyHtmlWithHeader(HtmlTagProgramModeSelector(controlID, mode));
	}

	void WebClient::HandleProgramRead(const map<string, string>& arguments)
	{
		ControlID controlID = static_cast<ControlID>(Utils::Utils::GetIntegerMapEntry(arguments, "control"));
		CvNumber cv = static_cast<CvNumber>(Utils::Utils::GetIntegerMapEntry(arguments, "cv"));
		ProgramMode mode = static_cast<ProgramMode>(Utils::Utils::GetIntegerMapEntry(arguments, "mode"));
		switch (mode)
		{
			case ProgramModeDccRegister:
			case ProgramModeDccPage:
			case ProgramModeDccDirect:
				manager.ProgramRead(controlID, mode, 0, cv);
				break;

			case ProgramModeDccPomLoco:
			case ProgramModeDccPomAccessory:
			case ProgramModeMfx:
			{
				Address address = static_cast<Address>(Utils::Utils::GetIntegerMapEntry(arguments, "address"));
				manager.ProgramRead(controlID, mode, address, cv);
				break;
			}

			default:
				break;
		}
		ReplyHtmlWithHeaderAndParagraph(Languages::TextProgramDccDirectRead, cv);
	}

	void WebClient::HandleProgramWrite(const map<string, string>& arguments)
	{
		ControlID controlID = static_cast<ControlID>(Utils::Utils::GetIntegerMapEntry(arguments, "control"));
		ProgramMode mode = static_cast<ProgramMode>(Utils::Utils::GetIntegerMapEntry(arguments, "mode"));
		CvNumber cv = static_cast<CvNumber>(Utils::Utils::GetIntegerMapEntry(arguments, "cv"));
		CvValue value = static_cast<CvValue>(Utils::Utils::GetIntegerMapEntry(arguments, "value"));
		switch (mode)
		{
			case ProgramModeMm:
			case ProgramModeDccRegister:
			case ProgramModeDccPage:
			case ProgramModeDccDirect:
				manager.ProgramWrite(controlID, mode, 0, cv, value);
				break;

			case ProgramModeMmPom:
			case ProgramModeDccPomLoco:
			case ProgramModeDccPomAccessory:
			case ProgramModeMfx:
			{
				Address address = static_cast<Address>(Utils::Utils::GetIntegerMapEntry(arguments, "address"));
				manager.ProgramWrite(controlID, mode, address, cv, value);
				break;
			}

			default:
				break;
		}
		ReplyHtmlWithHeaderAndParagraph(Languages::TextProgramDccDirectWrite, cv, value);
	}

	void WebClient::HandleUpdater(const map<string, string>& headers)
	{
		Response response;
		response.AddHeader("Cache-Control", "no-cache, must-revalidate");
		response.AddHeader("Pragma", "no-cache");
		response.AddHeader("Expires", "Sun, 12 Feb 2016 00:00:00 GMT");
		response.AddHeader("Content-Type", "text/event-stream; charset=utf-8");
		int ret = connection->Send(response);
		if (ret <= 0)
		{
			return;
		}

		unsigned int updateID = Utils::Utils::GetIntegerMapEntry(headers, "Last-Event-ID", 1);
		while(run)
		{
			string s;
			bool ok = server.NextUpdate(updateID, s);
			if (ok == false)
			{
				// FIXME: use signaling instead of sleep
				Utils::Utils::SleepForMilliseconds(100);
				continue;
			}

			string reply("id: ");
			reply += to_string(updateID);
			reply += "\r\n";
			reply += s;
			reply += "\r\n\r\n";

			++updateID;

			ret = connection->Send(reply);
			if (ret < 0)
			{
				return;
			}
		}
	}

	HtmlTag WebClient::HtmlTagLocoSelector(const string& selector, const LocoID locoID) const
	{
		map<string,LocoID> options = manager.LocoBaseIdsByName();
		if (options.size() != 1)
		{
			options["-"] = LocoNone;
		}
		return HtmlTagSelect("loco_" + selector, options, locoID).AddAttribute("onchange", "loadLoco(" + selector + ");");
	}

	void WebClient::HandleLoco(const map<string, string>& arguments)
	{
		string content;
		const LocoID locoID = Utils::Utils::GetIntegerMapEntry(arguments, "loco", LocoNone);
		const ObjectIdentifier locoBaseIdentifier(WebClientStatic::LocoIdToObjectIdentifier(locoID));
		if (!locoBaseIdentifier.IsSet())
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextPleaseSelectLoco);
			return;
		}
		LocoBase* locoBase = manager.GetLocoBase(locoBaseIdentifier);
		if (locoBase == nullptr)
		{
			ReplyHtmlWithHeaderAndParagraph(Languages::TextLocoDoesNotExist);
			return;
		}
		HtmlTag container("div");
		container.AddAttribute("class", "inner_loco");
		container.AddChildTag(HtmlTag("p").AddId("loconame").AddContent(locoBase->GetName()));
		unsigned int speed = locoBase->GetSpeed();
		map<string, string> buttonArguments;
		buttonArguments["loco"] = to_string(locoID);

		string id = "locospeed_" + to_string(locoID);
		container.AddChildTag(HtmlTagInputSliderLocoSpeed(id, MinSpeed, locoBase->GetMaxSpeed(), speed, locoID));
		buttonArguments["speed"] = to_string(MinSpeed);
		container.AddChildTag(HtmlTagButtonCommand("0", id + "_0", buttonArguments));
		buttonArguments["speed"] = to_string(locoBase->GetCreepingSpeed());
		container.AddChildTag(HtmlTagButtonCommand("I", id + "_1", buttonArguments));
		buttonArguments["speed"] = to_string(locoBase->GetReducedSpeed());
		container.AddChildTag(HtmlTagButtonCommand("II", id + "_2", buttonArguments));
		buttonArguments["speed"] = to_string(locoBase->GetTravelSpeed());
		container.AddChildTag(HtmlTagButtonCommand("III", id + "_3", buttonArguments));
		buttonArguments["speed"] = to_string(locoBase->GetMaxSpeed());
		container.AddChildTag(HtmlTagButtonCommand("IV", id + "_4", buttonArguments));
		buttonArguments.erase("speed");

		if (locoBaseIdentifier.GetObjectType() == ObjectTypeMultipleUnit)
		{
			id = "multipleunitedit_" + to_string(locoBaseIdentifier.GetObjectID());
			buttonArguments["multipleunit"] = to_string(locoBaseIdentifier.GetObjectID());
		}
		else
		{
			id = "locoedit_" + to_string(locoID);
		}
		container.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><circle r=\"7\" cx=\"14\" cy=\"14\" fill=\"black\" /><line x1=\"14\" y1=\"5\" x2=\"14\" y2=\"23\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"9.5\" y1=\"6.2\" x2=\"18.5\" y2=\"21.8\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"6.2\" y1=\"9.5\" x2=\"21.8\" y2=\"18.5\" stroke-width=\"2\" stroke=\"black\" /><line y1=\"14\" x1=\"5\" y2=\"14\" x2=\"23\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"9.5\" y1=\"21.8\" x2=\"18.5\" y2=\"6.2\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"6.2\" y1=\"18.5\" x2=\"21.8\" y2=\"9.5\" stroke-width=\"2\" stroke=\"black\" /><circle r=\"5\" cx=\"14\" cy=\"14\" fill=\"lightgray\" /><circle r=\"4\" cx=\"24\" cy=\"24\" fill=\"black\" /><line x1=\"18\" y1=\"24\" x2=\"30\" y2=\"24\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"28.2\" y1=\"28.2\" x2=\"19.8\" y2=\"19.8\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"24\" y1=\"18\" x2=\"24\" y2=\"30\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"19.8\" y1=\"28.2\" x2=\"28.2\" y2=\"19.8\" stroke-width=\"2\" stroke=\"black\" /><circle r=\"2\" cx=\"24\" cy=\"24\" fill=\"lightgray\" /></svg>", id, buttonArguments));
		if (locoBaseIdentifier.GetObjectType() == ObjectTypeMultipleUnit)
		{
			buttonArguments.erase("multipleunit");
		}

		id = "locoorientation_" + to_string(locoID);
		container.AddChildTag(HtmlTagButtonCommandToggle("<svg width=\"36\" height=\"36\">"
			"<polyline points=\"5,15 31,15 31,23 5,23\" stroke=\"black\" stroke-width=\"0\" fill=\"black\" />"
			"<polyline points=\"16,8 0,19 16,30\" stroke=\"black\" stroke-width=\"0\" fill=\"black\" class=\"orientation_left\" />"
			"<polyline points=\"20,8 36,19 20,30\" stroke=\"black\" stroke-width=\"0\" fill=\"black\" class=\"orientation_right\" />"
			"</svg>", id, locoBase->GetOrientation(), buttonArguments).AddClass("button_orientation"));

		id = "locofunction_" + to_string(locoID);
		std::vector<DataModel::LocoFunctionEntry> functions = locoBase->GetFunctionStates();
		for (DataModel::LocoFunctionEntry& function : functions)
		{
			string nrText(to_string(function.nr));
			buttonArguments["function"] = nrText;
			switch(function.type)
			{
				case DataModel::LocoFunctionTypeMoment:
					container.AddChildTag(HtmlTagButtonCommandPressRelease(DataModel::LocoFunctions::GetLocoFunctionIcon(function.nr, function.icon), id + "_" + nrText, buttonArguments));
					break;

				default:
					container.AddChildTag(HtmlTagButtonCommandToggle(DataModel::LocoFunctions::GetLocoFunctionIcon(function.nr, function.icon), id + "_" + nrText, function.state, buttonArguments));
					break;
			}
		}
		buttonArguments.erase("function");
		container.AddChildTag(HtmlTagInputHidden("loco", to_string(locoID)).AddId("loco"));
		ReplyHtmlWithHeader(container);
	}

	void WebClient::HandleNewPosition(const map<string, string>& arguments)
	{
		string result;
		HandleNewPositionInternal(arguments, result);
		ReplyHtmlWithHeaderAndParagraph(result);
	}

	void WebClient::HandleNewPositionInternal(const map<string, string>& arguments, string& result)
	{
		const LayoutPosition posX = static_cast<LayoutPosition>(Utils::Utils::GetIntegerMapEntry(arguments, "x", -1));
		if (posX == -1)
		{
			return;
		}

		const LayoutPosition posY = static_cast<LayoutPosition>(Utils::Utils::GetIntegerMapEntry(arguments, "y", -1));
		if (posY == -1)
		{
			return;
		}

		manager.LayoutItemNewPosition(ObjectIdentifier(arguments), posX, posY, result);
	}

	void WebClient::HandleRotate(const map<string, string>& arguments)
	{
		string result;
		manager.LayoutItemRotate(ObjectIdentifier(arguments), result);
		ReplyHtmlWithHeaderAndParagraph(result);
	}

	void WebClient::PrintMainHTML() {
		// handle base request
		HtmlTag body("body");
		body.AddAttribute("onload", "startUp();");
		body.AddId("body");

		map<string,string> buttonArguments;

		HtmlTag menu("div");
		menu.AddClass("menu");
		HtmlTag menuMain("div");
		menuMain.AddClass("menu_main");
		menuMain.AddChildTag(HtmlTagButtonCommand("<svg width=\"36\" height=\"36\"><polygon points=\"16,1.5 31,1.5 31,25.5 16,25.5\" fill=\"white\" style=\"stroke:black;stroke-width:1;\"/><polygon points=\"21,11.5 31,1.5 31,25.5 21,35.5\" fill=\"black\" style=\"stroke:black;stroke-width:1;\"/><polygon points=\"1,11 8.5,11 8.5,6 16,13.5 8.5,21 8.5,16 1,16\"/></svg>", "quit", Languages::TextExitRailControl));
		menuMain.AddChildTag(HtmlTagButtonCommandToggle("<svg width=\"36\" height=\"36\"><polyline points=\"13.5,9.8 12.1,10.8 10.8,12.1 9.8,13.5 9.1,15.1 8.7,16.8 8.5,18.5 8.7,20.2 9.1,21.9 9.8,23.5 10.8,24.9 12.1,26.2 13.5,27.2 15.1,27.9 16.8,28.3 18.5,28.5 20.2,28.3 21.9,27.9 23.5,27.2 24.9,26.2 26.2,24.9 27.2,23.5 27.9,21.9 28.3,20.2 28.5,18.5 28.3,16.8 27.9,15.1 27.2,13.5 26.2,12.1 24.9,10.8 23.5,9.8\" stroke=\"black\" stroke-width=\"3\" fill=\"none\"/><polyline points=\"18.5,3.5 18.5,16\" stroke=\"black\" stroke-width=\"3\" fill=\"none\"/></svg>", "booster", manager.Booster(), Languages::TextTurningBoosterOnOrOff).AddClass("button_booster"));
		menuMain.AddChildTag(HtmlTagButtonCommand("<svg width=\"36\" height=\"36\"><polyline points=\"2,12 2,11 11,2 26,2 35,11 35,26 26,35 11,35 2,26 2,12\" stroke=\"black\" stroke-width=\"1\" fill=\"red\"/><text x=\"4\" y=\"22\" fill=\"white\" font-size=\"11\">STOP</text></svg>", "stopallimmediately", Languages::TextStopAllLocos));
		menuMain.AddChildTag(HtmlTagButtonCommand("<svg width=\"36\" height=\"36\"><polygon points=\"17,36 17,28 15,28 10,23 10,5 15,0 21,0 26,5 26,23 21,28 19,28 19,36\" fill=\"black\" /><circle cx=\"18\" cy=\"8\" r=\"4\" fill=\"red\" /><circle cx=\"18\" cy=\"20\" r=\"4\" fill=\"darkgray\" /></svg>", "stopall", Languages::TextSetAllLocosToManualMode));
		menuMain.AddChildTag(HtmlTagButtonCommand("<svg width=\"36\" height=\"36\"><polygon points=\"17,36 17,28 15,28 10,23 10,5 15,0 21,0 26,5 26,23 21,28 19,28 19,36\" fill=\"black\" /><circle cx=\"18\" cy=\"8\" r=\"4\" fill=\"darkgray\" /><circle cx=\"18\" cy=\"20\" r=\"4\" fill=\"green\" /></svg>", "startall", Languages::TextSetAllLocosToAutomode));
		menuMain.AddChildTag(HtmlTag().AddContent("&nbsp;&nbsp;&nbsp;"));
		menu.AddChildTag(menuMain);

		HtmlTag menuAdd("div");
		menuAdd.AddClass("menu_add");
		menuAdd.AddChildTag(HtmlTagButtonCommandFullScreen());
		menuAdd.AddChildTag(HtmlTag().AddContent("&nbsp;&nbsp;&nbsp;"));
		if (manager.CanHandle(Hardware::CapabilityProgram))
		{
			menuAdd.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"1,5 35,5\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"1,16 35,16\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"3,3 3,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"6,3 6,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"9,3 9,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"12,3 12,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"15,3 15,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"18,3 18,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"21,3 21,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"24,3 24,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"27,3 27,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"30,3 30,18\" stroke=\"black\" stroke-width=\"1\" /><polyline points=\"33,3 33,18\" stroke=\"black\" stroke-width=\"1\" /><text x=\"3\" y=\"31\" fill=\"black\" >Prog</text></svg>", "program", Languages::TextProgrammer));
			menuAdd.AddChildTag(HtmlTag().AddContent("&nbsp;&nbsp;&nbsp;"));
		}
		menu.AddChildTag(menuAdd);

		HtmlTag menuConfigButton("div");
		menuConfigButton.AddClass("menu_configbutton");
		menuConfigButton.AddId("menu_configbutton");
		menuConfigButton.AddChildTag(HtmlTagButton("<svg width=\"36\" height=\"36\"><polyline points=\"5,11 31,11\" stroke=\"black\" stroke-width=\"3\"/><polyline points=\"5,18 31,18\" stroke=\"black\" stroke-width=\"3\"/><polyline points=\"5,25 31,25\" stroke=\"black\" stroke-width=\"3\"/></svg>", "showmenuconfig", Languages::TextConfigMenu).AddAttribute("onclick", "showMenuConfig(); return false;").AddClass("button_menuconfig"));
		menuConfigButton.AddChildTag(HtmlTag().AddContent("&nbsp;&nbsp;&nbsp;"));
		menu.AddChildTag(menuConfigButton);

		HtmlTag menuConfig("div");
		menuConfig.AddClass("menu_config");
		menuConfig.AddId("menu_config");
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><circle r=\"7\" cx=\"14\" cy=\"14\" fill=\"black\" /><line x1=\"14\" y1=\"5\" x2=\"14\" y2=\"23\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"9.5\" y1=\"6.2\" x2=\"18.5\" y2=\"21.8\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"6.2\" y1=\"9.5\" x2=\"21.8\" y2=\"18.5\" stroke-width=\"2\" stroke=\"black\" /><line y1=\"14\" x1=\"5\" y2=\"14\" x2=\"23\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"9.5\" y1=\"21.8\" x2=\"18.5\" y2=\"6.2\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"6.2\" y1=\"18.5\" x2=\"21.8\" y2=\"9.5\" stroke-width=\"2\" stroke=\"black\" /><circle r=\"5\" cx=\"14\" cy=\"14\" fill=\"white\" /><circle r=\"4\" cx=\"24\" cy=\"24\" fill=\"black\" /><line x1=\"18\" y1=\"24\" x2=\"30\" y2=\"24\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"28.2\" y1=\"28.2\" x2=\"19.8\" y2=\"19.8\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"24\" y1=\"18\" x2=\"24\" y2=\"30\" stroke-width=\"2\" stroke=\"black\" /><line x1=\"19.8\" y1=\"28.2\" x2=\"28.2\" y2=\"19.8\" stroke-width=\"2\" stroke=\"black\" /><circle r=\"2\" cx=\"24\" cy=\"24\" fill=\"white\" /></svg>", "settingsedit", Languages::TextEditSettings));
		menuConfig.AddChildTag(HtmlTag().AddContent("&nbsp;&nbsp;&nbsp;"));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polygon points=\"11,1.5 26,1.5 26,35.5 11,35.5\" fill=\"white\" style=\"stroke:black;stroke-width:1;\"/><polygon points=\"14,4.5 23,4.5 23,8.5 14,8.5\" fill=\"white\" style=\"stroke:black;stroke-width:1;\"/><circle cx=\"15.5\" cy=\"12\" r=\"1\" fill=\"black\"/><circle cx=\"18.5\" cy=\"12\" r=\"1\" fill=\"black\"/><circle cx=\"21.5\" cy=\"12\" r=\"1\" fill=\"black\"/><circle cx=\"15.5\" cy=\"15\" r=\"1\" fill=\"black\"/><circle cx=\"18.5\" cy=\"15\" r=\"1\" fill=\"black\"/><circle cx=\"21.5\" cy=\"15\" r=\"1\" fill=\"black\"/><circle cx=\"15.5\" cy=\"18\" r=\"1\" fill=\"black\"/><circle cx=\"18.5\" cy=\"18\" r=\"1\" fill=\"black\"/><circle cx=\"21.5\" cy=\"18\" r=\"1\" fill=\"black\"/><circle cx=\"15.5\" cy=\"21\" r=\"1\" fill=\"black\"/><circle cx=\"18.5\" cy=\"21\" r=\"1\" fill=\"black\"/><circle cx=\"21.5\" cy=\"21\" r=\"1\" fill=\"black\"/><circle cx=\"18.5\" cy=\"28.5\" r=\"5\" fill=\"black\"/></svg>", "controllist", Languages::TextEditControls));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"0,25 35,25\" fill=\"none\" stroke=\"black\"/><polygon points=\"35,22 6,22 5,19 8,10 35,10\" stroke=\"black\" fill=\"black\"/><polygon points=\"10,12 15,12 15,15 9,15\" fill=\"white\"/><polyline points=\"16,9 20,7 16,5\" stroke=\"black\" fill=\"none\"/><circle cx=\"12\" cy=\"22\" r=\"3\"/><circle cx=\"20\" cy=\"22\" r=\"3\"/></svg>", "locolist", Languages::TextEditLocos));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"0,25 35,25\" fill=\"none\" stroke=\"black\"/><polygon points=\"0,22 0,10 12,10 15,19 14,22\" stroke=\"black\" fill=\"black\"/><polygon points=\"10,12 11,15 5,15 5,12\" fill=\"white\"/><polyline points=\"8,9 4,7 8,5\" stroke=\"black\" fill=\"none\"/><circle cx=\"8\" cy=\"22\" r=\"3\"/><circle cx=\"0\" cy=\"22\" r=\"3\"/><polygon points=\"35,22 21,22 20,19 23,10 35,10\" stroke=\"black\" fill=\"black\"/><polygon points=\"25,12 30,12 30,15 24,15\" fill=\"white\"/><polyline points=\"27,9 31,7 27,5\" stroke=\"black\" fill=\"none\"/><circle cx=\"27\" cy=\"22\" r=\"3\"/><circle cx=\"35\" cy=\"22\" r=\"3\"/><polyline points=\"0,20 35,20\" fill=\"none\" stroke=\"black\"/></svg>", "multipleunitlist", Languages::TextEditMultipleUnits));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polygon points=\"2,31 26,31 35,21 11,21\" fill=\"white\" stroke=\"black\"/><polygon points=\"2,26 26,26 35,16 11,16\" fill=\"white\" stroke=\"black\"/><polygon points=\"2,21 26,21 35,11 11,11\" fill=\"white\" stroke=\"black\"/><polygon points=\"2,16 26,16 35,6 11,6\" fill=\"white\" stroke=\"black\"/></svg>", "layerlist", Languages::TextEditLayers));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"1,12 35,12\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"1,23 35,23\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"3,10 3,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"6,10 6,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"9,10 9,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"12,10 12,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"15,10 15,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"18,10 18,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"21,10 21,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"24,10 24,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"27,10 27,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"30,10 30,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"33,10 33,25\" stroke=\"black\" stroke-width=\"1\"/></svg>", "tracklist", Languages::TextEditTracks));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"1,12 17,12\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"1,23 17,23\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"19,12 35,12\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"19,23 35,23\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"3,10 3,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"6,10 6,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"9,10 9,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"12,10 12,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"15,10 15,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"21,10 21,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"24,10 24,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"27,10 27,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"30,10 30,25\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"33,10 33,25\" stroke=\"black\" stroke-width=\"1\"/></svg>", "clusterlist", Languages::TextEditClusters));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"1,20 7.1,19.5 13,17.9 18.5,15.3 23.5,11.8 27.8,7.5\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"1,28 8.5,27.3 15.7,25.4 22.5,22.2 28.6,17.9 33.9,12.6\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"1,20 35,20\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"1,28 35,28\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"3,18 3,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"6,18 6,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"9,17 9,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"12,16 12,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"15,15 15,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"18,13 18,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"21,12 21,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"24,9 24,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"27,17 27,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"30,18 30,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"33,18 33,30\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"24,9 32,17\" stroke=\"black\" stroke-width=\"1\"/><polyline points=\"26,7 34,15\" stroke=\"black\" stroke-width=\"1\"/></svg>", "switchlist", Languages::TextEditSwitches));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polygon points=\"17,36 17,28 15,28 10,23 10,5 15,0 21,0 26,5 26,23 21,28 19,28 19,36\" fill=\"black\" /><circle cx=\"18\" cy=\"8\" r=\"4\" fill=\"red\" /><circle cx=\"18\" cy=\"20\" r=\"4\" fill=\"green\" /></svg>", "signallist", Languages::TextEditSignals));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"1,20 10,20 30,15\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"28,17 28,20 34,20\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/></svg>", "accessorylist", Languages::TextEditAccessories));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"1,25 35,25\" fill=\"none\" stroke=\"black\"/><polygon points=\"4,25 4,23 8,23 8,25\" fill=\"black\" stroke=\"black\"/><polygon points=\"35,22 16,22 15,19 18,10 35,10\" stroke=\"black\" fill=\"black\"/><polygon points=\"20,12 25,12 25,15 19,15\" fill=\"white\"/><polyline points=\"26,10 30,8 26,6\" stroke=\"black\" fill=\"none\"/><circle cx=\"22\" cy=\"22\" r=\"3\"/><circle cx=\"30\" cy=\"22\" r=\"3\"/></svg>", "feedbacklist", Languages::TextEditFeedbacks));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><polyline points=\"5,34 15,1\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"31,34 21,1\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"18,34 18,30\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"18,24 18,20\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"18,14 18,10\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/><polyline points=\"18,4 18,1\" stroke=\"black\" stroke-width=\"1\" fill=\"none\"/></svg>", "routelist", Languages::TextEditRoutes));
		menuConfig.AddChildTag(HtmlTagButtonPopup("<svg width=\"36\" height=\"36\"><text x=\"4\" y=\"22\" fill=\"black\" font-size=\"15\">Text</text></svg>", "textlist", Languages::TextEditTexts));
		menu.AddChildTag(menuConfig);

		body.AddChildTag(menu);

		const unsigned int MaxNumberOfLocoControls = 5;
		for (unsigned int i = 1; i <= MaxNumberOfLocoControls; ++i)
		{
			const string iText = to_string(i);
			HtmlTag locoContainer("div");
			locoContainer.AddClass("loco_container");
			locoContainer.AddId("loco_container_" + iText);

			HtmlTag locoSelector("div");
			locoSelector.AddClass("loco_selector");
			locoSelector.AddClass("loco_selector_" + iText);
			locoSelector.AddId("loco_selector_" + iText);
			locoSelector.AddChildTag(HtmlTagLocoSelector(iText));
			locoContainer.AddChildTag(locoSelector);

			HtmlTag loco("div");
			loco.AddClass("loco");
			loco.AddClass("loco_" + iText);
			loco.AddId("loco_" + iText);
			locoContainer.AddChildTag(loco);
			if (i > 1)
			{
				locoContainer.AddClass("hidden");
			}
			body.AddChildTag(locoContainer);
		}
		body.AddChildTag(HtmlTag("div").AddClass("layer_selector").AddId("layer_selector").AddChildTag(HtmlTagLayerSelector()));
		body.AddChildTag(HtmlTag("div").AddClass("layout").AddId("layout")
			.AddAttribute("oncontextmenu", "loadLayoutContext(event);")
			.AddAttribute("ondragover", "allowDrop(event);")
			.AddAttribute("ondrop", "drop(event);")
			);
		body.AddChildTag(HtmlTag("div").AddClass("reduce_locos").AddId("reduce_locos").AddContent("&lt;").AddAttribute("onclick", "reduceLocos(); return false;"));
		body.AddChildTag(HtmlTag("div").AddClass("extend_locos").AddId("extend_locos").AddContent("&gt;").AddAttribute("onclick", "extendLocos(); return false;"));
		body.AddChildTag(HtmlTag("div").AddClass("clock").AddId("clock").AddContent("<object data=\"/station-clock.svg\" class=\"clock2\" type=\"image/svg+xml\"><param name=\"secondHand\" value=\"din 41071.1\"/><param name=\"minuteHandBehavior\" value=\"sweeping\"/><param name=\"secondHandBehavior\" value=\"steeping\"/><param name=\"axisCoverRadius\" value=\"0\"/><param name=\"updateInterval\" value=\"250\"/></object>"));
		body.AddChildTag(HtmlTag("div").AddClass("status").AddId("status"));
		body.AddChildTag(HtmlTag("div").AddClass("popup").AddId("popup"));
		body.AddChildTag(HtmlTag("div").AddClass("infobox").AddId("infobox"));

		body.AddChildTag(HtmlTag("div").AddClass("contextmenu").AddId("layout_context")
			.AddChildTag(HtmlTag("ul").AddClass("contextentries")
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddClass("real_layer_only").AddContent(Languages::GetText(Languages::TextAddTrack)).AddAttribute("onClick", "loadPopup('/?cmd=trackedit&track=0');"))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddClass("real_layer_only").AddContent(Languages::GetText(Languages::TextAddSwitch)).AddAttribute("onClick", "loadPopup('/?cmd=switchedit&switch=0');"))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddClass("real_layer_only").AddContent(Languages::GetText(Languages::TextAddSignal)).AddAttribute("onClick", "loadPopup('/?cmd=signaledit&signal=0');"))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddClass("real_layer_only").AddContent(Languages::GetText(Languages::TextAddAccessory)).AddAttribute("onClick", "loadPopup('/?cmd=accessoryedit&accessory=0');"))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddContent(Languages::GetText(Languages::TextAddFeedback)).AddAttribute("onClick", "loadPopup('/?cmd=feedbackedit&feedback=0');"))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddClass("real_layer_only").AddContent(Languages::GetText(Languages::TextAddRoute)).AddAttribute("onClick", "loadPopup('/?cmd=routeedit&route=0');"))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddClass("real_layer_only").AddContent(Languages::GetText(Languages::TextAddText)).AddAttribute("onClick", "loadPopup('/?cmd=textedit&text=0');"))
			));

		connection->Send(ResponseHtmlFull("RailControl", body));
	}
}} // namespace Server::Web
