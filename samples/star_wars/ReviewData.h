// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef REVIEWDATA_H
#define REVIEWDATA_H

#include "ReviewObject.h"

namespace graphql::learn {

class Review
{
public:
	explicit Review(
		response::IntType stars, std::optional<response::StringType> commentary) noexcept;

	response::IntType getStars(service::FieldParams&& params) const noexcept;
	std::optional<response::StringType> getCommentary(service::FieldParams&& params) const noexcept;

private:
	const response::IntType stars_;
	const std::optional<response::StringType> commentary_;
};

} // namespace graphql::learn

#endif // REVIEWDATA_H
