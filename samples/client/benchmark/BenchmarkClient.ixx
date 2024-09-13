// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include "BenchmarkClient.h"

export module GraphQL.Benchmark.BenchmarkClient;

namespace included = graphql::client;

export namespace graphql::client {

namespace exported {

namespace benchmark {

using included::benchmark::GetRequestText;
using included::benchmark::GetRequestObject;

} // namespace benchmark

namespace query::Query {

using included::benchmark::GetRequestText;
using included::benchmark::GetRequestObject;
using included::query::Query::GetOperationName;

using included::query::Query::Response;
using included::query::Query::parseResponse;

using included::query::Query::Traits;

} // namespace query::Query

} // namespace exported

using namespace exported;

} // namespace graphql::client
