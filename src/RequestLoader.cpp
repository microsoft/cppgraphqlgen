// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "RequestLoader.h"

#include "graphqlservice/GraphQLGrammar.h"

#include <array>
#include <sstream>

namespace graphql::generator {

RequestLoader::RequestLoader()
{
}

void RequestLoader::visit(const peg::ast& request, std::string_view operationName)
{
}

} /* namespace graphql::generator */
