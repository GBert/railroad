cmake_minimum_required(VERSION 3.10)

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
	find_package(Git QUIET)
	if(NOT Git_FOUND)
		message(WARNING "Git not found")
		return()
	endif()

	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE RAILCONTROL_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} log -1 --format=%H
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} log -1 --format=%at
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_TIMESTAMP
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} status -s
		COMMAND wc -l
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_DIRTY
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
else()
	file(READ VERSION_RAILCONTROL RAILCONTROL_VERSION)
	string(STRIP ${RAILCONTROL_VERSION} RAILCONTROL_VERSION)
	file(READ VERSION_GIT_HASH GIT_HASH)
	string(STRIP ${GIT_HASH} GIT_HASH)
	file(READ VERSION_GIT_TIMESTAMP GIT_TIMESTAMP)
	string(STRIP ${GIT_TIMESTAMP} GIT_TIMESTAMP)
	set(GIT_DIRTY 0)
endif()

# Version starts with v
string(REGEX REPLACE "^v" "" RAILCONTROL_VERSION "${RAILCONTROL_VERSION}")

project(railcontrol VERSION ${RAILCONTROL_VERSION})

STRING(TIMESTAMP COMPILE_TIMESTAMP "%s" UTC)

add_executable(${PROJECT_NAME}
Manager.cpp
Manager.h
Server/Web/WebClient.cpp
Server/Web/WebClient.h
ArgumentHandler.cpp
ArgumentHandler.h
Config.cpp
Config.h
ControlInterface.h
DataModel/Accessory.cpp
DataModel/Accessory.h
DataModel/AccessoryBase.cpp
DataModel/AccessoryBase.h
DataModel/AccessoryConfig.h
DataModel/Cluster.cpp
DataModel/Cluster.h
DataModel/Counter.cpp
DataModel/Counter.h
DataModel/DataModel.h
DataModel/Feedback.cpp
DataModel/Feedback.h
DataModel/FeedbackConfig.h
DataModel/HardwareHandle.cpp
DataModel/HardwareHandle.h
DataModel/Layer.h
DataModel/LayoutItem.cpp
DataModel/LayoutItem.h
DataModel/LockableItem.cpp
DataModel/LockableItem.h
DataModel/Loco.cpp
DataModel/Loco.h
DataModel/LocoBase.cpp
DataModel/LocoBase.h
DataModel/LocoConfig.h
DataModel/LocoFunctions.cpp
DataModel/LocoFunctions.h
DataModel/MultipleUnit.cpp
DataModel/MultipleUnit.h
DataModel/Object.cpp
DataModel/Object.h
DataModel/ObjectIdentifier.cpp
DataModel/ObjectIdentifier.h
DataModel/Relation.cpp
DataModel/Relation.h
DataModel/Route.cpp
DataModel/Route.h
DataModel/Serializable.cpp
DataModel/Serializable.h
DataModel/Signal.cpp
DataModel/Signal.h
DataModel/Switch.cpp
DataModel/Switch.h
DataModel/Text.cpp
DataModel/Text.h
DataModel/Track.cpp
DataModel/Track.h
DataTypes.h
Fallthrough.h
Hardware/AccessoryCache.cpp
Hardware/AccessoryCache.h
Hardware/CS1.h
Hardware/CS2Tcp.cpp
Hardware/CS2Tcp.h
Hardware/CS2Udp.cpp
Hardware/CS2Udp.h
Hardware/Capabilities.h
Hardware/CcSchnitte.cpp
Hardware/CcSchnitte.h
Hardware/DR5000.h
Hardware/DccPpExSerial.cpp
Hardware/DccPpExSerial.h
Hardware/DccPpExTcp.cpp
Hardware/DccPpExTcp.h
Hardware/Ecos.h
Hardware/FeedbackCache.cpp
Hardware/FeedbackCache.h
Hardware/HardwareHandler.cpp
Hardware/HardwareHandler.h
Hardware/HardwareInterface.h
Hardware/HardwareParams.h
Hardware/Hsi88.cpp
Hardware/Hsi88.h
Hardware/Intellibox.h
Hardware/Intellibox2.h
Hardware/LocoCache.cpp
Hardware/LocoCache.h
Hardware/LocoNetAdapter63120.h
Hardware/LocoNetAdapter63820.h
Hardware/M6051.cpp
Hardware/M6051.h
Hardware/MasterControl.h
Hardware/MasterControl2.h
Hardware/OpenDcc.h
Hardware/Protocols/DccPpEx.cpp
Hardware/Protocols/DccPpEx.h
Hardware/Protocols/EsuCAN.cpp
Hardware/Protocols/EsuCAN.h
Hardware/Protocols/LocoNet.cpp
Hardware/Protocols/LocoNet.h
Hardware/Protocols/LocoNetLocoCache.h
Hardware/Protocols/MaerklinCAN.h
Hardware/Protocols/MaerklinCANCommon.cpp
Hardware/Protocols/MaerklinCANCommon.h
Hardware/Protocols/P50x.cpp
Hardware/Protocols/P50x.h
Hardware/Protocols/P50xCache.h
Hardware/Protocols/P50xEthernet.h
Hardware/Protocols/P50xSerial.h
Hardware/Protocols/P50xUhlenbrock.h
Hardware/Protocols/Z21.cpp
Hardware/Protocols/Z21.h
Hardware/Protocols/Z21FeedbackCache.h
Hardware/Protocols/Z21LocoCache.h
Hardware/Protocols/Z21TurnoutCache.h
Hardware/RedBox.h
Hardware/Rektor.h
Hardware/TwinCenter.h
Hardware/Virtual.cpp
Hardware/Virtual.h
Hardware/Z21.h
Hardware/ZLib.cpp
Hardware/ZLib.h
Languages.cpp
Languages.h
Logger/Logger.cpp
Logger/Logger.h
Logger/LoggerClient.h
Logger/LoggerClientConsole.h
Logger/LoggerClientFile.h
Logger/LoggerClientTcp.h
Logger/LoggerServer.cpp
Logger/LoggerServer.h
Network/Select.h
Network/Serial.cpp
Network/Serial.h
Network/TcpClient.cpp
Network/TcpClient.h
Network/TcpConnection.cpp
Network/TcpConnection.h
Network/TcpServer.cpp
Network/TcpServer.h
Network/UdpClient.h
Network/UdpConnection.cpp
Network/UdpConnection.h
Network/UdpServer.cpp
Network/UdpServer.h
RailControl.cpp
RailControl.h
Server/CS2/CS2Client.cpp
Server/CS2/CS2Client.h
Server/CS2/CS2Server.cpp
Server/CS2/CS2Server.h
Server/Web/HtmlTag.cpp
Server/Web/HtmlTag.h
Server/Web/HtmlTagAccessory.cpp
Server/Web/HtmlTagAccessory.h
Server/Web/HtmlTagButton.cpp
Server/Web/HtmlTagButton.h
Server/Web/HtmlTagButtonCancel.h
Server/Web/HtmlTagButtonCommand.cpp
Server/Web/HtmlTagButtonCommand.h
Server/Web/HtmlTagButtonCommandFullScreen.cpp
Server/Web/HtmlTagButtonCommandFullScreen.h
Server/Web/HtmlTagButtonCommandPressRelease.cpp
Server/Web/HtmlTagButtonCommandPressRelease.h
Server/Web/HtmlTagButtonCommandToggle.cpp
Server/Web/HtmlTagButtonCommandToggle.h
Server/Web/HtmlTagButtonCommandWide.h
Server/Web/HtmlTagButtonMinus.h
Server/Web/HtmlTagButtonOK.h
Server/Web/HtmlTagButtonPlus.h
Server/Web/HtmlTagButtonPopup.cpp
Server/Web/HtmlTagButtonPopup.h
Server/Web/HtmlTagButtonPopupWide.h
Server/Web/HtmlTagCounter.cpp
Server/Web/HtmlTagCounter.h
Server/Web/HtmlTagFeedback.cpp
Server/Web/HtmlTagFeedback.h
Server/Web/HtmlTagInput.cpp
Server/Web/HtmlTagInput.h
Server/Web/HtmlTagInputCheckbox.h
Server/Web/HtmlTagInputCheckboxWithLabel.h
Server/Web/HtmlTagInputHidden.h
Server/Web/HtmlTagInputInteger.cpp
Server/Web/HtmlTagInputInteger.h
Server/Web/HtmlTagInputIntegerWithLabel.h
Server/Web/HtmlTagInputSlider.cpp
Server/Web/HtmlTagInputSlider.h
Server/Web/HtmlTagInputSliderLocoSpeed.cpp
Server/Web/HtmlTagInputSliderLocoSpeed.h
Server/Web/HtmlTagInputText.h
Server/Web/HtmlTagInputTextWithLabel.h
Server/Web/HtmlTagLabel.h
Server/Web/HtmlTagLayoutItem.cpp
Server/Web/HtmlTagLayoutItem.h
Server/Web/HtmlTagRoute.cpp
Server/Web/HtmlTagRoute.h
Server/Web/HtmlTagSelect.cpp
Server/Web/HtmlTagSelect.h
Server/Web/HtmlTagSelectMultiple.cpp
Server/Web/HtmlTagSelectMultiple.h
Server/Web/HtmlTagSelectMultipleWithLabel.cpp
Server/Web/HtmlTagSelectMultipleWithLabel.h
Server/Web/HtmlTagSelectOrientation.h
Server/Web/HtmlTagSelectOrientationWithLabel.h
Server/Web/HtmlTagSelectWithLabel.cpp
Server/Web/HtmlTagSelectWithLabel.h
Server/Web/HtmlTagSignal.cpp
Server/Web/HtmlTagSignal.h
Server/Web/HtmlTagSwitch.cpp
Server/Web/HtmlTagSwitch.h
Server/Web/HtmlTagText.cpp
Server/Web/HtmlTagText.h
Server/Web/HtmlTagTrack.cpp
Server/Web/HtmlTagTrack.h
Server/Web/Response.cpp
Server/Web/Response.h
Server/Web/ResponseCsv.cpp
Server/Web/ResponseCsv.h
Server/Web/ResponseHtml.cpp
Server/Web/ResponseHtml.h
Server/Web/ResponseHtmlFull.cpp
Server/Web/ResponseHtmlFull.h
Server/Web/ResponseHtmlNotFound.cpp
Server/Web/ResponseHtmlNotFound.h
Server/Web/ResponseHtmlNotImplemented.cpp
Server/Web/ResponseHtmlNotImplemented.h
Server/Web/WebClientCluster.cpp
Server/Web/WebClientCluster.h
Server/Web/WebClientCounter.cpp
Server/Web/WebClientCounter.h
Server/Web/WebClientRoute.cpp
Server/Web/WebClientRoute.h
Server/Web/WebClientSignal.cpp
Server/Web/WebClientSignal.h
Server/Web/WebClientStatic.cpp
Server/Web/WebClientStatic.h
Server/Web/WebClientText.cpp
Server/Web/WebClientText.h
Server/Web/WebClientTrack.cpp
Server/Web/WebClientTrack.h
Server/Web/WebServer.cpp
Server/Web/WebServer.h
Server/Z21/Z21Client.cpp
Server/Z21/Z21Client.h
Server/Z21/Z21Server.cpp
Server/Z21/Z21Server.h
Storage/Sqlite.cpp
Storage/Sqlite.h
Storage/StorageHandler.cpp
Storage/StorageHandler.h
Storage/StorageInterface.h
Storage/StorageParams.h
Storage/TransactionGuard.cpp
Storage/TransactionGuard.h
Utils/Integer.cpp
Utils/Integer.h
Utils/Network.cpp
Utils/Network.h
Utils/ThreadSafeQueue.h
Utils/Utils.cpp
Utils/Utils.h
${CMAKE_CURRENT_BINARY_DIR}/Version.cpp
Version.h
)

option(USE_SYSTEM_LIBRARIES "Build with system libraries" OFF)

if (USE_SYSTEM_LIBRARIES)
	find_package(SQLite3)
	target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)

	find_package(ZLIB)
	target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
else()
	target_include_directories(${PROJECT_NAME} PRIVATE Storage/sqlite)
	add_library(sqlite3 STATIC
		Storage/sqlite/sqlite3.c
		Storage/sqlite/sqlite3.h
		Storage/sqlite/sqlite3ext.h
	)
	target_link_libraries(${PROJECT_NAME} PRIVATE sqlite3)

	target_include_directories(${PROJECT_NAME} PRIVATE Hardware/zlib)
	add_library(zlib STATIC
		Hardware/zlib/adler32.c
		Hardware/zlib/compress.c
		Hardware/zlib/crc32.c
		Hardware/zlib/crc32.h
		Hardware/zlib/deflate.c
		Hardware/zlib/deflate.h
		Hardware/zlib/gzclose.c
		Hardware/zlib/gzguts.h
		Hardware/zlib/gzlib.c
		Hardware/zlib/gzread.c
		Hardware/zlib/gzwrite.c
		Hardware/zlib/infback.c
		Hardware/zlib/inffast.c
		Hardware/zlib/inffast.h
		Hardware/zlib/inffixed.h
		Hardware/zlib/inflate.c
		Hardware/zlib/inflate.h
		Hardware/zlib/inftrees.c
		Hardware/zlib/inftrees.h
		Hardware/zlib/trees.c
		Hardware/zlib/trees.h
		Hardware/zlib/uncompr.c
		Hardware/zlib/zconf.h
		Hardware/zlib/zlib.h
		Hardware/zlib/zutil.c
		Hardware/zlib/zutil.h
	)
	target_link_libraries(${PROJECT_NAME} PRIVATE zlib)
endif()

option(USE_CPPTRACE "Build with cpptrace library" OFF)

if (USE_CPPTRACE)
	find_package(cpptrace CONFIG REQUIRED)
	target_link_libraries(${PROJECT_NAME} PRIVATE cpptrace::cpptrace)
endif()

set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -DSQLITE_ENABLE_FTS4 -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RTREE -DHAVE_USLEEP -Wno-implicit-function-declaration -Wno-return-local-addr")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads dl)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

configure_file(Version.cpp.in Version.cpp @ONLY)

set(CMAKE_INSTALL_BINDIR ".")
set(DATADIR ".")

install(TARGETS ${PROJECT_NAME})
install(FILES railcontrol.conf.dist DESTINATION ${DATADIR})
install(DIRECTORY html DESTINATION ${DATADIR})

include(CPack)

# Documentation
find_program(PANDOC_EXECUTABLE pandoc)
if(NOT PANDOC_EXECUTABLE)
	message(WARNING "pandoc not found!")
endif()

set(DOC_DE
	Documentation/de/index.md
	Documentation/de/startup-arguments.md
	Documentation/de/general-settings.md
	Documentation/de/config-controls.md
	Documentation/de/config-intellibox.md
	Documentation/de/config-mastercontrol2.md
	Documentation/de/config-mastercontrol.md
	Documentation/de/config-redbox.md
	Documentation/de/config-twincenter.md
	Documentation/de/control-6051.md
	Documentation/de/control-cc-schnitte.md
	Documentation/de/control-cs2-tcp.md
	Documentation/de/control-cs2-udp.md
	Documentation/de/control-hsi88.md
	Documentation/de/control-opendcc-z1.md
	Documentation/de/control-z21.md
	Documentation/de/config-rs232-usb.md
	Documentation/de/config-locomotives.md
	Documentation/de/config-multipleunits.md
	Documentation/de/config-layers.md
	Documentation/de/config-tracks.md
	Documentation/de/config-groups.md
	Documentation/de/config-switches.md
	Documentation/de/config-signals.md
	Documentation/de/config-accessories.md
	Documentation/de/config-routes.md
	Documentation/de/config-feedbacks.md
	Documentation/de/config-texts.md
	Documentation/de/config-counter.md
	Documentation/de/automatic.md
)
set(DOCDATA_DE
    ${PROJECT_SOURCE_DIR}/Documentation/de/config_layers_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/control_list_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/counter_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/counter_position_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/cs2_config_can_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/cs2_config_ip_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/cs2_tcp_config_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/cs2_udp_config_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/feedback_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/feedback_position_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/groups_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/groups_tracks_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/locomotives_automode_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/locomotives_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/locomotives_functions_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/multipleunits_automode_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/multipleunits_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/multipleunits_functions_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/multipleunits_multipleunits_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/routes_atlock_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/routes_atunlock_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/routes_automode_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/routes_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/routes_position_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/settings_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/signals_addresses_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/signals_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/signals_position_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/switches_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/switches_position_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/texts_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/texts_position_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/tracks_automode_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/tracks_basics_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/tracks_feedbacks_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/track_signals_de.png
    ${PROJECT_SOURCE_DIR}/Documentation/de/tracks_position_de.png
)

set(DOC_EN
	Documentation/en/index.md
	Documentation/en/startup-arguments.md
	Documentation/en/general-settings.md
	Documentation/en/config-controls.md
	Documentation/en/config-intellibox.md
	Documentation/en/config-mastercontrol2.md
	Documentation/en/config-mastercontrol.md
	Documentation/en/config-redbox.md
	Documentation/en/config-twincenter.md
	Documentation/en/control-6051.md
	Documentation/en/control-cc-schnitte.md
	Documentation/en/control-cs2-tcp.md
	Documentation/en/control-cs2-udp.md
	Documentation/en/control-hsi88.md
	Documentation/en/control-opendcc-z1.md
	Documentation/en/control-z21.md
	Documentation/en/config-rs232-usb.md
	Documentation/en/config-locomotives.md
	Documentation/en/config-multipleunits.md
	Documentation/en/config-layers.md
	Documentation/en/config-tracks.md
	Documentation/en/config-groups.md
	Documentation/en/config-switches.md
	Documentation/en/config-signals.md
	Documentation/en/config-accessories.md
	Documentation/en/config-routes.md
	Documentation/en/config-feedbacks.md
	Documentation/en/config-texts.md
	Documentation/en/config-counter.md
	Documentation/en/automatic.md
)
set(DOCDATA_EN
    ${PROJECT_SOURCE_DIR}/Documentation/en/config_layers_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/control_list_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/counter_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/counter_position_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/cs2_tcp_config_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/cs2_udp_config_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/feedback_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/feedback_position_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/groups_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/groups_tracks_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/locomotives_automode_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/locomotives_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/locomotives_functions_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/multipleunits_automode_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/multipleunits_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/multipleunits_functions_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/multipleunits_multipleunits_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/routes_atlock_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/routes_atunlock_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/routes_automode_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/routes_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/routes_position_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/settings_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/signals_addresses_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/signals_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/signals_position_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/switches_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/switches_position_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/texts_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/texts_position_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/tracks_automode_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/tracks_basics_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/tracks_feedbacks_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/track_signals_en.png
    ${PROJECT_SOURCE_DIR}/Documentation/en/tracks_position_en.png
)

set(DOC_ES
	Documentation/es/index.md
	Documentation/es/startup-arguments.md
	Documentation/es/general-settings.md
	Documentation/es/config-controls.md
	Documentation/es/config-intellibox.md
	Documentation/es/config-mastercontrol2.md
	Documentation/es/config-mastercontrol.md
	Documentation/es/config-redbox.md
	Documentation/es/config-twincenter.md
	Documentation/es/control-6051.md
	Documentation/es/control-cc-schnitte.md
	Documentation/es/control-cs2-tcp.md
	Documentation/es/control-cs2-udp.md
	Documentation/es/control-hsi88.md
	Documentation/es/control-opendcc-z1.md
	Documentation/es/control-z21.md
	Documentation/es/config-rs232-usb.md
	Documentation/es/config-locomotives.md
	Documentation/es/config-multipleunits.md
	Documentation/es/config-layers.md
	Documentation/es/config-tracks.md
	Documentation/es/config-groups.md
	Documentation/es/config-switches.md
	Documentation/es/config-signals.md
	Documentation/es/config-accessories.md
	Documentation/es/config-routes.md
	Documentation/es/config-feedbacks.md
	Documentation/es/config-texts.md
	Documentation/es/config-counter.md
	Documentation/es/automatic.md
)
set(DOCDATA_ES
    ${PROJECT_SOURCE_DIR}/Documentation/es/config_layers_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/control_list_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/cs2_tcp_config_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/cs2_udp_config_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/feedback_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/feedback_position_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/groups_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/groups_tracks_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/locomotives_automode_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/locomotives_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/locomotives_functions_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/multipleunits_automode_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/multipleunits_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/multipleunits_functions_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/multipleunits_multipleunits_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/routes_atlock_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/routes_atunlock_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/routes_automode_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/routes_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/routes_position_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/settings_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/signals_addresses_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/signals_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/signals_position_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/switches_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/switches_position_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/texts_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/texts_position_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/tracks_automode_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/tracks_basics_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/tracks_feedbacks_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/track_signals_es.png
    ${PROJECT_SOURCE_DIR}/Documentation/es/tracks_position_es.png
)

set(DOCDATA
    ${PROJECT_SOURCE_DIR}/Documentation/menu_accessory.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_booster.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_control.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_counter.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_feedback.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_fullscreen.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_group.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_layer.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_loco.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_menu.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_multipleunit.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_program.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_quit.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_route.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_settings.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_signalgreen.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_signal.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_signalred.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_stop.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_street.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_switch.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_text.png
    ${PROJECT_SOURCE_DIR}/Documentation/menu_track.png
    ${PROJECT_SOURCE_DIR}/Documentation/style.css
)

set(OUTPUT_HTML_DE ${CMAKE_BINARY_DIR}/doc/de/index.html)
set(OUTPUT_HTML_EN ${CMAKE_BINARY_DIR}/doc/en/index.html)
set(OUTPUT_HTML_ES ${CMAKE_BINARY_DIR}/doc/es/index.html)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/de)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/en)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/es)

set(PANDOC_OPTS --toc --toc-depth=2
                --standalone
                --css=../style.css
                --template=Documentation/template.html)

add_custom_command(
    OUTPUT ${OUTPUT_HTML_DE}
    COMMAND pandoc ${PANDOC_OPTS}
                   --metadata-file=Documentation/de/metadata.yaml
                   --output=${OUTPUT_HTML_DE} ${DOC_DE}
    DEPENDS ${DOC_DE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating German HTML documentation"
    VERBATIM
)

add_custom_command(
    OUTPUT ${OUTPUT_HTML_EN}
    COMMAND pandoc ${PANDOC_OPTS}
                   --metadata-file=Documentation/en/metadata.yaml
                   --output=${OUTPUT_HTML_EN} ${DOC_EN}
    DEPENDS ${DOC_EN}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating English HTML documentation"
    VERBATIM
)

add_custom_command(
    OUTPUT ${OUTPUT_HTML_ES}
    COMMAND pandoc ${PANDOC_OPTS}
                   --metadata-file=Documentation/es/metadata.yaml
                   --output=${OUTPUT_HTML_ES} ${DOC_ES}
    DEPENDS ${DOC_ES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating Spanish HTML documentation"
    VERBATIM
)

add_custom_target(doc
    DEPENDS ${OUTPUT_HTML_DE} ${OUTPUT_HTML_EN} ${OUTPUT_HTML_ES}
)

foreach(DOCDATA ${DOCDATA})
    add_custom_command(TARGET doc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${DOCDATA}
            ${PROJECT_BINARY_DIR}/doc
    )
endforeach()

foreach(DOCDATA_DE ${DOCDATA_DE})
    add_custom_command(TARGET doc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${DOCDATA_DE}
            ${PROJECT_BINARY_DIR}/doc/de
    )
endforeach()

foreach(DOCDATA_EN ${DOCDATA_EN})
    add_custom_command(TARGET doc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${DOCDATA_EN}
            ${PROJECT_BINARY_DIR}/doc/en
    )
endforeach()

foreach(DOCDATA_ES ${DOCDATA_ES})
    add_custom_command(TARGET doc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${DOCDATA_ES}
            ${PROJECT_BINARY_DIR}/doc/es
    )
endforeach()

add_custom_target(doc-for-web
    DEPENDS doc
)

add_custom_command(TARGET doc-for-web POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/Documentation/de/documentation-de
            ${PROJECT_BINARY_DIR}/doc/de/documentation-de
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/Documentation/en/documentation-en
            ${PROJECT_BINARY_DIR}/doc/en/documentation-en
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/Documentation/es/documentation-es
            ${PROJECT_BINARY_DIR}/doc/es/documentation-es
)

# Build documentation as small HTML files ("chunked html")
set(OUTPUT_CHUNKEDHTML_DE ${CMAKE_BINARY_DIR}/chunkedhtml/de)
set(OUTPUT_CHUNKEDHTML_EN ${CMAKE_BINARY_DIR}/chunkedhtml/en)
set(OUTPUT_CHUNKEDHTML_ES ${CMAKE_BINARY_DIR}/chunkedhtml/es)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/chunkedhtml)

set(PANDOC_CHUNKEDOPTS --toc --toc-depth=2
                       --standalone
                       --css=../style.css
                       --template=Documentation/template.html
                       --write=chunkedhtml
                       --embed-resources
                       --split-level=1)
add_custom_command(
    OUTPUT ${OUTPUT_CHUNKEDHTML_DE}/index.html
    COMMAND pandoc ${PANDOC_CHUNKEDOPTS}
                   --resource-path=".:Documentation:Documentation/de:doc:doc/de"
                   --metadata-file=Documentation/de/metadata.yaml
                   --output=${OUTPUT_CHUNKEDHTML_DE} ${DOC_DE}
    DEPENDS ${DOC_DE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating German Chunked HTML documentation"
    VERBATIM
)

add_custom_command(
    OUTPUT ${OUTPUT_CHUNKEDHTML_EN}/index.html
    COMMAND pandoc ${PANDOC_CHUNKEDOPTS}
                   --resource-path=".:Documentation:Documentation/en:doc:doc/en"
                   --metadata-file=Documentation/en/metadata.yaml
                   --output=${OUTPUT_CHUNKEDHTML_EN} ${DOC_EN}
    DEPENDS ${DOC_EN}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating English Chunked HTML documentation"
    VERBATIM
)

add_custom_command(
    OUTPUT ${OUTPUT_CHUNKEDHTML_ES}/index.html
    COMMAND pandoc ${PANDOC_CHUNKEDOPTS}
                   --resource-path="doc/en:Documentation/en:.:Documentation:Documentation/es:doc:doc/es"
                   --metadata-file=Documentation/es/metadata.yaml
                   --output=${OUTPUT_CHUNKEDHTML_ES} ${DOC_ES}
    DEPENDS ${DOC_ES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating Spanish Chunked HTML documentation"
    VERBATIM
)

add_custom_target(chunked-doc
    DEPENDS ${OUTPUT_CHUNKEDHTML_DE}/index.html ${OUTPUT_CHUNKEDHTML_EN}/index.html ${OUTPUT_CHUNKEDHTML_ES}/index.html
)

foreach(DOCDATA ${DOCDATA})
    add_custom_command(TARGET chunked-doc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${DOCDATA}
            ${PROJECT_BINARY_DIR}/chunkedhtml
    )
endforeach()

include(CTest)
add_test(NAME pytest COMMAND pytest -vv ${CMAKE_SOURCE_DIR}/tests)
