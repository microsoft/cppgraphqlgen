# Introduction 

[![Build Status](https://dev.azure.com/wravery/Build%20Pipelines/_apis/build/status/CppGraphQLGen-CI%20(Microsoft)?branchName=master)](https://dev.azure.com/wravery/Build%20Pipelines/_build/latest?definitionId=7&branchName=master)

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

# Getting Started

## Related projects

I created a couple of sample projects that worked up to version 2.x to demonstrate integrating the
[schema.today.graphql](./samples/today/schema.today.graphql) service into an Electron app. I haven't migrated them
to 3.x and C++17 yet, but they're available under my personal account. Feel free to use either or both of these as a
starting point to integrate your own generated service with Node or Electron. PRs with links to your own samples or
migrating either of those projects to 3.x are welcome.

- [electron-cppgraphql](https://github.com/wravery/electron-cppgraphql): Node Native Module which compiles
against the version of the Node headers included in Electron.
- [cppgraphiql](https://github.com/wravery/cppgraphiql): Electron app which consumes `electron-cppgraphql` and
exposes an instance of [GraphiQL](https://github.com/graphql/graphiql) on top of it.

## Installation process

I've tested this on Windows with both Visual Studio 2017 and 2019, and on Linux using an Ubuntu 18.04 LTS instance running in
WSL with both gcc 7.3.0 and clang 6.0.0. The key compiler requirement is support for C++17, earlier versions of gcc and clang
may not have enough support for that.

The easiest way to get all of these and to build `cppgraphqlgen` in one step is to use
[microsoft/vcpkg](https://github.com/microsoft/vcpkg). To install with vcpkg, make sure you've pulled the latest version
and then run `vcpkg install cppgraphqlgen` (or `cppgraphqlgen:x64-windows`, `cppgraphqlgen:x86-windows-static`, etc.
depending on your platform). To install just the dependencies and work in a clone of this repo, you'll need some subset
of `vcpkg install pegtl boost-program-options boost-filesystem rapidjson gtest`. It works for Windows, Linux, and Mac,
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

- GraphQL parsing: [Parsing Expression Grammar Template Library (PEGTL)](https://github.com/taocpp/PEGTL) release 3.0.0,
which is part of [The Art of C++](https://taocpp.github.io/) library collection. I've added this as a sub-module, so you
do not need to install this separately. If you already have 3.0.0 installed where CMake can find it, it will use that
instead of the sub-module and avoid installing another copy of PEGTL. _Note: PEGTL 3.0.0 is currently at pre-release._

### graphqlservice

The core library depends on `graphqlpeg` and it references the PEGTL headers itself at build time. Both of those mean it
depends on PEGTL as well.

### graphqljson (`GRAPHQL_USE_RAPIDJSON=ON`)

- JSON support: [RapidJSON](https://github.com/Tencent/rapidjson) release 1.1.0. If you don't need JSON support, you can
also avoid installing this dependency. You will need to set `GRAPHQL_USE_RAPIDJSON=OFF` in your CMake configuration to
do that.

### schemagen

I'm using [Boost](https://www.boost.org/doc/libs/1_69_0/more/getting_started/index.html) for `schemagen`:

- C++17 std::filesystem support on Unix:
[Boost.Filesystem](https://www.boost.org/doc/libs/1_69_0/libs/filesystem/doc/index.htm). Most of the default C++
compilers on Linux still have `std::filesystem` from C++17 in an experimental directory and require an extra
library. The standard just adopted the Boost library, so on Unix systems I have an `#ifdef` which redirects back to
it for the time being.
- Command line handling: [Boost.Program_options](https://www.boost.org/doc/libs/1_69_0/doc/html/program_options.html).
Run `schemagen -?` to get a list of options. Many of the files in the [samples](samples/) directory were generated
with `schemagen`, you can look at [samples/CMakeLists.txt](samples/CMakeLists.txt) for a few examples of how to call it:
```
Usage:  schemagen [options] <schema file> <output filename prefix> <output namespace>
Command line options:
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
```

I've only tested this with Boost 1.69.0, but I expect it will work fine with most other versions. The Boost dependencies
are only used by the `schemagen` utility at or before your build, so you probably don't need to redistribute it or the
Boost libraries with your project.

If you are building shared libraries on Windows (DLLs) using vcpkg or `BUILD_SHARED_LIBS=ON` in CMake, be aware that this
adds a runtime dependency on a Boost DLL. The `schemagen` tool won't run without it. However, in addition to automating
the install of Boost, vcpkg also takes care of installing the dependencies next to `schemagen.exe` when building the
Windows and UWP shared library targets (the platform triplets which don't end in `-static`).

### tests (`GRAPHQL_BUILD_TESTS=ON`)

- Unit testing: [Google Test](https://github.com/google/googletest) for the unit testing framework. If you don't want to
build or run the unit tests, you can avoid this dependency as well by setting `GRAPHQL_BUILD_TESTS=OFF` in your CMake
configuration.

## API references

See [GraphQLService.h](include/graphqlservice/GraphQLService.h) for the base types implemented in
the `graphql::service` namespace. Take a look at [UnifiedToday.h](samples/today/UnifiedToday.h) and
[UnifiedToday.cpp](samples/today/UnifiedToday.cpp) to see a sample implementation of a custom schema defined
in [schema.today.graphql](samples/today/schema.today.graphql) for testing purposes.

All of the generated files are in the [samples](samples/) directory. There are two different versions of
the generated code, one which creates a single pair of files (`samples/unified/`), and one which uses the
`--separate-files` flag with `schemagen` to generate individual header and source files (`samples/separate/`)
for each of the object types which need to be implemeneted. The only difference between
[UnifiedToday.h](samples/today/UnifiedToday.h)
and [SeparateToday.h](samples/today/SeparateToday.h) should be that the `SeparateToday` use a generated
[TodayObjects.h](samples/separate/TodayObjects.h) convenience header which includes all of the inidividual
object header along with the rest of the schema in [TodaySchema.h](samples/separate/TodaySchema.h). If you modify the
code generator in SchemaGenerator.* and rebuild, building the install target will update them. Please remember to
include updating the samples in any pull requests which change them.

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

- Configure the build system: `"cmake ."`
- Tell CMake to invoke the build system: `"cmake --build ."` _You can repeat this step to rebuild your changes._
- CTest comes with CMake and runs the tests: `"ctest ."` _Run this frequently, and make sure it passes before commits._

You can then optionally install the public outputs by running.

- `cmake --build . --config Release --target install` _You probably need to use `sudo` on Unix to do this._

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
