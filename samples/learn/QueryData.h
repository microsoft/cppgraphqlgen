// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef QUERYDATA_H
#define QUERYDATA_H

#include "QueryObject.h"

#include "DroidData.h"
#include "HumanData.h"

namespace graphql::learn {

class Query
{
public:
	explicit Query(std::map<Episode, SharedHero>&& heroes,
		std::map<response::IdType, std::shared_ptr<Human>>&& humans,
		std::map<response::IdType, std::shared_ptr<Droid>>&& droids) noexcept;

	std::shared_ptr<object::Character> getHero(std::optional<Episode> episodeArg) const noexcept;
	std::shared_ptr<object::Human> getHuman(const response::IdType& idArg) const noexcept;
	std::shared_ptr<object::Droid> getDroid(const response::IdType& idArg) const noexcept;

private:
	const std::map<Episode, SharedHero> heroes_;
	const std::map<response::IdType, std::shared_ptr<Human>> humans_;
	const std::map<response::IdType, std::shared_ptr<Droid>> droids_;
};

} // namespace graphql::learn

#endif // QUERYDATA_H
