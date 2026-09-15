// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef SUBSCRIPTIONDATA_H
#define SUBSCRIPTIONDATA_H

#include "SubscriptionObject.h"

namespace graphql::learn {

namespace object {

class Character;

} // namespace object

class Subscription
{
public:
	explicit Subscription() noexcept;

	std::shared_ptr<object::Character> getCharacterChanged() const noexcept;
};

} // namespace graphql::learn

#endif // SUBSCRIPTIONDATA_H
