### PicoAC - a small, but very fierce (not really) user mode anti-cheat client component for Windows 64-bit apps

It turns out I have terribly poor ideas for thesis projects.

### Features

- Utilized thread pool for spreading out work
- Thread pool workers added in main thread, some things will run in main thread as well
- Compatible with Counter-Strike 2 (should work both with Vulkan and DX11 render systems) and should be compatible (theoretically, as long as you update the function AOB signature or switch to a different address resolver method) with Rust
- Currently bootstrapped by basic LoadLibrary injector along with a hook on the low latency sleep function of the respective games
- Detection vectors include 
    - An integrity checker based on analyzing exception directory functions and relocation checks
    - Integrity check will trace detour hooks to their destination using [Zydis](https://github.com/zyantific/zydis) as a disassembler, logging the destination memory location and module
    - Grabbing thread contexts every now and then and analyzing their stacks for executable memory addresses
    - Process working set checks to catch manually mapped modules
    - Handle, process and window enumeration
    - Process working set watch introspection
    - Thread creation detection via instrumentation callbacks (other callback-based detection vectors in progress/planned)
    - (CS2-specific) Walking the interface list in modules exporting `CreateInterface`
- System integrity verification includes checking for Secure Boot, proper code integrity settings and checking for known bad certificate drivers via `WinVerifyTrust` and a build time-generated vulnerable driver hash list
- Prioritization for ease of integration and development
- Not particularly tailored to any game for the sake of easy integration
- Written in modern-ish C++ and uses XMake for package management and building

### Planned features - internal detection

- Making fallbacks for the integrity check in case of exception information missing
- IAT hook detection
- Grabbing thread contexts via APC
- Additional performance optimization

### Planned features - external detection

- I don't believe there's much more to be done to deter external hacks from user mode save for making it a pain to actually do anything with the read memory (some simple and fast encryption of important values like view matrix, camera position and camera angles that changes the key every frame?), you'll need to bring a gun (kernel mode)

### Planned features - environment integrity

- Dump Windows Measured Boot logs

### Planned features - integration
- (maybe) Deadlock support

### Not planned

- Kernel mode component
- Backend component
- Proper bootstrapper
- Linux support
- x86 support

### Problems

- It's heavily unfinished
- High reliance on third party libraries (wil, spdlog, ...)
- No defenses against kernel mode - once an adversary is in there, we can only pray they're merciful and don't hook every syscall from the get-go with their evil hypervisor to tell us what we want to hear
- No networking/heartbeat components
- Protection can be completely broken by removing the low latency sleep detour

### Integration

- Call export function `Pico_Init()` once during initialization, then call `Pico_PollMainLoop()` every frame in the main thread
- Will likely need more steps to guard against shadow VMT hooks and the like

### Credits

- Arttu M. - big hints for architecture and features, as well as reading material
- [psiberx](https://github.com/psiberx) - uses his memory utilities and detour library
- [TiltedPhoques team](https://github.com/tiltedphoques) - [TiltedCore](https://github.com/tiltedphoques/TiltedCore)
- [LinuxPE](https://github.com/can1357/linux-pe)
- https://github.com/winsiderss - various Windows structs
- [Geoff Chappell](https://www.geoffchappell.com/index.htm) (RIP)
