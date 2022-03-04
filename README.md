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

If what you're after is a way to consume a GraphQL service from C++, this project also includes
a `graphqlclient` library and a `clientgen` utility to generate types matching a GraphQL request
document, its variables, and all of the serialization code you need to talk to a `graphqlservice`
implementation. If you want to consume another service, you will need access to the schema definition
(rather than the Introspection query results), and you will need be able to send requests along with
any variables to the service and parse its responses into a `graphql::response::Value` (e.g. with the
`graphqljson` library) in your code.

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

I created a couple of sample projects to demonstrate integrating the
[schema.today.graphql](./samples/schema.today.graphql) service into an Electron app. The
`schema.today.graphql` schema and service implementation in `TodayMock.*` are focused on testing,
so the specific bootstrapping used in these projects should not be re-used, but you can use them
as a scaffold to start your own integration with Electron or Node.js. They're available under
my personal account, and I recently updated them to match the latest release of `cppgraphqlgen`:
- [electron-cppgraphql](https://github.com/wravery/electron-cppgraphql): Node Native Module which compiles
against the version of the Node headers included in Electron. This was the starting point for
`electron-gqlmapi`, and it is still useful as a sample because it does not depend on a platform-specific
API like `MAPI`, so it works cross-platform.
- [cppgraphiql](https://github.com/wravery/cppgraphiql): Electron app which consumes `electron-cppgraphql` and
exposes an instance of [GraphiQL](https://github.com/graphql/graphiql) on top of it.

Feel free to use either or both of these as a starting point to integrate your own generated
service with `Node`, `Electron`, or `Tauri`. PRs with links to your own samples are always welcome.

# Getting Started

## Installation process

The minimum OS and toolchain versions I've tested with this version of `cppgraphqlgen` are:
- Microsoft Windows: Visual Studio 2019
- Linux: Ubuntu 20.04 LTS with gcc 10.3.0
- macOS: 11 (Big Sur) with AppleClang 13.0.0.

The key compiler requirement is support for C++20 including coroutines and concepts. Some of these compiler
versions still treat coroutine support as experimental, and the CMake configuration can auto-detect that,
but earlier versions of gcc and clang do not seem to have enough support for C++20.

The easiest way to build and install `cppgraphqlgen` is to use [microsoft/vcpkg](https://github.com/microsoft/vcpkg).
See the [Getting Started](https://github.com/microsoft/vcpkg#getting-started) section of the `vcpkg` README
for details. Once you have that configured, run `vcpkg install cppgraphqlgen` (or `cppgraphqlgen:x64-windows`,
`cppgraphqlgen:x86-windows-static`, etc. depending on your platform). That will build and install all of the
dependencies for `cppgraphqlgen`, and then build `cppgraphqlgen` itself without any other setup. The `cppgraphqlgen`
package (and its dependencies) are advertised to the `CMake` `find_package` function through the
`-DCMAKE_TOOLCHAIN_FILE=<...>/scripts/buildsystems/vcpkg.cmake` parameter/variable. There are more details about
this in the `vcpkg` documentation.

If you want to build `cppgraphqlgen` yourself, you can do that with `CMake` from a clone or archive of this repo.
See the [Build and Test](#build-and-test) section below for instructions. You will need to install the dependencies
first where `find_package` can find them. If `vcpkg` works otherwise, you can do that with `vcpkg install pegtl
boost-program-options rapidjson gtest`. Some of these are optional, if for example you do not build the tests. If
`vcpkg` does not work, please see the documentation for each of those dependencies, as well as your
platform/toolchain documentation for perferred installation mechanisms. You may need to build some or all of them
separately from source.

## Software dependencies

The build system for this project uses [CMake](http://www.cmake.org/). You will need to have CMake (at least version
3.15.0) installed, and the library dependencies need to be where CMake can find them. Otherwise you need to disable the
options which depend on them.

Besides the MIT license for this project, if you redistribute any source code or binaries built from these library
dependencies, you should still follow the terms of their individual licenses. As of this writing, this library and
all of its dependencies are available under either the MIT License or the Boost Software License (BSL). Both
licenses roughly mean that you may redistribute them freely as long as you include an acknowledgement along with
the license text. Please see the license or copyright notice which comes with each project for more details.

### graphqlpeg

- GraphQL parsing: [Parsing Expression Grammar Template Library (PEGTL)](https://github.com/taocpp/PEGTL) release 3.2.5,
which is part of [The Art of C++](https://taocpp.github.io/) library collection. I've added this as a sub-module, so you
do not need to install this separately. If you already have 3.2.5 installed where CMake can find it, it will use that
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

Sample output for `clientgen` is in the sub-directories of [samples/client](samples/client), and each sample is consumed by
a unit test in [test/ClientTests.cpp](test/ClientTests.cpp).

### tests (`GRAPHQL_BUILD_TESTS=ON`)

- Unit testing: [Google Test](https://github.com/google/googletest) for the unit testing framework. If you don't want to
build or run the unit tests, you can avoid this dependency as well by setting `GRAPHQL_BUILD_TESTS=OFF` in your CMake
configuration.

## API references

See [GraphQLService.h](include/graphqlservice/GraphQLService.h) for the base types implemented in
the `graphql::service` namespace.

Take a look at the [samples/learn](samples/learn) directory, starting with
[StarWarsData.cpp](samples/learn/StarWarsData.cpp) to see a sample implementation of a custom schema defined in
[schema.learn.graphql](samples/learn/schema/schema.learn.graphql). This is the same schema and sample data used in the
GraphQL tutorial on https://graphql.org/learn/. This directory builds an interactive command line application which
can execute query and mutation operations against the sample data in memory.

There are several helper functions for `CMake` declared in
[cmake/cppgraphqlgen-functions.cmake](cmake/cppgraphqlgen-functions.cmake), which is automatically included if you use
`find_package(cppgraphqlgen)` in your own `CMake` project. See
[samples/learn/schema/CMakeLists.txt](samples/learn/schema/CMakeLists.txt) and
[samples/learn/CMakeLists.txt](samples/learn/CMakeLists.txt), or the `CMakeLists.txt` files in some of the
other samples sub-directories for examples of how to use them to automatically rerun the code generators and update
the files in your source directory.

### Additional Documentation

There are some more targeted documents in the [doc](./doc) directory:

* [Awaitable Types](./doc/awaitable.md)
* [Parsing GraphQL](./doc/parsing.md)
* [Query Responses](./doc/responses.md)
* [JSON Representation](./doc/json.md)
* [Field Resolvers](./doc/resolvers.md)
* [Field Parameters](./doc/fieldparams.md)
* [Directives](./doc/directives.md)
* [Subscriptions](./doc/subscriptions.md)

### Samples

All of the samples are under [samples](samples/), with nested sub-directories for generated files:
- [samples/today](samples/today/): There are two different samples generated from
[schema.today.graphql](samples/today/schema.today.graphql) in this directory. The default
[schema](samples/today/schema/) target includes Introspection support (which is the default), while the
[nointrospection](samples/today/nointrospection/) target demonstrates how to disable Introspection support
with the `schemagen --no-introspection` parameter. The mock implementation of the service for both schemas is in
[samples/today/TodayMock.h](samples/today/TodayMock.h) and [samples/today/TodayMock.cpp](samples/today/TodayMock.cpp).
It builds an interactive `sample`/`sample_nointrospection` and `benchmark`/`benchmark_nointrospection` target for
each version, and it uses each of them in several unit tests.
- [samples/client](samples/client/): Several sample queries built with `clientgen` against the
[schema.today.graphql](samples/today/schema.today.graphql) schema shared with [samples/today](samples/today/). It
includes a `client_benchmark` executable for comparison with benchmark executables using the same hardcoded query
in [samples/today/]. The benchmark links with the default [schema](samples/today/schema/) target in
[samples/today](samples/today/) to handle the benchmark query.
- [samples/learn](samples/learn/): Simpler standalone which builds a `learn_star_wars` executable that follows
the tutorial examples on https://graphql.org/learn/.
- [samples/validation](samples/validation/): This schema is based on the examples and counter-examples from the
[Validation](https://spec.graphql.org/October2021/#sec-Validation) section of the October 2021 GraphQL spec. There
is no implementation of this schema, it relies entirely generated stubs (created with `schemagen --stubs`) to build
successfully without defining more than placeholder objects fo the Query, Mutation, and Subscription operations in
[samples/validation/ValidationMock.h](samples/validation/ValidationMock.h). It is used to test the validation logic
with every example or counter-example in the spec in [test/ValidationTests.cpp](test/ValidationTests.cpp).

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
change the config from `Debug` (default) to `Release`, use another build tool like `Ninja`, etc. If you are using `vcpkg`
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

If you want to try an interactive version, you can run `samples/today/sample` or `samples/today/sample_nointrospection`
and paste in queries against the same mock service or load a query from a file on the command line.

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
