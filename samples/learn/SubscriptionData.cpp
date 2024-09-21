// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CharacterObject.h"

#include "SubscriptionData.h"

namespace graphql::learn {

Subscription::Subscription() noexcept
{
}

std::shared_ptr<object::Character> Subscription::getCharacterChanged() const noexcept
{
	return {};
}

} // namespace graphql::learn
