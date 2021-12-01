// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ReviewData.h"

namespace graphql::learn {

Review::Review(int stars, std::optional<std::string>&& commentary) noexcept
	: stars_ { stars }
	, commentary_ { std::move(commentary) }
{
}

int Review::getStars() const noexcept
{
	return stars_;
}

const std::optional<std::string>& Review::getCommentary() const noexcept
{
	return commentary_;
}

} // namespace graphql::learn
