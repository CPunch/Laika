# Laika

Laika is a simple botnet stack for red teaming. It allows authenticated communication across a custom protocol with generated key pairs which are embedded into the executable. 

Some notable features thus far:
- Lightweight, the bot alone is 80kb and uses very little resources.
- Uses obfuscation techniques also seen in the wild (string obfuscation, tiny VMs executing sensitive operations, etc.)
- Simple configuration using CMake (setting keys, obfuscation modes, etc.)

## Why 'Laika'?

During the soviet space race, [Laika](https://en.wikipedia.org/wiki/Laika) was the first dog in space; however shortly after died of asphyxiation and overheating of the shuttle. Take whatever you want from this information.

## Configuration and compilation

First, compile the target normally

```
cmake -B build && cmake --build build
```

Now, generate your custom key pair using `genKey`

```
./bin/genKey
```

Next, rerun cmake, but passing your public and private keypairs

```
rm -rf build && cmake -B build -DLAIKA_PUBKEY=997d026d1c65deb6c30468525132be4ea44116d6f194c142347b67ee73d18814 -DLAIKA_PRIVKEY=1dbd33962f1e170d1e745c6d3e19175049b5616822fac2fa3535d7477957a841 && cmake --build build
```
