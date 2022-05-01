# Laika

<p align="center">
    <a href="https://github.com/CPunch/Laika/actions/workflows/check-build.yaml"><img src="https://github.com/CPunch/Laika/actions/workflows/check-build.yaml/badge.svg" alt="Workflow"></a>
    <a href="https://github.com/CPunch/Laika/blob/main/LICENSE.md"><img src="https://img.shields.io/github/license/CPunch/Laika" alt="License"></a>
</p>

[![asciicast](https://asciinema.org/a/487686.svg)](https://asciinema.org/a/487686)

Laika is a simple cross-platform Remote Access Toolkit stack for educational purposes. It allows encrypted communication across a custom binary protocol. The bot client supports both Windows & Linux environments, while the shell & CNC server specifically target Linux environments. Laika is meant to be small and discreet, Laika believes in hiding in plain sight.

Some notable features thus far:
- [X] Lightweight, the bot alone is 183kb (`MinSizeRel`) and uses very little resources minimizing Laika's footprint.
- [X] Authentication & packet encryption using LibSodium and a predetermined public CNC key. (generated with `bin/genKey`)
- [X] Server and Shell configuration through `.ini` files.
- [X] Ability to open shells remotely on the victim's machine.
- [X] Persistence across reboot: (toggled with `-DLAIKA_PERSISTENCE=On`)
    - [X] Persistence via Cron on Linux-based systems.
    - [X] Persistence via Windows Registry.
- [ ] Ability to relay socket connections to/from the victim's machine.
- [ ] Uses obfuscation techniques also seen in the wild (string obfuscation, tiny VMs executing sensitive operations, etc.)
- [ ] Simple configuration using CMake:
    - [X] Setting keypairs (`-DLAIKA_PUBKEY=? -DLAIKA_PRIVKEY=?`, etc.)
    - [ ] Obfuscation modes

## Why?

Most public malware sources in the wild are nerf'd or poorly made. Laika is written in modern C, and strives to adhere to best practices while keeping a maintainable and readable code base. The reader is encouraged to compile a `MinSizeRel` build of Laika and open it up in their favorite disassembler. Take a look at how certain functions or subroutines look compared to its plaintext source. See if you can dump strings during runtime with a debugger, try to break Laika. Play both sides by breaking Laika, and improving it to make reversing and analysis harder. Most malware depend on the time that it takes to analyze a sample, this gives their malware time to do whatever before eventually being shutdown. Playing both sides will help give you insight into the methods and bitterness that is this cat and mouse game.

## Would this work in real world scenarios?

My hope is that this becomes complete enough to be accurate to real RAT sources seen in the wild. However since Laika uses a binary protocol, the traffic the bot/CNC create would look very suspect and scream to sysadmins. This is why most RATs/botnets nowadays use an HTTP-based protocol, not only to 'blend in' with traffic, but it also scales well with large networks of bots where the CNC can be deployed across multiple servers and have a generic HTTP load balancer.

I could add some padding to each packet to make it look pseudo-HTTP-like, however I haven't given much thought to this.

## CMake Definitions

| Definition        | Description                           | Example                                                                           |
| ----------------- | ------------------------------------- | --------------------------------------------------------------------------------- |
| LAIKA_PUBKEY      | Sets CNC's public key                 | -DLAIKA_PUBKEY=997d026d1c65deb6c30468525132be4ea44116d6f194c142347b67ee73d18814   |
| LAIKA_PRIVKEY     | Sets CNC's private key                | -DLAIKA_PRIVKEY=1dbd33962f1e170d1e745c6d3e19175049b5616822fac2fa3535d7477957a841  |
| LAIKA_CNC_IP      | Sets CNC's public ip                  | -DLAIKA_CNC_IP=127.0.0.1                                                          |
| LAIKA_CNC_PORT    | Sets CNC's bind()'d port              | -DLAIKA_CNC_PORT=13337                                                            |
| LAIKA_PERSISTENCE | Enables persistence for LaikaBot      | -DLAIKA_PERSISTENCE=On                                                            |
> examples are passed to `cmake -B <dir>`

## Configuration and compilation

Make sure you have the following libraries and tools installed:
- CMake (>=3.10)
- Compiler with C11 support (GCC >= 4.7, Clang >= 3.1, etc.)

The only dependency (LibSodium) is vender'd and statically compiled against the `/lib`. This should be kept up-to-date against stable and security related updates to LibSodium.

First, compile the target normally

```sh
$ cmake -B build && cmake --build build
```

Now, generate your custom key pair using `genKey`

```sh
$ ./bin/genKey
```

Next, rerun cmake, but passing your public and private keypairs

```sh
$ rm -rf bin build &&\
    cmake -B build -DLAIKA_PUBKEY=997d026d1c65deb6c30468525132be4ea44116d6f194c142347b67ee73d18814 -DLAIKA_PRIVKEY=1dbd33962f1e170d1e745c6d3e19175049b5616822fac2fa3535d7477957a841 -DCMAKE_BUILD_TYPE=MinSizeRel &&\
    cmake --build build
```

Output binaries are put in the `./bin` folder

## Looking to contribute?

Read `CONTRIBUTING.md`

# Ansible-Playbook

To setup a test VPS for a Laika CNC, check out [this ansible playbook](https://github.com/CPunch/Laika-Playbook).
