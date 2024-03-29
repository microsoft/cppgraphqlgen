// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef MUTATIONDATA_H
#define MUTATIONDATA_H

#include "MutationObject.h"

#include "ReviewData.h"

namespace graphql::learn {

class Mutation
{
public:
	explicit Mutation() noexcept;

	std::shared_ptr<object::Review> applyCreateReview(
		Episode epArg, ReviewInput&& reviewArg) noexcept;

private:
	// This is just an example, the Mutation object probably shouldn't own a mutable store for the
	// reviews in a member variable.
	std::map<Episode, std::vector<std::shared_ptr<Review>>> reviews_;
};

} // namespace graphql::learn

#endif // MUTATIONDATA_H
