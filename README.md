Steam switcher is a program intended to allow multiple instances of steam to operate simaltaineously, it works by changing information in a shared memory page that causes steams IPC calls to be routed to whichever variant of steam the user chooses. In order for this program to do anything multiple instances of steam must be running, which requires a slightly modified version of steam (only 1 byte must be modified)

Before you use steamswitcher you must make multisteam.exe.

----

HOW TO MAKE multisteam.exe

get a copy of steam.exe

open steam.exe in a hex editor

use the editors search function to find the following bytes:

83 C4 04 84 C0 74 14 6A 00

Look for the first byte after the sequence of bytes you just found that has a value between 70 and 7F (a byte that starts with 7). Change that byte to EB

save as multisteam.exe

This method works as of 7/30/22 but may be invalidated in the future, I do not intend to support this project any longer, nor do I intend to provide altered copies of steam.exe and potentially infringe on valves copyright.

----

Instructions for steamswitcher:

Start multiple versions of steam using multisteam.exe

When you want to switch to a specific steam process, highlight the process ID for that steam and click set. This will suspend all other processes named steam.exe or multisteam.exe

When you want all copies of steams processes to not be suspended, click runall.

If you want to quickly terminate a copy of steam.exe or multisteam.exe, highlight that processes PID and click kill

If you want to refresh the process list, click refresh (there is an autorefresh function that automatically refreshes the list every second, so this shouldn't be necessary)