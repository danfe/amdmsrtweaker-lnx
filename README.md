amdmsrt
=======

amdmsrt (amdmsrtweaker-lnx) is a Linux/FreeBSD port of the AmdMsrTweaker tool written by Martin Kinkelin and extended by Marcus Pollice.

It has been tested with a 14h (AMD E-350) and a 15h (AMD A10-5700) processor, but should work with all processors supported by AmdMsrTweaker (families 10h-15h).

Changes to frequency will not be reflected by e.g. `/proc/cpuinfo` (on Linux), but a quick benchmark such as `openssl speed sha1` should show a speed difference.

Compilation
-----------

Clone the repo and make sure you have C++ compiler and _make(1)_ installed. Then execute `make` (both BSD and GNU implementations are supported) in the cloned directory. Optionally copy the resulting file `amdmsrt` to a directory in `$PATH` such as /usr/sbin or /usr/local/sbin.

Usage
-----

Make sure you have the `msr` and `cpuid` (on Linux) or `cpuctl` (on FreeBSD) kernel modules loaded. The program will otherwise exit with a corresponding error message.

See the file `readme.txt` for usage examples. The program has to executed as root and the executable is called `amdmsrt`.

TODO
----

- Proper documentation
- Find a way to modify GPU P-states for Trinity. (10W more in idle on Linux compared to Windows, probably caused by missing Linux PowerPlay support for Trinity with the open drivers.)
