Steam switcher is a program intended to allow multiple instances of steam to operate simaltaineously, it works by changing information in a shared memory page that causes steams IPC calls to be routed to whichever variant of steam the user chooses. In order for this program to do anything multiple instances of steam must be running, which requires a slightly modified version of steam (only 1 byte must be modified)

Before you use steamswitcher you must make multisteam.exe.

----

HOW TO MAKE multisteam.exe

get a copy of steam.exe
open steam.exe in a hex editor
use the editors search function to find the following bytes:
83 C4 04 84 C0 74 14 6A 00
after that look for the first byte with values between 70 and 7F (a byte that starts with 7) following the pattern you just found. Change that byte to EB
save as multisteam.exe

This method may be invalidated in the future, I do not intend to support this project any longer, nor do I intend to provide altered copies of steam.exe and potentially infringe on valves copyright.

Instructions:
Start multiple versions of steam using multisteam.exe
when you want to switch to a specific process of steam, highlight the process ID for that steam and click set. This will suspend all other processes named steam.exe or multisteam.exe
when you want all copies of steam processes to not be suspended, click runall.
if you want to quickly terminate a copy of steam.exe or multisteam.exe, highlight that processes PID and click kill
if you want to refresh the process list, click refresh (there is an autorefresh function that automatically refreshes the list every second, so this shouldn't be necessary)