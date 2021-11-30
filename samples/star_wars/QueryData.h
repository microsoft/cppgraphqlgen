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
	explicit Query(std::map<Episode, SharedHero> heroes,
		std::map<response::StringType, std::shared_ptr<Human>> humans,
		std::map<response::StringType, std::shared_ptr<Droid>> droids) noexcept;

	service::FieldResult<std::shared_ptr<service::Object>> getHero(
		service::FieldParams&& params, std::optional<Episode>&& episodeArg) const;
	service::FieldResult<std::shared_ptr<object::Human>> getHuman(
		service::FieldParams&& params, response::StringType&& idArg) const;
	service::FieldResult<std::shared_ptr<object::Droid>> getDroid(
		service::FieldParams&& params, response::StringType&& idArg) const;

private:
	const std::map<Episode, SharedHero> heroes_;
	const std::map<response::StringType, std::shared_ptr<Human>> humans_;
	const std::map<response::StringType, std::shared_ptr<Droid>> droids_;
};

} // namespace graphql::learn

#endif // QUERYDATA_H
