# Parsing GraphQL Documents

## PEGTL

As mentioned in the [README](../README.md), `cppgraphqlgen` uses the
[Parsing Expression Grammar Template Library (PEGTL)](https://github.com/taocpp/PEGTL)
release 3.1.1, which is part of [The Art of C++](https://taocpp.github.io/)
library collection. I've added this as a sub-module, so you do not need to
install this separately. If you already have 3.1.1 installed where CMake can
find it, it will use that instead of the sub-module and avoid installing
another copy of PEGTL.

It uses the [contrib/parse_tree.hpp](../PEGTL/include/tao/pegtl/contrib/parse_tree.hpp)
module to build an AST automatically while parsing the document. The AST and
the underlying grammar rules are tuned to the needs of `cppgraphqlgen`, but if
you have another use for a GraphQL parser you could probably make a few small
tweaks to include additional information in the rules or in the resulting AST.
You could also use the grammar without the AST module if you want to handle
the parsing callbacks another way. The grammar itself is defined in
[Grammar.h](../include/graphqlservice/internal/Grammar.h), and the AST
selector callbacks are all defined in [SyntaxTree.cpp](../src/SyntaxTree.cpp).
The grammar handles both the schema definition syntax which is used in
`schemagen`, and the query/mutation/subscription operation syntax used in
`Request::resolve` and `Request::subscribe`.

## Utilities

The [GraphQLParse.h](../include/graphqlservice/GraphQLParse.h) header includes
several utility methods to help generate an AST from a `std::string_view`
(`parseString`), an input file (`parseFile`), or using a
[UDL](https://en.cppreference.com/w/cpp/language/user_literal) (`_graphql`)
for hardcoded documents.

The UDL is used throughout the sample unit tests and in `schemagen` for the
hard-coded introspection schema. It will be useful for additional unit tests
against your own custom schema.

At runtime, you will probably call `parseString` most often to handle dynamic
queries. If you have persisted queries saved to the file system or you are
using a snapshot/[Approval Testing](https://approvaltests.com/) strategy you
might also use `parseFile` to parse queries saved to text files.

When parsing an executable document with `parseString`, `parseFile`, or the
UDL, the parser will try a subset of the grammar first which does not accept
schema definitions, and if that fails it will try the full grammar as a
fallback so that the validation step can check for documents with an invalid
mix of executable and schema definitions.

There are `parseSchemaString` and `parseSchemaFile` functions which do the
opposite, but unless you are building additional tooling on top of the
`graphqlpeg` library, you will probably not need them. They have only been used
by `schemagen` and `clientgen` in this project.

## Encoding

The document must use a UTF-8 encoding. If you need to handle documents in
another encoding you will need to convert them to UTF-8 before parsing.

If you need to convert the encoding at runtime, I would recommend using
`std::wstring_convert`, with the caveat that it has been
[deprecated](https://en.cppreference.com/w/cpp/locale/wstring_convert) in
C++17. You could keep using it until it is replaced in the standard, you
could use a portable non-standard library like
[ICU](http://site.icu-project.org/design/cpp), or you could use
platform-specific conversion routines like
[WideCharToMultiByte](https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte)
on Windows instead.