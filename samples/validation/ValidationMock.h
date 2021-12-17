// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VALIDATIONMOCK_H
#define VALIDATIONMOCK_H

#include "ValidationSchema.h"

#include "QueryObject.h"
#include "MutationObject.h"
#include "SubscriptionObject.h"

namespace graphql::validation {

class Query
{
public:
	explicit Query() = default;
};

class Mutation
{
public:
	explicit Mutation() = default;
};

class Subscription
{
public:
	explicit Subscription() = default;
};

} // namespace graphql::validation

#endif // VALIDATIONMOCK_H
