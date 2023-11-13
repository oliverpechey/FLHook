FLHook log files are created in `.\EXE\flhook_logs`. The following logs are created:

* `flhook.log`: General debug log.
* `flhook_kicks.log`: Logs all character kicks and their cause (e.g. command, idle, ping, packetloss, corrupted character file, IP ban, host ban).
* `flhook_cheaters.log`: Cheat detection logging.
* `flhook_connects.log`: Every login attempt, if enabled.
* `flhook_usercmds.log`: Every user command, if enabled.
* `flhook_admincmds.log`: Every admin command, if enabled.
* `flhook_socketcmds.log`: Every socket command, if enabled.
* `flhook_perftimers.log`: Performance timer reports, if enabled.

If `debug` is enabled in the INI config, timestamped debug log files will be created in  `.\EXE\flhook_logs\debug`.