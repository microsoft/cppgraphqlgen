# Introduction 

[GraphQL](https://graphql.org/) and [React](https://reactjs.org/) go together like peanut butter and jelly, especially if you use a GraphQL client/compiler like [Relay](http://facebook.github.io/relay/en/) or [Apollo](https://github.com/apollographql/apollo-client).

But GraphQL services are only implemented on the server. When using React Native or React JS in a hybrid application, you typically have a native application which hosts islands or entire pages of UI rendered with React components, and you might like to display content that you've cached offline or that you otherwise generate on the client without needing to declare a separate data interface or require a server round trip to load it.

This project includes a `graphqlservice` library with the core functionality of a GraphQL service and a `schemagen` utility to generate types for your custom GraphQL service schema definition. Once you implement the pure virtual methods on the object interfaces and add hooks to the React Environment/Network to call your service, you can use the same GraphQL client code (e.g. Relay) to access your native data source or a GraphQL service online. You might even be able to share some more of that code between a progressive web app and your native app.

# Getting Started

## Installation process

First, clone this repo, then make sure you have the dependencies. Acquiring the dependencies is a bit different depending on your platform.

I've tested this on Windows with Visual Studio 2017 and Linux using an Ubuntu instance running in WSL.

## Software dependencies

I picked [cpprestsdk](https://github.com/Microsoft/cpprestsdk) for the JSON support, [libgraphqlparser](https://github.com/graphql/libgraphqlparser) for the GraphQL parsing, and [Google Test](https://github.com/google/googletest) for the unit testing framework. The build for all of these uses [CMake](http://www.cmake.org/).

You'll need to have all 3 projects installed on your system along with CMake to build this project.

### Using vcpkg

Vcpkg can install the dependencies from source on either platform, and that's what I'm using on Windows. The Windows [installation instructions](https://github.com/Microsoft/vcpkg/blob/master/README.md) for [cpprestsdk](https://github.com/Microsoft/cpprestsdk) recommend it, and if you add the port I did for [graphqlparser](https://github.com/Microsoft/vcpkg/pull/3953) you can get both of them with this tool.

This approach works well on Linux as well, but you'll still need that pull request to add graphqlparser to the ports collection.

### Windows

The [cpprestsdk](https://github.com/Microsoft/cpprestsdk) package should also be available in NuGet packages if you don't want to use [vcpkg](https://github.com/Microsoft/vcpkg).

However, the [libgraphqlparser](https://github.com/graphql/libgraphqlparser) project does not officially support building on Windows yet. I submitted another [pull request](https://github.com/graphql/libgraphqlparser/pull/67) to add that.

### Linux

Clone [libgraphqlparser](https://github.com/graphql/libgraphqlparser) and follow the installation instructions in the [README](https://github.com/graphql/libgraphqlparser/blob/master/README.md#building-libgraphqlparser). At some point it may prompt you to install additional components like Google Test.

Follow the Linux [installation instructions](https://github.com/Microsoft/cpprestsdk/blob/master/README.md#getting-started) for cpprestsdk. Again, it may pull in more dependencies.

## API references

See [GraphQLService.h](GraphQLService.h) for the base types implemented in the `facebook::graphql::service` namespace. Take a look at [Today.h](Today.h) and [Today.cpp](Today.cpp) to see a sample implementation of a custom schema defined in [schema.today.graphql](schema.today.graphql) for testing purposes. The unit tests.

# Build and Test

## Windows

There are a couple of options for building on Windows. You can either run CMake from the command line, or you can use the CMake integration in Visual Studio. They behave a little differently, but I prefer building and running tests in Visual Studio, then optionally performing a Release build and install from the command line.

### Visual Studio

Use the Open Folder command to open the root of the repo. If you've installed the dependencies with vcpkg and run its Visual Studio integration command, Visual Studio should know how to build each of the targets in this project automatically. I've only been able to get x86-debug to work so far in Visual Studio though, it doesn't switch properly between the Debug and Release dependencies installed by vcpkg on my machine.

### Command Line

To build from the command line, run:

`cmake -DCMAKE_TOOLCHAIN_FILE=<...path to vcpkg root...>/scripts/buildsystems/vcpkg.cmake .` once to populate the Visual Studio solution files, then:

`msbuild cppgraphql.sln` to perform the build.

You can also build optional `.vcxproj` project files with msbuild to perform the associated task, e.g. installing or running tests.

If you want to make a Release build, add the `/p:Configuration=Release` argument to the `msbuild` command line.

## Linux

To build everything on Linux run:

`cmake .`

`make`

You can then optionally install the public outputs by running:

`sudo make install`.

## Testing

Run the unit tests with `tests` from the build output directory.

If you want to try an interactive version, you can run `test_today` and paste in queries against the same mock service or load a query from a file on the command line.

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
