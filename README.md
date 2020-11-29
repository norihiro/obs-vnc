# OBS VNC Source

## Introduction

This plugin is a VNC viewer that works as a source in OBS Studio.

## Preparing a VNC Server
### Windows
If you are to view Windows desktop by this plugin, these two VNC server applications are my recommendation.
- [TightVNC](https://www.tightvnc.com/download.php)
- [UltraVNC](https://www.uvnc.com/downloads/ultravnc.html)

### MacOS
MacOS has builtin VNC server as Screen Sharing service.

### Multi-display
The generic VNC protocol does not have an option to select display.
Most VNC servers send whole desktop.

As a workaround of multi-display, the plugin provides settings `Skip update (left/right/top/bottom)`.
Still the plugin receives the picture from the VNC server,
these settings will discard an update-notification that goes to the graphics engine of OBS-Studio if all the updated region from the VNC-server is in the range of the values provided by these settings.
It is recommended to set the same values to the Crop settings in Scene Item Transform of your source.

## Furture plan

* configurations for hide cursor, color-depth, user-name, etc.,
* and other items if requested.
