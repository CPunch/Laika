# Laika

Laika is a simple Remote Access Toolkit stack for red teaming. It allows authenticated communication across a custom protocol with generated key pairs which are embedded into the executable (only the public key is embedded in the bot client ofc). This project currently only targets Linux, however the socket API has already been ported to work under various OS including Windows, Linux & MacOS. Cross-platform support for the bot client will come eventually.

![DEMO](img/demo.gif)

Some notable features thus far:
- [X] Lightweight, the bot alone is 270kb (22kb if not statically linked with LibSodium) and uses very little resources.
- [ ] Uses obfuscation techniques also seen in the wild (string obfuscation, tiny VMs executing sensitive operations, etc.)
- [ ] Simple configuration using CMake
    - [X] Setting keypairs (`-DLAIKA_PUBKEY=? -DLAIKA_PRIVKEY=?`)
    - [ ] Obfuscation modes

## Would this work in real world scenarios?

My hope is that this becomes complete enough to be accurate to real RAT sources seen in the wild. However since Laika uses a binary protocol, the traffic the bot/CNC create would look very suspect and scream to sysadmins. This is why most RATs/botnets nowadays use an HTTP-based protocol, not only to 'blend in' with traffic, but it also scales well with large networks of bots where the CNC can be deployed across multiple servers and have a generic HTTP load balancer.

I could add some padding to each packet to make it look pseudo-HTTP-like, however I haven't given much thought to this.

## Directories explained

- `/cmake-modules` holds helper functions for finding things like libSodium.
- `/lib` is a shared static library between the client, peer & panel clients.
- `/cnc` is the Command aNd Control server.
- `/bot` is the bot client to be ran on the target machine.
- `/shell` is the main shell to connect to the CNC server with to issue commands.
- `/tools` holds tools for generating keypairs, etc.

## CMake Definitions

| Definition        | Description                           | Example                                                                           |
| ----------------- | ------------------------------------- | --------------------------------------------------------------------------------- |
| LAIKA_PUBKEY      | Sets CNC's public key                 | -DLAIKA_PUBKEY=997d026d1c65deb6c30468525132be4ea44116d6f194c142347b67ee73d18814   |
| LAIKA_PRIVKEY     | Sets CNC's private key                | -DLAIKA_PRIVKEY=1dbd33962f1e170d1e745c6d3e19175049b5616822fac2fa3535d7477957a841  |
| LAIKA_CNC_IP      | Sets CNC's public ip                  | -DLAIKA_CNC_IP=127.0.0.1                                                          |
| LAIKA_CNC_PORT    | Sets CNC's bind()'d port              | -DLAIKA_CNC_PORT=13337                                                            |
> examples are passed to `cmake -B <dir>`

## Configuration and compilation

Make sure you have the following libraries and tools installed:
- CMake (>=3.10)
- Compiler with C11 support (GCC >= 4.7, Clang >= 3.1, etc.)
- LibSodium (static library)

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
$ rm -rf build &&\
    cmake -B build -DLAIKA_PUBKEY=997d026d1c65deb6c30468525132be4ea44116d6f194c142347b67ee73d18814 -DLAIKA_PRIVKEY=1dbd33962f1e170d1e745c6d3e19175049b5616822fac2fa3535d7477957a841 -DCMAKE_BUILD_TYPE=MinSizeRel &&\
    cmake --build build
```

Output binaries are put in the `./bin` folder