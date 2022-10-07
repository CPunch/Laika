# Laika

<p align="center">
    <a href="https://github.com/CPunch/Laika/actions/workflows/check-build.yaml"><img src="https://github.com/CPunch/Laika/actions/workflows/check-build.yaml/badge.svg?branch=main" alt="Workflow"></a>
    <a href="https://github.com/CPunch/Laika/blob/main/LICENSE.md"><img src="https://img.shields.io/github/license/CPunch/Laika" alt="License"></a>
    <br>
    <a href="https://asciinema.org/a/499508" target="_blank"><img src="https://asciinema.org/a/499508.svg" /></a>
</p>

Laika is a simple cross-platform Remote Access Toolkit stack for educational purposes. It allows encrypted communication across a custom binary protocol. The bot client supports both Windows & Linux environments, while the shell & CNC server specifically target Linux environments. Laika is meant to be small and discreet, Laika believes in hiding in plain sight.

Some notable features thus far:
- [X] Lightweight, the bot alone is 183kb (`MinSizeRel`) and uses very little resources minimizing Laika's footprint.
- [X] Authentication & packet encryption using LibSodium and a predetermined public CNC key. (generated with `bin/genKey`)
- [X] CNC and Shell configuration through `.ini` files.
- [X] Open shells remotely on the victim machine.
- [X] Persistence across reboot: (toggled with `-DLAIKA_PERSISTENCE=On`)
    - [X] Persistence via Cron on Linux-based systems.
    - [X] Persistence via Windows Registry.
- [X] Uses obfuscation techniques also seen in the wild (string obfuscation, tiny VMs executing sensitive operations, etc.)
- [ ] Simple configuration using CMake:
    - [X] Setting keypairs (`-DLAIKA_CNC_IP=? -DLAIKA_CNC_PORT=?`, etc.)
    - [X] Enabling/Disabling Obfuscation (`-DLAIKA_OBFUSCATE=On`)
        - [ ] Obfuscation modes

## How do I use this?

Please refer to the [Wiki](https://github.com/CPunch/Laika/wiki) for any questions relating to deployment, compilation & setup.

## Looking to contribute?

Read `CONTRIBUTING.md`
