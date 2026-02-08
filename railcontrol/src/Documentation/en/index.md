# Download
RailControl can be downloaded on the [Download](https://www.railcontrol.org/index.php/en/download-en)-page.

## Installation
The downloaded archive can be extracted everywhere on the computer.

### Installing on Windows
On Windows the archive has to be extracted in a subdirectory. For example D:\\ is not allowed, D:\\ModelRailway\\ is OK.

### Installing on Debian GNU/Linux

Since Debian GNU/Linux 13 "trixie" RailControl is included in Debian. To
install, execute in a terminal

```
sudo apt install railcontrol
```

**Note:** Debian-specific documentation is located in the directory
`/usr/share/doc/railcontrol`.

## Configuration-File
In the extracted archive there is a template of the configuration file (railcontrol.conf.dist). At first start of RailControl it will be copied to railcontrol.conf. Usually it is not needed to change any settings in the config file.

## Starting RailControl
RailControl can be started with a double click on the executable file railcontrol.exe (Windows) respectively railcontrol (other operation systems). To get logging informations from RailControl it is recommended to start RailControl from a console. Also possible is to change the behavior of RailControl with [startup arguments](#startup-arguments).

## Shutdown RailControl
RailControl should be terminated by entering q+Enter or Ctrl+C in the RailControl-terminal or by using the top left button in the browser.

**Important: If the X in the top right of the RailControl-terminal is used some settings are not stored correctly and problems can apear when starting RailControl again.**

Entering q+Enter or Ctrl+C in the terminal or using the terminate-button in the browser multiple times will terminate RailControl early. This should only be used if RailControl does not shutdown cleanly.

## Browser
If RailControl already has started one can connect with a recent Browser. RailControl lists the possible links that can be used in the browser. One of these links can be copied into the browser. Links with localhost, 127.0.0.1 and [::1] only work on the device where RailControl is executed. The other links also work on other devices.

# Functions and Configuration
After connecting with the browser, the important functions and configurations can be reached in the menu bar at the top:

These are the functions from left to right:

![](../menu_quit.png "Button Quit")  
Terminating RailControl-server, including stopping all trains

![](../menu_booster.png "Button Booster")  
Turn on/off current on the track

![](../menu_stop.png "Button Stop")  
Stopping all trains immediately (speed zero)

![](../menu_signalred.png "Button Signal Red")  
Stopping all trains in automode at the next signal / at end of street

![](../menu_signalgreen.png "Button Signal Green")  
Putting all trains into automode

![](../menu_fullscreen.png "Button Full Screen")  
Show RailControl in full screen.

![](../menu_program.png "Button Program")  
CV-Programming. Is only displayed when the control and its API support it.

![](../menu_menu.png "Button Menu")  
On narrow screens the second part of the menu can be made visible.

![](../menu_settings.png "Button Settings")  
[General Settings](#general-settings)

![](../menu_control.png "Button Control")  
[Configuration of controls](#configuration-of-controls)

![](../menu_loco.png "Button Loco")  
[Configuration of locomotives](#configuration-of-locomotives)

![](../menu_multipleunit.png "Button Multiple Unit")  
[Configuration of Multiple Units](#configuration-of-multiple-units)

![](../menu_layer.png "Button Layer")  
[Configuration of layers](#configuration-of-layers)

![](../menu_track.png "Button Track")  
[Configuration of tracks](#configuration-of-tracks)

![](../menu_group.png "Button Track Group")  
[Configuration of trackgroups](#configuration-of-trackgroups)

![](../menu_switch.png "Button Switch")  
[Configuration of switches](#configuration-of-switches)

![](../menu_signal.png "Button Signal")  
[Configuration of signals](#configuration-of-signals)

![](../menu_accessory.png "Button Accessory")  
[Configuration of accessories](#configuration-of-accessories)

![](../menu_feedback.png "Button Feedback")  
[Configuration of feedbacks](#configuration-of-feedbacks)

![](../menu_route.png "Button Route")  
[Configuration of routes](#configuration-of-routes)

![](../menu_counter.png "Button Counter")  
[Configuration of counters](#configuration-of-counters)

![](../menu_text.png "Button Text")  
[Configuration of texts](#configuration-of-texts)

