Tinker Hybrid Kernel
===================

Hybrid Kernel written in C and assembly.

Supported targets:
# PowerPC
- gdb simulator (aka PSIM)

# ARM
- Raspberry Pi

In development:

Licence
=======

GPL v3

Overview
========

I've worked on & off on this hybrid kernel for the last few years to help my understanding in operating systems.

Tinker is:
* Limited - it's an RTOS Kernel so that's the idea
* In development
* Single-core at present
* Written from a clean slate (excluding bits of boot-code from sel4 and osdev) from trail and error. Mostly error.

He is my hobby. The aim is to keep it simple enough for anyone to understand and therefore also
be simple enough to port and possibly even verify. I used to try and read the Linux Kernel but got lost. The aim of Tinker is to be easy to read so it's easy to learn what's going on.

Features
========

* Processes & Threads with a priority based pre-emptive scheduler
* Semaphores
* Shared Memory
* Pipes
* Timers
* Clock

Building
========

Gradle is used as the build system. It can build debug and release versions of either the parts
(which are static libraries) or executables

For example, to build an individual target use (in this case a Raspberry Pi kernel.img file):

    # Windows
    gradlew debugPiBinary releasePiBinary
    
    # Linux
    ./gradlew debugPiBinary releasePiBinary
    
To build everything (all targets, debug and release) use:

	# Windows
	gradlew clean assemble
	
	# Linux
	./gradlew clean assemble

Loading
=======

You can place user-code in tinker.c for now but eventually there'll be a bootstrap that contains
the hybridkernel and the required user-services to startup. The bootstrap will place the kernel
and services into memory, load the kernel and then start the user-services (much like init).

 bootstrap
  - kernel >\
  - (setup) |
  - service<| (i.e. filesystem)
  - service<| (i.e. block access for sata)
  - service<| (i.e. tcp/ip stack)
  - service</ (i.e. hardware device driver)
  
The intention is that the kernel is started by firmware or bootloader like u-boot.

Drivers
=======

Drivers and written as userland services with MMIO through the MMU.

The kernel Board Support Package (BSP) should only have drivers for timers and a debugging port such as a UART.

The root Interrupt Handler should be part of the kernel, other ones should really be installed as
user processes/servers that listen to the pipe.

Issues / TODO
=============

These are the things I need to address in a rough order:

* Target: ARM support (Raspberry Pi)
	* Add ARM MMU support
* Kernel: Don't map the whole process memory pool to the user-space in the MMU
	* Just map the entries you need (code, data, shm etc)
	* Stops the process having access to it's own internal data structures
* Kernel: Allow user-mode to map to real addresses (user-mode drivers)
    * (interrupts can be handled by pipes in the bsp)
* Kernel: Break up the syscalls into separate static/inline functions
* Support: Create newlib port
* Kernel: Add mechanism to load elfs
* Support: Create bootstrap for kernel + services
* Kernel: Add DMA support for pipes
* Kernel: Timeouts on pipe (open/read/write)
* Doc: Doc it with doxygen

Toolchain
=========

A standard stage 1 gcc compiler will work (i.e. C compiler without libc support).

Helpful Commands
================

Starting QEMU for the Raspberry Pi build

	# Disassemble the Raspberry Pi build so we can look at addresses
	arm-eabi-objdump -dS build\binaries\armRaspPiExecutable\debug\armRaspPi.exe > dis.txt
	
	# Start a listening netcat port to listen to the UART
	nc -L -p 5555 -vv
	# Start the emulator
	qemu-system-arm -m 512M -kernel build\binaries\armRaspPiExecutable\debug\armRaspPi.exe -gdb tcp::1234,ipv4 -no-reboot -no-shutdown -machine raspi -serial tcp:127.0.0.1:5555 -S

	# Start a debugger
	arm-eabi-gdb build\binaries\armRaspPiExecutable\debug\armRaspPi.exe
	# Connect to the debugger
	target remote localhost:1234
	# Set a breakpoint at the start
	b kernel_main
	# Start the kernel
	continue

# Concepts
Below is a description of the core concepts of the kernel and the rational of any design decisions.
## Locking
Locking is absent as it's a single-core kernel and pre-emption in syscalls/interrupts is disabled. In the future this will change but for now it keeps it simple and small.
## Process & Threads
Proceses are used as the container that stores everything related to the process - threads, objects and memory etc. As we're aiming for real-time, when a process creates a new process, any memory for the new process is allocated from the parent process. The exclusion for this is the processes initialised from the kmain() code where the process's memory is taken from the main pool. This avoids problems with possible starvation. Later on, with an ELF loader, it's possible that any new process could also be loaded from the main pool.
## Pipes (messaging inc. interrupts)
Pipes are used to send messages between processes and can register as interrupt handlers. They can be created as senders, receivers or both. Their width (message size) and depth (message count) are predefined. When multipled together, and aligned, it equals the amount of memory requred.

Once a message has been read by the application, it must be acknowledged. This is because the read gets a pointer to the buffer and the acknowledment frees that memory so another can write into it.

There's a built in Pipe in the kernel called 'in' which can be used to read characters from the character input device (like a UART or keyboard).
## Semaphores
Nothing special here. Support priority inheritance.
## Shared Memory
Nothing special here.
## Timers
Nothing special here.
## Objects (the registry)
The register stores objects by name so they can be looked up by different processes.
