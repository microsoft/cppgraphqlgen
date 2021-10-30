// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ReviewData.h"

namespace graphql::learn {

Review::Review(response::IntType stars, std::optional<response::StringType> commentary) noexcept
	: stars_ { stars }
	, commentary_ { std::move(commentary) }
{
}

service::FieldResult<response::IntType> Review::getStars(service::FieldParams&& params) const
{
	return { stars_ };
}

service::FieldResult<std::optional<response::StringType>> Review::getCommentary(
	service::FieldParams&& params) const
{
	return { commentary_ };
}

} // namespace graphql::learn
