// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "QueryData.h"

namespace graphql::learn {

Query::Query(std::map<Episode, SharedHero> heroes,
	std::map<response::StringType, std::shared_ptr<Human>> humans,
	std::map<response::StringType, std::shared_ptr<Droid>> droids) noexcept
	: heroes_ { std::move(heroes) }
	, humans_ { std::move(humans) }
	, droids_ { std::move(droids) }
{
}

std::shared_ptr<service::Object> Query::getHero(
	service::FieldParams&&, std::optional<Episode>&& episodeArg) const noexcept
{
	std::shared_ptr<service::Object> result;
	const auto episode = episodeArg ? *episodeArg : Episode::NEW_HOPE;

	if (const auto itr = heroes_.find(episode); itr != heroes_.end())
	{
		result = std::visit(
			[](const auto& hero) noexcept {
				using hero_t = std::decay_t<decltype(hero)>;

				if constexpr (std::is_same_v<std::shared_ptr<Human>, hero_t>)
				{
					return std::static_pointer_cast<service::Object>(
						std::make_shared<object::Human>(hero));
				}
				else if constexpr (std::is_same_v<std::shared_ptr<Droid>, hero_t>)
				{
					return std::static_pointer_cast<service::Object>(
						std::make_shared<object::Droid>(hero));
				}
			},
			itr->second);
	}

	return result;
}

std::shared_ptr<object::Human> Query::getHuman(
	service::FieldParams&&, response::StringType&& idArg) const noexcept
{
	std::shared_ptr<Human> result;

	if (const auto itr = humans_.find(idArg); itr != humans_.end())
	{
		result = itr->second;
	}

	return std::make_shared<object::Human>(std::move(result));
}

std::shared_ptr<object::Droid> Query::getDroid(
	service::FieldParams&&, response::StringType&& idArg) const noexcept
{
	std::shared_ptr<Droid> result;

	if (const auto itr = droids_.find(idArg); itr != droids_.end())
	{
		result = itr->second;
	}

	return std::make_shared<object::Droid>(std::move(result));
}

} // namespace graphql::learn
