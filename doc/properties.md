## VNC Source Properties

### Host name
Host name or IP address for VNC server.

### Host port
Port number for VNC server.
Default is 5900.

### User name
Specifies user name for SASL authentication.
This field is available if the plugin is built with libvncserver having SASL.

### Password
Password for VNC server.
Plain password is stored in the setting file.

### Connection timeout
(Windows only)
Specifies the timeout to connect to the VNC server.

### Color level
Select the color level to use on the link.
Default is 24-bit and available settings are 24-bit, 16-bit, 8-bit.
For VNC-server on macOS, use 24-bit. Otherwise, the connection will fail.

### Preferred encodings
This option specifies the preferred encoding to use.
Default is `Auto`, which will select the encoding from the available list received from the server.
Available settings are `Auto`, `Tight`, `ZRLE`, `Ultra`, `Hextile`, `ZLib`, `CoRRE`, `RRE`, and `Raw`.

### Compress level
Use specified lossless compression level. 0 = poor, 9 = best. Default is 3.

### Enable JPEG
Enable lossy JPEG compression.
Default is enabled.

### Quality level for JPEG (0=poor, 9=best)
Quality level for JPEG. 0 = poor, 9 = best. Default is 5.

### QoS DSCP
Specifies the QoS IP DSCP for this client.
Default is 0.

### Connection timing
Specifies when to connect to the server and when to disconnect from the server.
- Always: When OBS started and the source is added, the connection attempts will start.
- Connect at shown: When the source is shown on at least one of Program, Preview, or Projector view, the connection attempts will start.
- Connect at activated: When the source appears on the Program, the connection attempts will start.
  Since the connection will take time, blank or old frame will be shown until the connection is established.
- Disconnect at hidden: When the source is not shown on any of Program, Preview, or Projector view, the connection will be disconnected.
- Disconnect at inactive: When the source is not shown on the Program, the connection will be disconnected.
The default is Always.

### Clear texture at disconnection
If enabled, the texture will be cleared when the VNC connection is disconnected.
If disabled, the texture will remain unchanged.

### Skip update (left, right, top, bottom)
This property requests the server to send only inside the specified area.
It will help to reduce the amount of data transferred from the VNC server.
It is recommended to set the same values to the Crop settings in Scene Item Transform of your source.
