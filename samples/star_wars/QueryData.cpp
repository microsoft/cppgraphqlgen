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

service::FieldResult<std::shared_ptr<service::Object>> Query::getHero(
	service::FieldParams&&, std::optional<Episode>&& episodeArg) const
{
	std::shared_ptr<service::Object> result;

	if (episodeArg)
	{
		if (const auto itr = heroes_.find(*episodeArg); itr != heroes_.end())
		{
			result = std::visit(
				[](const auto& hero) noexcept {
					return std::static_pointer_cast<service::Object>(hero);
				},
				itr->second);
		}
	}

	return { result };
}

service::FieldResult<std::shared_ptr<object::Human>> Query::getHuman(
	service::FieldParams&&, response::StringType&& idArg) const
{
	std::shared_ptr<object::Human> result;

	if (const auto itr = humans_.find(idArg); itr != humans_.end())
	{
		result = itr->second;
	}

	return { result };
}

service::FieldResult<std::shared_ptr<object::Droid>> Query::getDroid(
	service::FieldParams&&, response::StringType&& idArg) const
{
	std::shared_ptr<object::Droid> result;

	if (const auto itr = droids_.find(idArg); itr != droids_.end())
	{
		result = itr->second;
	}

	return { result };
}

} // namespace graphql::learn
