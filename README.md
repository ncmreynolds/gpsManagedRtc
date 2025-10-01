# gpsManagedRtc
An Arduino library to provide a GPS managed system time with backup to a hardware RTC on ESP32.

This is very much a bit of niche stuff for my own personal projects and not intended for general use. Management of time on the ESP32 beyond "set up NTP and forget it" can be a bit messy so this is trying to handle that. It's specifically intended for when there is no Wi-Fi so you can't rely on NTP.

**Beware this currently has a memory leak that needs investigating.**
