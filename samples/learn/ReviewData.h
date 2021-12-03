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
	explicit Review(int stars, std::optional<std::string>&& commentary) noexcept;

	int getStars() const noexcept;
	const std::optional<std::string>& getCommentary() const noexcept;

private:
	const int stars_;
	const std::optional<std::string> commentary_;
};

} // namespace graphql::learn

#endif // REVIEWDATA_H
