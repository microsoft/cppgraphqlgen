// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CharacterObject.h"

#include "QueryData.h"

namespace graphql::learn {

Query::Query(std::map<Episode, SharedHero>&& heroes,
	std::map<response::IdType, std::shared_ptr<Human>>&& humans,
	std::map<response::IdType, std::shared_ptr<Droid>>&& droids) noexcept
	: heroes_ { std::move(heroes) }
	, humans_ { std::move(humans) }
	, droids_ { std::move(droids) }
{
}

std::shared_ptr<object::Character> Query::getHero(std::optional<Episode> episodeArg) const noexcept
{
	std::shared_ptr<object::Character> result;
	const auto episode = episodeArg ? *episodeArg : Episode::NEW_HOPE;

	if (const auto itr = heroes_.find(episode); itr != heroes_.end())
	{
		result = make_hero(itr->second);
	}

	return result;
}

std::shared_ptr<object::Human> Query::getHuman(const response::IdType& idArg) const noexcept
{
	std::shared_ptr<Human> result;

	if (const auto itr = humans_.find(idArg); itr != humans_.end())
	{
		result = itr->second;
	}

	return std::make_shared<object::Human>(std::move(result));
}

std::shared_ptr<object::Droid> Query::getDroid(const response::IdType& idArg) const noexcept
{
	std::shared_ptr<Droid> result;

	if (const auto itr = droids_.find(idArg); itr != droids_.end())
	{
		result = itr->second;
	}

	return std::make_shared<object::Droid>(std::move(result));
}

} // namespace graphql::learn
