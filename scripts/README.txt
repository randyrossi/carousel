kill_switch.py: Emulator Kill/Shutdown Switch

This python script monitors a push button switch hooked up to GPIO18 
and GND pins on your raspberry pi's GPIO.

When your button is pressed, it will kill all the emulators on the system,
taking you back to the carousel immediately. I find this is a much better
user experience than having to navigating whatever menu system the emulator
provides to exit (i.e. AdvMame : hitting escape, moving up/down, pressing a
button).

I drilled a small hole in my arcade cabinet and installed a red
SPST push button.  If you use an arcade stick, you can avoid having to map
one of your buttons to the escape key (i.e. X-Arcade Tankstick).

Raspberry pi's should be shutdown properly rather than having the
power yanked out from underneath them.  If you hold down the push button for
3 seconds, it will shut down the system.

To automatically launch the script at startup, put the kill_switch.conf
file into /etc/supervisor/conf.d
