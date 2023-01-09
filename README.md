# SPS-VP

A modified version of [SymEx-VP][symex-vp github] with support for symbolic execution of stateful network protocol implementations.
This version of SymEx-VP is intended to be used in conjunction with [SPS][sps github] protocol specifications.
The implementation is further described in the publication “Specification-based Symbolic Execution for Stateful Network Protocol Implementations in the IoT” which will be published in the IEEE Internet of Things Journal.

## Usage

As described in Section III E) of the aforementioned paper, this version of SymEx-VP receives input specifications from a [state protocol server][sps github] (SPS).
When starting a state protocol server, a host and port must be specified.
This host and port information needs to be passed to the SymEx-VP executable using the `--sps-host` and `--sps-port` command-line arguments.
For example:

    $ hifive-vp --sps-host 127.0.0.1 --sps-port 2342 <path to executable>

The `hifive-vp` will then communicate with the state protocol server to obtain symbolic input format specifications based on the current state in the specified protocol state machine.
Refer to Section III of the journal publication for more information.

## License

See the [original SymEx-VP license description][symex-vp license].
This modified version of SymEx-VP also includes a [Bencode implementation][bencode.hpp github] by Jim Porter which is licensed under MIT.

[sps github]: https://github.com/agra-uni-bremen/sps
[symex-vp github]: https://github.com/agra-uni-bremen/symex-vp
[symex-vp license]: https://github.com/agra-uni-bremen/symex-vp#license
[bencode.hpp github]: https://github.com/jimporter/bencode.hpp
