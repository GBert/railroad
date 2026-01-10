# Configuration of Accessories
On the main screen one can open the configuration of the accesdories with the icon ![](../menu_accessory.png).

## Basic data
TODO FIGURE (Figure on railcontrol.org seems false, as it shows a switch configuration)

### Name
Every accessory requires a unique name. If there is no name RailControl chooses a name for you and if the name is not unique it will be prefixed with a number to make it unique.
 
### Control
If more than one control is configured by RailControl, the control that controls the accessory has to be selected. Otherwise the selection is not shown at all. If there is only one control configured the field is not visible.

### Protocol
If the control supports more than one digital protocol, the protocol that is used by the accessory has to be selected. If the control only supports one protocol the field is not visible.

### Address
The digital address that is used by the accessory has to be entered.
 
### Duration (ms)
The accessories have to be turned on and after the effective switching turned off again. New accessories can switch within 100ms. Older and inert accessories require 250ms to switch. Some controls handle the switching time itself, so 0ms can be choosen. Dependent on the accessory type turning off is not needed, especially servo and motordrivers and corresponding decoders do not need it.

### Inverted
If an accessory is connected inverted to the decoder, RailControl can invert them virtually again.

## Position
TODO FIGURE (Figure on railcontrol.org seems false, as it shows a switch postion)

### Position X
The position of the element in squares from the left of the track diagram. Counting starts at zero. If an element is bigger then one square the square at the top left is relevant for the counting.

### Position Y
The position of the element in squares from the top of the track diagram. Counting starts at zero. If an element is bigger then one square the square at the top left is relevant for the counting.

### Layer
The layer the element should be visible on.

### Rotation
The elements can be rotated in steps of 90 degrees.

