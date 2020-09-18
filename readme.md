# Zoner Universe fork of FLHook

## Credits

This repo is forked from the main FLHook repo maintained by FriendlyFire. Please see this repo for further documentation: https://github.com/fl-devs/FLHook

The majority of changes are intended to eventually be pushed to upstream.

* Initial FLHook development by mc_horst
* Versions 1.5.5 to 1.6.0 by w0dk4
* Versions 1.6.0 to 2.0.0 based on open-source SVN, supervised by w0dk4
* Versions 2.1.0 and later on Github, supervised by FriendlyFire

## Website and Server Information

**Website:** https://zoneruniverse.com/  
**Discord:** https://zoneruniverse.com/discord  
**Server Address:** server.zoneruniverse.com  
**Port:** 2302  

## Compiling the Project

1. Install Visual Studio 2019
2. Clone https://github.com/microsoft/vcpkg.git
3. Run bootstrap-vcpkg.bat
4. In this directory, run `.\vcpkg integrate install`
5. Clone this repository
6. Open `project\FLHook.sln`
7. Ensure you are on `Release` and build the solution.

## Installation
**N.B. FLHOOK ONLY WORKS WITH THE 1.1 PATCH INSTALLED. USING IT WITH 1.0 WILL CRASH FLSERVER!**

1. Copy all files from `dist\$Configuration` to your `.\EXE` directory. You may selectively remove some of the default plugins as desired.
2. Edit `.\EXE\FLHook.ini` as required.
3. Edit `.\EXE\dacomsrv.ini` and append `FLHook.dll` to the `[Libraries]` section.
4. FLServer should now load FLHook whenever you start it!