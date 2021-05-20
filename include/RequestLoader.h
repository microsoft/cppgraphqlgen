// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef REQUESTLOADER_H
#define REQUESTLOADER_H

#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLService.h"

namespace graphql::generator {

class RequestLoader
{
public:
	explicit RequestLoader();

	void visit(const peg::ast& request, std::string_view operationName);

private:

};

} /* namespace graphql::generator */

#endif // REQUESTLOADER_H
