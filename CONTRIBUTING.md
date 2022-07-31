# CONTRIBUTING to Laika
HEAD: https://github.com/CPunch/Laika/tree/main

## Directories explained
- `/lib` is a shared static library between the bot, shell & CNC. LibSodium is also vendor'd here.
- `/cnc` is the Command aNd Control server. (Currently only targets Linux)
- `/bot` is the bot client to be ran on the target machine. (Targets both Linux and Windows)
- `/shell` is the main shell to connect to the CNC server with to issue commands. (Currently only targets Linux)
- `/tools` holds tools for generating keypairs, etc.

## Coding style
Laika uses clang-format to enforce a consistent style across the codebase. Before committing changes, make sure to run `git clang-format` and add the changes to the commit. Pull requests with wrong styling will be rejected.

## Tasks and TODOs
Looking for some simple tasks that need to get done for that sweet 'contributor' cred? Check here!

- Change `lib/lin/linshell.c` to use openpty() instead of forkpty() for BSD support
- Fix address sanitizer for CMake DEBUG builds
- Change laikaT_getTime in `lib/src/ltask.c` to not use C11 features
- Implement more LAIKA_BOX_* VMs in `lib/include/lbox.h`
- Import more WinAPI manually using the method listed below

## Bot: Windows API Imports Obfuscation
Laika uses the fairly common technique of importing several API functions during runtime to help lower AV detection rates. In short, this just removes our library function from our IAT (Import Address Table), making it harder for AV to know what APIs we're loading and using. The logic for importing API is in `lib/win/winobf.c`. To add another API to our import list, first make the function typedef & function pointer definition in `lib/include/obf.h`, for example:

```C
typedef HINSTANCE(WINAPI *_ShellExecuteA)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);

extern _ShellExecuteA oShellExecuteA;
```
> The naming convention for the typedef is an underscore '_' and then the WinAPI import name; for the identifier it's 'o' (for obfuscated) and then the WinAPI import name

Next, define the function pointer in `bot/win/winobf.c`, right before the `laikaO_init()` function.

```C
_ShellExecuteA oShellExecuteA;
```

Now to dump the calculated hash (and to check and make sure there's no collisions) feel free to use this tiny code stub and just run a debug build of Laika.

```C
    uint32_t _hash = getHashName("ShellExecuteA");
    printf("ShellExecuteA: real is %p, hashed is %p. [HASH: %x]\n",
           (void *)ShellExecuteA,
           findByHash("shell32.dll", _hash), _hash);
```
> I usually just insert this at the end of `laikaO_init()`, although it doesn't really matter since we're just dumping the hash and making sure it works properly.
> NOTE: To find out what API is imported from what library, just look at the executable's import table using a tool like [PE-explorer](http://www.pe-explorer.com/)

You'll see output like so:

```
ShellExecuteA: real is 00007FFC6A71E780, hashed is 00007FFC6A71E780. [HASH: 89858cd3]
```

If the `real` & `hashed` match, that means our manual runtime import and the import from the windows loader matched the same function! So it worked fine. Next we'll take the `HASH` from that output and plug it into a call to `findByHash()`. After removing that previous code stub our end result should look something like:

```C
    oShellExecuteA = (_ShellExecuteA)findByHash("shell32.dll", 0x89858cd3);
```
> Again, just put yours next to the others in `laikaO_init()`

Now just replace all of the calls to the raw WinAPI (in our case, ShellExecuteA) with our new manually imported oShellExecuteA function pointer. Format & commit your changes, and open a PR and I'll merge your changes. Thanks!

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
Laika has a tiny VM for decrypting sensitive information. For details on the ISA read `lib/include/lvm.h`, for information on how to use them read `lib/include/lbox.h`. Feel free to write your own boxes and contribute them :D

## Bot: Platform-specific backends

`bot/win` and `bot/lin` include code for platform-specific code that can't be quickly "ifdef"d away. These mainly include stuff like persistence or opening pseudo-ttys.