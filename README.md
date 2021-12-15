# Introduction 

![Windows](https://github.com/microsoft/cppgraphqlgen/workflows/Windows/badge.svg)
![macOS](https://github.com/microsoft/cppgraphqlgen/workflows/macOS/badge.svg)
![Linux](https://github.com/microsoft/cppgraphqlgen/workflows/Linux/badge.svg)

[GraphQL](https://graphql.org/) and [React](https://reactjs.org/) go together like peanut butter
and jelly, especially if you use a GraphQL client/compiler like [Relay](http://facebook.github.io/relay/en/)
or [Apollo](https://github.com/apollographql/apollo-client).

But GraphQL services are only implemented on the server. When using React Native or React JS in a
hybrid application, you typically have a native application which hosts islands or entire pages of
UI rendered with React components, and you might like to display content that you've cached offline
or that you otherwise generate on the client without needing to declare a separate data interface
or require a server round trip to load it.

This project includes a `graphqlservice` library with the core functionality of a GraphQL service
and a `schemagen` utility to generate types for your custom GraphQL service schema definition. Once
you implement the pure virtual methods on the object interfaces and add hooks to the Relay
[Network Layer](https://facebook.github.io/relay/docs/en/network-layer.html)/Apollo
[Terminating Link](https://www.apollographql.com/docs/link/overview.html#terminating) to call your
service, you can use the same GraphQL client code to access your native data source or a GraphQL
service online. You might even be able to share some more of that code between a progressive web
app and your native/hybrid app.

If what you're after is a way to consume a GraphQL service from C++, as of
[v3.6.0](https://github.com/microsoft/cppgraphqlgen/releases/tag/v3.6.0) this project also includes
a `graphqlclient` library and a `clientgen` utility to generate types matching a GraphQL request
document, its variables, and all of the serialization code you need to talk to a `graphqlservice`
implementation. If you want to consume another service, you will need access to the schema definition
(rather than the Introspection query results), and you will need be able to send requests along with
any variables to the service and parse its responses into a `graphql::response::Value` (e.g. with the
`graphqljson` library) in your code.

# Getting Started

## Related projects

The most complete examples I've built are related to [GqlMAPI](https://github.com/microsoft/gqlmapi):

- [eMAPI](https://github.com/microsoft/eMAPI): Windows-only Electron app which lets you access
the [MAPI](https://en.wikipedia.org/wiki/MAPI) interface used by
[Microsoft Outlook](https://en.wikipedia.org/wiki/Microsoft_Outlook). Its goal is to be a spiritual
successor to a debugging and diagnostic tool called
[MFCMAPI](https://github.com/stephenegriffin/mfcmapi).
- [electron-gqlmapi](https://github.com/microsoft/electron-gqlmapi): Native module for Electron
which hosts `GqlMAPI` in `eMAPI`. It includes JS libraries for calling the native module across the
Electron IPC channel.
- [Tauri-GqlMAPI](https://github.com/wravery/tauri-gqlmapi): Reimplementation of `eMAPI` built
in [Rust](https://www.rust-lang.org/) and [TypeScript](https://www.typescriptlang.org/) on top of
[Tauri](https://tauri.studio/en).
- [gqlmapi-rs](https://github.com/wravery/gqlmapi-rs): `Rust` crate I built to expose safe
bindings for `GqlMAPI`. It is loosely based on `electron-gqlmapi`, and it is used by
`Tauri-GqlMAPI`.

I created a couple of sample projects that worked with earlier versions of `cppgraphqlgen` to
demonstrate integrating the [schema.today.graphql](./samples/schema.today.graphql) service into an
Electron app. They're still available under my personal account, but I haven't updated them
recently:

- [electron-cppgraphql](https://github.com/wravery/electron-cppgraphql): Node Native Module which compiles
against the version of the Node headers included in Electron. This was the starting point for
`electron-gqlmapi`, and it is still useful as a sample because it does not depend on a platform-specific
API like `MAPI`, so it works cross-platform.
- [cppgraphiql](https://github.com/wravery/cppgraphiql): Electron app which consumes `electron-cppgraphql` and
exposes an instance of [GraphiQL](https://github.com/graphql/graphiql) on top of it.

Feel free to use either or both of these as a starting point to integrate your own generated
service with `Node`, `Electron`, or `Tauri`. PRs with links to your own samples are always welcome.

## Installation process

I've tested this on Windows with both Visual Studio 2017 and 2019, and on Linux using an Ubuntu 20.04 LTS instance running in
WSL with both gcc 9.3.0 and clang 10.0.0. The key compiler requirement is support for C++17 including std::filesystem, earlier
versions of gcc and clang may not have enough support for that.

The easiest way to get all of these and to build `cppgraphqlgen` in one step is to use
[microsoft/vcpkg](https://github.com/microsoft/vcpkg). To install with vcpkg, make sure you've pulled the latest version
and then run `vcpkg install cppgraphqlgen` (or `cppgraphqlgen:x64-windows`, `cppgraphqlgen:x86-windows-static`, etc.
depending on your platform). To install just the dependencies and work in a clone of this repo, you'll need some subset
of `vcpkg install pegtl boost-program-options rapidjson gtest`. It works for Windows, Linux, and Mac,
but if you want to try building for another platform (e.g. Android or iOS), you'll need to do more of this manually.

Manual installation will work best if you clone the GitHub repos for each of the dependencies and follow the installation
instructions for each project. You might also be able to find pre-built packages depending on your platform, but the
versions need to match.

## Software dependencies

The build system for this project uses [CMake](http://www.cmake.org/). You will need to have CMake (at least version
3.8.0) installed, and the library dependencies need to be where CMake can find them. Otherwise you need to disable the
options which depend on them.

I also picked a few other projects as dependencies, most of which are optional when consuming this project. If you
redistribute any binaries built from these libraries, you should still follow the terms of their individual licenses. As
of this writing, this library and all of its redistributable dependencies are available under the MIT license, which
means you need to include an acknowledgement along with the license text.

### graphqlpeg

- GraphQL parsing: [Parsing Expression Grammar Template Library (PEGTL)](https://github.com/taocpp/PEGTL) release 3.2.0,
which is part of [The Art of C++](https://taocpp.github.io/) library collection. I've added this as a sub-module, so you
do not need to install this separately. If you already have 3.2.0 installed where CMake can find it, it will use that
instead of the sub-module and avoid installing another copy of PEGTL.

### graphqlservice

The core library depends on `graphqlpeg` and it references the PEGTL headers itself at build time. Both of those mean it
depends on PEGTL as well.

### graphqljson (`GRAPHQL_USE_RAPIDJSON=ON`)

- JSON support: [RapidJSON](https://github.com/Tencent/rapidjson) release 1.1.0. If you don't need JSON support, you can
also avoid installing this dependency. You will need to set `GRAPHQL_USE_RAPIDJSON=OFF` in your CMake configuration to
do that.

### schemagen

I'm using [Boost](https://www.boost.org/doc/libs/1_69_0/more/getting_started/index.html) for `schemagen`:

- Command line handling: [Boost.Program_options](https://www.boost.org/doc/libs/1_69_0/doc/html/program_options.html).
Run `schemagen -?` to get a list of options. Many of the files in the [samples](samples/) directory were generated
with `schemagen`, you can look at [samples/CMakeLists.txt](samples/CMakeLists.txt) for a few examples of how to call it:
```
Usage:  schemagen [options] <schema file> <output filename prefix> <output namespace>
Command line options:
  --version              Print the version number
  -? [ --help ]          Print the command line options
  -v [ --verbose ]       Verbose output including generated header names as
                         well as sources
  -s [ --schema ] arg    Schema definition file path
  -p [ --prefix ] arg    Prefix to use for the generated C++ filenames
  -n [ --namespace ] arg C++ sub-namespace for the generated types
  --source-dir arg       Target path for the <prefix>Schema.cpp source file
  --header-dir arg       Target path for the <prefix>Schema.h header file
  --no-stubs             Generate abstract classes without stub implementations
  --separate-files       Generate separate files for each of the types
  --no-introspection     Do not generate support for Introspection
```

I've tested this with several versions of Boost going back to 1.65.0. I expect it will work fine with most versions of
Boost after that. The Boost dependencies are only used by the `schemagen` utility at or before your build, so you
probably don't need to redistribute it or the Boost libraries with your project.

If you are building shared libraries on Windows (DLLs) using vcpkg or `BUILD_SHARED_LIBS=ON` in CMake, be aware that this
adds a runtime dependency on a Boost DLL. The `schemagen` tool won't run without it. However, in addition to automating
the install of Boost, vcpkg also takes care of installing the dependencies next to `schemagen.exe` when building the
Windows and UWP shared library targets (the platform triplets which don't end in `-static`).

### clientgen

The `clientgen` utility is based on `schemagen` and shares the same external dependencies. The command line arguments
are almost the same, except it takes an extra file for the request document and there is no equivalent to `--no-stubs` or
`--separate-files`:
```
Usage:  clientgen [options] <schema file> <request file> <output filename prefix> <output namespace>
Command line options:
  --version              Print the version number
  -? [ --help ]          Print the command line options
  -v [ --verbose ]       Verbose output including generated header names as
                         well as sources
  -s [ --schema ] arg    Schema definition file path
  -r [ --request ] arg   Request document file path
  -o [ --operation ] arg Operation name if the request document contains more
                         than one
  -p [ --prefix ] arg    Prefix to use for the generated C++ filenames
  -n [ --namespace ] arg C++ sub-namespace for the generated types
  --source-dir arg       Target path for the <prefix>Client.cpp source file
  --header-dir arg       Target path for the <prefix>Client.h header file
  --no-introspection     Do not expect support for Introspection
```

This utility should output one header and one source file for each request document. A request document may contain
more than one operation, in which case you must specify the `--operation` (or `-o`) parameter to indicate which one
should be used to generate the files. If you want to generate client code for more than one operation in the same
document, you will need to run `clientgen` more than once and specify another operation name each time.

The generated code depends on the `graphqlclient` library for serialization of built-in types. If you link the generated
code, you'll also need to link `graphqlclient`, `graphqlpeg` for the pre-parsed, pre-validated request AST, and
`graphqlresponse` for the `graphql::response::Value` implementation.

Sample output for `clientgen` is in the [samples/client](samples/client) directory, and each sample is consumed by
a unit test in [test/ClientTests.cpp](test/ClientTests.cpp).

### tests (`GRAPHQL_BUILD_TESTS=ON`)

- Unit testing: [Google Test](https://github.com/google/googletest) for the unit testing framework. If you don't want to
build or run the unit tests, you can avoid this dependency as well by setting `GRAPHQL_BUILD_TESTS=OFF` in your CMake
configuration.

## API references

See [GraphQLService.h](include/graphqlservice/GraphQLService.h) for the base types implemented in
the `graphql::service` namespace. Take a look at [TodayMock.h](samples/today/TodayMock.h) and
[TodayMock.cpp](samples/today/TodayMock.cpp) to see a sample implementation of a custom schema defined
in [schema.today.graphql](samples/schema.today.graphql) for testing purposes.

### Additional Documentation

There are some more targeted documents in the [doc](./doc) directory:

* [Parsing GraphQL](./doc/parsing.md)
* [Query Responses](./doc/responses.md)
* [JSON Representation](./doc/json.md)
* [Field Resolvers](./doc/resolvers.md)
* [Field Parameters](./doc/fieldparams.md)
* [Directives](./doc/directives.md)
* [Subscriptions](./doc/subscriptions.md)

### Samples

All of the generated files are in the [samples](samples/) directory. There are two different versions of
the generated code, one which creates a single pair of files (`samples/unified/`), and one which uses the
`--separate-files` flag with `schemagen` to generate individual header and source files (`samples/separate/`)
for each of the object types which need to be implemeneted. The only difference between
[TodayMock.h](samples/today/TodayMock.h) with and without `IMPL_SEPARATE_TODAY` defined should be that the
`--separate-files` option generates a [TodayObjects.h](samples/separate/TodayObjects.h) convenience header
which includes all of the inidividual object header along with the rest of the schema in
[TodaySchema.h](samples/separate/TodaySchema.h).

# Build and Test

### Visual Studio on Windows

Use the Open Folder command to open the root of the repo. If you've installed the dependencies with
vcpkg and run its Visual Studio integration command, Visual Studio should know how to build each of
the targets in this project automatically.

Once you've built the project Visual Studio's Test Explorer window should list the unit tests, and you
can run all of them from there.

### Command Line on any platform

Your experience will vary depending on your build toolchain. The same instructions should work for any platform that
CMake supports. These basic steps will build and run the tests. You can add options to build in another target directory,
change the config from `Debug` (default) to `Release`, use another build tool like `Ninja`, etc. If you are using vcpkg
to install the dependencies, remember to specify the `-DCMAKE_TOOLCHAIN_FILE=...` option when you run the initial build
configuration.

- Create a build directory: `"mkdir build && cd build"`
- Configure the build system: `"cmake .."`
- Tell CMake to invoke the build system: `"cmake --build ."` _You can repeat this step to rebuild your changes._
- CTest comes with CMake and runs the tests: `"ctest ."` _Run this frequently, and make sure it passes before commits._

You can then optionally install the public outputs by configuring it with `Release`:
- `cmake -DCMAKE_BUILD_TYPE=Release ..`
- `cmake --build . --target install` _You probably need to use `sudo` on Unix to do this._

## Interactive tests

If you want to try an interactive version, you can run `samples/sample` and paste in queries against
the same mock service or load a query from a file on the command line.

## Reporting Security Issues

Security issues and bugs should be reported privately, via email, to the Microsoft Security
Response Center (MSRC) at [secure@microsoft.com](mailto:secure@microsoft.com). You should
receive a response within 24 hours. If for some reason you do not, please follow up via
email to ensure we received your original message. Further information, including the
[MSRC PGP](https://technet.microsoft.com/en-us/security/dn606155) key, can be found in
the [Security TechCenter](https://technet.microsoft.com/en-us/security/default).

# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
