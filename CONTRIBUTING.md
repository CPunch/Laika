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

- Implement `lib/win/winpersist.c`
- Fix address sanitizer for CMake DEBUG builds

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

## Lib: Task Service
Tasks can be scheduled on a delta-period (call X function every approximate N seconds). laikaT_pollTasks() is used to check & run any currently queued tasks. This is useful for sending keep-alive packets, polling shell pipes, or other repeatably scheduled tasks. Most laikaT_pollTasks() calls are done in the peerHandler for each client/server.

## Bot: Platform-specific backends

`bot/win` and `bot/lin` include code for platform-specific code that can't be quickly "ifdef"d away. These mainly include stuff like persistence or opening pseudo-ttys.