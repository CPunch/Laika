# CONTRIBUTING to Laika
HEAD: https://github.com/CPunch/Laika/tree/main

## Directories explained
- `/cmake-modules` holds helper functions for CMake.
- `/lib` is a shared static library between the bot, shell & CNC. LibSodium is also vendor'd here.
- `/cnc` is the Command aNd Control server. (Currently only targets Linux)
- `/bot` is the bot client to be ran on the target machine. (Targets both Linux and Windows)
- `/shell` is the main shell to connect to the CNC server with to issue commands. (Currently only targets Linux)
- `/tools` holds tools for generating keypairs, etc.

## Tasks and TODOs
Looking for some simple tasks that need to get done for that sweet 'contributor' cred? Check here!

- Change `lib/lin/linshell.c` to use openpty() instead of forkpty() for BSD support
- Fix address sanitizer for CMake DEBUG builds
- Change laikaT_getTime in `lib/src/ltask.c` to not use C11 features.
- Implement more LAIKA_BOX_* VMs in `lib/include/lbox.h`

## Lib: Error Handling
Error handling in Laika is done via the 'lerror.h' header library. It's a small and simple error handling solution written for laika, however can be stripped and used as a simple error handling library. Error handling in Laika is used similarly to other languages, implementing a try & catch block and is achieved using setjmp(). The LAIKA_ERROR(...) is used to throw errors.

Example:
```C 
LAIKA_TRY
    printf("Ran first\n");
    LAIKA_ERROR("Debug message here\n");
    printf("You'll never see this\n");
LAIKA_CATCH
    printf("Ran second!\n");
LAIKA_TRYEND

printf("Ran last!\n");
```

Some minor inconveniences include:
- `return` or other control-flow statements that leave the current scope cannot be used in the LAIKA_TRY or LAIKA_CATCH scopes.
- max of 32 depth, avoid using recursively.
- not thread safe.

## Lib: Packet Handlers

Laika has a simple binary protocol & a small backend (see `lib/src/lpeer.c`) to handle packets to/from peers. `lib/include/lpacket.h` includes descriptions for each packet type. For an example of proper packet handler definitions see `bot/src/bot.c`. It boils down to passing a sLaika_peerPacketInfo table to laikaS_newPeer. To add packet handlers to the bot, add your handler info to laikaB_pktTbl in `bot/src/bot.c`. To add packet handlers to the shell, add your handler info to shellC_pktTbl in `shell/src/sclient.c`. For adding packet handlers to cnc, make sure you add them to the corresponding table in `cnc/src/cnc.c`, laikaC_botPktTbl for packets being received from a bot peer, laikaC_authPktTbl for packets being received from an auth peer (shell), or DEFAULT_PKT_TBL if it's received by all peer types (things like handshakes, keep-alive, etc.)

## Lib: Task Service
Tasks can be scheduled on a delta-period (call X function every approximate N seconds). laikaT_pollTasks() is used to check & run any currently queued tasks. This is useful for sending keep-alive packets, polling shell pipes, or other repeatably scheduled tasks. Most laikaT_pollTasks() calls are done in the peerHandler for each client/server.

## Lib: VM Boxes
Laika has a tiny VM for decrypting sensitive information (currently unused, but functional). For details on the ISA read `lib/include/lvm.h`, for information on how to use them read `lib/include/lbox.h`. Feel free to write your own boxes and contribute them :D

## Bot: Platform-specific backends

`bot/win` and `bot/lin` include code for platform-specific code that can't be quickly "ifdef"d away. These mainly include stuff like persistence or opening pseudo-ttys.