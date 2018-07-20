# Thunderbolt(TM) user-space components

[![Build Status](https://travis-ci.org/intel/thunderbolt-software-user-space.svg?branch=master)](https://travis-ci.org/intel/thunderbolt-software-user-space)

## License
These components are distributed under a BSD-style license. See COPYING for the
full license.


## Overview
Thunderbolt™ technology is a transformational high-speed, dual protocol
I/O that provides unmatched performance with up to 40Gbps bi-directional
transfer speeds. It provides flexibility and simplicity by supporting both
data (PCIe, USB3.1) and video (DisplayPort) on a single cable connection
that can daisy-chain up to six devices.


## Features
The user-space components implement device approval support:
1. Easier interaction with the kernel module for approving connected devices.
2. ACL for auto-approving devices white-listed by the user.


## tbtacl
tbtacl is intended to be triggered by udev (see the udev rules in tbtacl.rules).
It auto-approves devices that are found in ACL.


## tbtadm
tbtadm is a user-facing CLI tool. It provides operations for device approval,
handling the ACL and more.


## Supported OSes
- Ubuntu* 16.04 and 17.04
- Fedora* 26
- Clear Linux*
- SUSE Linux Enterprise(SLE)* 15
- openSUSE Tumbleweed*


## Kernel/Daemon Compatibility
The user-space components operate in coordination with the upstream Thunderbolt
kernel driver (found in v4.13) to provide the Thunderbolt functionalities. These
components are NOT compatible with the old out-of-tree Thunderbolt kernel
module.


## Build instructions
### Build dependencies
Build dependencies are:
- CMake
- boost.filesystem
- txt2tags (for generating the man page)

You also need a c++ compiler with C++14 support and gzip.

Tested with:
- g++ - v5.4 and v7.1.1
- CMake - v3.5.1 and v3.9.1
- boost - v1.58 and v1.63
- txt2tags - v2.5 and v2.6

For example, on Ubuntu you can install the dependencies with the following
command:  
`sudo apt-get install cmake libboost-filesystem-dev txt2tags pkg-config`

On Fedora, use this:  
`dnf install cmake boost-devel txt2tags`

on SLE15 and openSUSE Tumbleweed
`zypper in txt2tags libboost_filesystem-devel cmake`

### Building
Use the CMakeLists.txt file found in the root directory to build the project.
For example (run it in the directory holding the code):
1. `mkdir build`
2. `cd build`
3. `cmake .. -DCMAKE_BUILD_TYPE=Release`
4. `cmake --build .`

On step 3, `CMAKE_INSTALL_PREFIX`, `UDEV_BIN_DIR` and `UDEV_RULES_DIR` variables
can be used for changing the default installation location, e.g. to install
`tbtadm` under `/usr/bin` instead of the default `/usr/local/bin` run:  
`cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr`

### Installation
Installation can be done in one of 2 options:
- From build directory, run `cmake --build . --target install`.
- From build directory, run `cpack -G RPM` to create an RPM package or
  `cpack -G DEB` to create a DEB package. Then, use your distro package manager
  to install the resulted package.


## Changelog
### v0.9.3
- xdomain: added loading Thunderbolt networking driver automatically on XDomain
connection
- tbtadm: added `peers` command and XDomain is now shown in the topology output
- tbtadm: added `add` command for adding to ACL database without `approve`
command
- tbtadm: fixed adding to ACL database in security level 0
- tbtadm: fixed Coverity error reformatting string array initialization
- tbtadm: fixed multi-controller topology tree
- tbtadm: improved readability of console output
- tests: added automatic testing in umockdev simulated environment with docker
- build: remove unneeded `libboost-program-options` dependency

### v0.9.2
- tbtadm: added `--once` flag for `approve-all` command
- tbtadm: `approve` command added
- tbtadm: bash completion support added (GitHub issue #27)
- tbtacl: udev dir config variable default values are taken from `pkg-config udev`
- tbtadm: handle empty vendor/device name correctly (GitHub issue #25)

### v0.9.1
- Build definition updated to support configuration, installation and packaging
- Documentation update (GitHub issue #23)
- man page added (GitHub issue #9)
- Fixes for documentation (GitHub issue #20)
- Build definition updated (GitHub issues #21, #22)
- tbtadm: Compilation warnings (GitHub issue #22)

### v0.9
- First official release
- tbtacl: use C++ instead of Python for write action (GitHub issue #19)

### Eng. drop 2
- tbtadm: more commands added (devices, topology, acl)
- tbtadm: 'remove' accepts route-string, not only UUID
- tbtadm: 'remove-all' prints removed entry count
- tbtadm: future compatibility with xdomain changes
- tbtacl: use sh instead of bash
- tbtacl: improved error reporting (using write.py to get the actual errno)
- tbtadm, tbtacl, tbtacl.rules: improvement and bug fixes in SL2 support
- tbtacl: fixed SL2 handling
- tbtacl: don't assume errno(1) is installed
- tbtacl.rules: correctly handle change with authorized==2 (for SL2)
- tbtadm: correctly handle multi-controller systems
- tbtadm: 'approve-all' - do nothing if SL isn't 1 or 2
- tbtadm: 'approve-all' - add key on SL2
- tbtadm: removing non-existing ACL entry is just a warning, not an error
- tbtadm: File class reports errors more accurately for write() and read()


## Known Issues
- tbtadm should use a helper + polkit for better permission handling
- error reporting can be improved
- bash completion rules are less strict about completions than what `tbtadm`
  actually accepts


## Information
The source for this code:
- https://github.com/01org/thunderbolt-software-user-space

Mailing list:
- thunderbolt-software@lists.01.org
- Register at: https://lists.01.org/mailman/listinfo/thunderbolt-software
- Archives at: https://lists.01.org/pipermail/thunderbolt-software/

For additional information about Thunderbolt technology visit:
- https://01.org/thunderbolt-sw
- https://thunderbolttechnology.net/
