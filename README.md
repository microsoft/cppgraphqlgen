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

I created a couple of sample projects to demonstrate integrating the [schema.today.graphql](./samples/schema.today.graphql)
service into an Electron app. They're available under my personal account, feel free to use either or both of these as a
starting point to integrate your own generated service with Node or Electron:
* [electron-cppgraphql](https://github.com/wravery/electron-cppgraphql): Node Native Module which compiles
against the version of the Node headers included in Electron.
* [cppgraphiql](https://github.com/wravery/cppgraphiql): Electron app which consumes `electron-cppgraphql` and
exposes an instance of [GraphiQL](https://github.com/graphql/graphiql) on top of it.

## Installation process

First, clone this repo, then make sure you have the dependencies. Acquiring the dependencies is a
bit different depending on your platform.

I've tested this on Windows with Visual Studio 2017 and Linux using an Ubuntu instance running in
WSL.

I added a vcpkg port for this project, if you have vcpkg you can also install everything with `vcpkg install cppgraphqlgen`.

## Software dependencies

For now, I'm maintaining compatibility with C++11 for maximum portability. I picked a few projects as dependencies:

- JSON support: [RapidJSON](https://github.com/Tencent/rapidjson).
- GraphQL parsing: [Parsing Expression Grammar Template Library (PEGTL)](https://github.com/taocpp/PEGTL), which is part of [The Art of C++](https://taocpp.github.io/) library collection. Specifically, you should use the [2.x branch](https://github.com/taocpp/PEGTL/tree/2.x) of PEGTL instead of master since it's compatible with C++11.
- Unit testing: [Google Test](https://github.com/google/googletest) for the unit testing framework.

The build system for this project uses [CMake](http://www.cmake.org/). You'll need to have all 3
dependencies installed on your system along with CMake to build this project.

### Using vcpkg

Vcpkg can install the dependencies from source on either platform, and that's what I'm using on
Windows. Use `vcpkg install rapidjson pegtl gtest` to get all of them.

This approach works well on Linux. I've done all of my testing for Linux with WSL using vcpkg.

### Windows with NuGet

All of these packages dependencies should also be available in NuGet packages if you don't want to
use [vcpkg](https://github.com/Microsoft/vcpkg) but still want to use a package manager.

### Linux or Windows from GitHub

Clone each of the repos from GitHub and follow the installation instructions in the README.md
files.

## API references

See [GraphQLService.h](include/graphqlservice/GraphQLService.h) for the base types implemented in
the `facebook::graphql::service` namespace. Take a look at [Today.h](include/Today.h) and [Today.cpp](Today.cpp)
to see a sample implementation of a custom schema defined in [schema.today.graphql](samples/schema.today.graphql)
for testing purposes.

All of the generated files are in the [samples](samples/) directory. If you modify the code
generator in SchemaGenerator.* and rebuild, `make install` will update them. Please remember to
include updating the samples in any pull requests which change them.

# Build and Test

## Windows

There are a couple of options for building on Windows. You can either run CMake from the command
line, or you can use the CMake integration in Visual Studio. They behave a little differently, but
I prefer building and running tests in Visual Studio, then optionally performing a Release build
and install from the command line.

### Visual Studio

Use the Open Folder command to open the root of the repo. If you've installed the dependencies with
vcpkg and run its Visual Studio integration command, Visual Studio should know how to build each of
the targets in this project automatically. I've only been able to get x86-debug to work so far in
Visual Studio though, it doesn't switch properly between the Debug and Release dependencies
installed by vcpkg on my machine.

### Command Line

To build from the command line, run:

`cmake -DCMAKE_TOOLCHAIN_FILE=<...path to vcpkg root...>/scripts/buildsystems/vcpkg.cmake .` once
to populate the Visual Studio solution files, then:

`msbuild cppgraphql.sln` to perform the build.

You can also build optional `.vcxproj` project files with msbuild to perform the associated task,
e.g. installing or running tests.

If you want to make a Release build, add the `/p:Configuration=Release` argument to the `msbuild`
command line.

## Linux

To build everything on Linux run:

`cmake .`

`make`

You can then optionally install the public outputs by running:

`sudo make install`.

## Testing

Run the unit tests with `tests` from the build output directory.

If you want to try an interactive version, you can run `test_today` and paste in queries against
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
