Smartcard Sniffer
=================

Wrote this as I was using [APDUView]( http://www.fernandes.org/apduview/index.html ) utility quite a lot, but it's sometimes complicated to set up. This tool uses AppInit_Dlls functionality to achieve the same with more ease. It hooks `winscard!SCardTransmit` and logs both sent and received data to a log file.
Log file is located in the same directory as the `SmartcardSniffer.dll`, and has 
a name of the application that is talking to the smart card. 

To install the dll, just add it's path to:
* for 32 bit DLL on 32 bit systems:
 - ```[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows\AppInit_DLLs]```
* for 64 bit DLL on 64 bit system:
 - ```[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows\AppInit_DLLs]```
* for 32 bit DLL on 64 bit system (for hooking 32 bit applications on 64 bit systems):
 - ```[HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Windows\AppInit_DLLs]```
 
Multiple entries are separated by space and/or ```,```. Make sure you path to the DLL doesn't have any spaces in it.
 
Note that on Windows Vista and later, you'd also need to set ```RequireSignedAppInit_DLLs``` key to 0
to be able to load unsigned DLLs and make sure ```LoadAppInit_DLLs``` is set to 1.
 
To uninstall it, simply remove the DLL entry from AppInit_DLLs list.
   
In the log file ```>>>``` specifies the data to be sent, and ```<<<``` received data.
Here's the example of the logged data:
```     Winscard!SCardTransmit:
        >>> 00:A4:04:00:0B:A0:00:00:03:97:43:49:44:5F:01:00
        <<< 6A:82
        Winscard!SCardTransmit:
        >>> 00:CA:7F:68:00
        <<< 7F:60:1F:83:A1:21:06:06:60:81:4B:01:65:03:A1:14:81:01:08:82:11:08:87:02:00:1A:88:02:08:04:B1:04:B1:02:14:3C:90:00
        Winscard!SCardTransmit:
        >>> 00:A4:04:00:09:A0:00:00:03:08:00:00:10:00
        <<< 6A:82
```

 - ea
