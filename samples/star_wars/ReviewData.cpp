// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ReviewData.h"

namespace graphql::learn {

Review::Review(response::IntType stars, std::optional<response::StringType>&& commentary) noexcept
	: stars_ { stars }
	, commentary_ { std::move(commentary) }
{
}

response::IntType Review::getStars() const noexcept
{
	return stars_;
}

std::optional<response::StringType> Review::getCommentary() const noexcept
{
	return commentary_;
}

} // namespace graphql::learn
