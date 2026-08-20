Lksc Core
==========

This is the official reference wallet for Lks digital currency and comprises the backbone of the Lks peer-to-peer network. You can [download Lksc Core](https://www.lksfoundation.org/downloads/) or [build it yourself](#building) using the guides below.

Running
---------------------
The following are some helpful notes on how to run Lksc Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/lks-qt` (GUI) or
- `bin/lksd` (headless)

### Windows

Unpack the files into a directory, and then run lks-qt.exe.

### macOS

Drag Lksc Core to your applications folder, and then run Lksc Core.

### Need Help?

* See the [Lks documentation](https://docs.lksfoundation.org)
for help and more information.
* Ask for help on [Lks Discord](http://staylksy.com)
* Ask for help on the [Lks Forum](https://lksfoundation.org/forum)

Building
---------------------
The following are developer notes on how to build Lksc Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Lksc Core repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Productivity Notes](productivity.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- Source Code Documentation ***TODO***
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [JSON-RPC Interface](JSON-RPC-interface.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* See the [Lks Developer Documentation](https://lkscore.readme.io/)
  for technical specifications and implementation details.
* Discuss on the [Lks Forum](https://lksfoundation.org/forum), in the Development & Technical Discussion board.
* Discuss on [Lks Discord](http://staylksy.com)
* Discuss on [Lks Developers Discord](http://chat.lksdevs.org/)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [lks.conf Configuration File](lks-conf.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Memory](reduce-memory.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)
- [PSBT support](psbt.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
