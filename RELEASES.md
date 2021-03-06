# SolidFrame Releases

## Backlog

* solid_frame_mpipc: SOCKS5
* solid_frame_mpipc: test with thousands of connections
* DOCUMENTAION: API
* solid_serialization: test agaist ProtoBuf and FlatBuffers

## Version 2.1
* (DONE) solid_frame_aio_openssl: Improved OpenSSL/BoringSSL support
* (DONE) solid_frame_mpipc: Pluggable (header only) support for SSL
* (DONE) BUILD: Android support - https://github.com/vipalade/bubbles
* (DONE) solid_serialization: Support for std::bitset, std::vector<bool> and std::set
* (DONE) solid_serialization: Test suported stl containers.
* (DONE) BUILD: Support for CMake extern command for find_package(SolidFrame)
* (DONE) solid_frame_mpipc: Basic, pluggable compression support using [Snappy](https://google.github.io/snappy/)
* (DONE) TUTORIAL: mpipc_request_ssl


## Version 2.0
* Fix utility memoryfile examples
* pushCall instead of pushReinit
* Finalizing the MPIPC library.
* Cross-platform support:
	* Linux
	* FreeBSD
	* Darwin/OSX (starting with XCode 8 for thread_local)
* Add "make install" support.
* Documentation.

