// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MutationData.h"

namespace graphql::learn {

Mutation::Mutation() noexcept
{
}

std::shared_ptr<object::Review> Mutation::applyCreateReview(
	service::FieldParams&& params, Episode&& epArg, ReviewInput&& reviewArg) noexcept
{
	auto review = std::make_shared<Review>(reviewArg.stars, std::move(reviewArg.commentary));

	// Save a copy of this review associated with this episode.
	reviews_[epArg].push_back(review);

	return std::make_shared<object::Review>(std::move(review));
}

} // namespace graphql::learn
