Commands can be executed by administrators in several ways:
* Using the FLHook console (access to all commands)
* In game by typing `.command` in the chat(e.g. `.getcash Player1`). This will only work when you own the appropriate rights which may be set via the `setadmin` command. FLHook will store the rights of each player in their account directory in the file `flhookadmin.ini`.
* Via a socket connection in raw text mode (e.g. with putty). Connect to the port given in `.\EXE\FLHook.ini` and enter `PASS password`. After having successfully logged in, you may input commands as though from the console, provided the password is associated with the requisite permissions. You may have several socket connections at the same time. Exiting the connection may be done by entering "quit" or simply by closing it.

All commands return "OK" when successful or "ERR some text" when an error occurred. A full list of commands can be found with `.help`.

### Shorthand

There are four shorthand symbols recognized by FLHook to simplify the job of selecting a character for a command.

First, you can use client IDs instead of character names by appending `$` to the command: `getcash$ 12` will perform the command for the currently logged in player with client ID 12.

You can also use "shortcuts" instead of the whole character name for a currently logged in player by appending `&` to the cmd. FLHook traverses all logged in players and checks if their character name contains the substring you passed (case insensitive) and if so, the command will operate on this player. An error will be shown if the search string given is ambiguous. For instance, given two players logged in, Foobar and Foo, `.kick& bar` will kick Foobar, but `.kick& Foo` will fail because two players match.

Third, you can target yourself by appending `!` to the command. This obviously only works if you are executing the command as a logged in player. For instance, `.setcash! 999999` will set your own character's cash to $999,999.

Finally, you can target your currently selected target by appending `?` to the command. Again, this only works if you are logged in as a player, in space, and targeting *another player*. For instance, `.kill?` will destroy whoever you are targeting.

### Permissions
Permissions may be separated by a comma (e.g. `setadmin playerxy cash,kickban,msg`). The following permissions are available by default:
* `superadmin`  → Everything
* `cash`        → Cash commands
* `kickban`     → Kick/ban commands
* `beamkill`    → Beam/kill/resetrep/setrep command
* `msg`         → Message commands
* `cargo`       → Cargo commands
* `characters`  → Character editing/reading commands
* `reputation`  → Reputation commands
* `eventmode`   → Eventmode (only when connected via socket)
* `settings`    → Rehash and reserved slots (`setadmin` only with `superadmin` rights)
* `plugins`     → Plugin commands
* `other`       → All other commands
* `special1`    → Special commands #1
* `special2`    → Special commands #2
* `special3`    → Special commands #3
All other commands except `setadmin`/`getadmin`/`deladmin` may be executed by any admin.

### XML Text Reference

The `fmsg*` commands allow you to format text in several ways. Please see [XML Text Reference](md_docs__usage__x_m_l__text__reference.html) for more information.