// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef HUMANDATA_H
#define HUMANDATA_H

#include "HeroData.h"

#include "HumanObject.h"

namespace graphql::learn {

class Human
{
public:
	explicit Human(response::StringType&& id, std::optional<response::StringType>&& name,
		std::vector<Episode>&& appearsIn,
		std::optional<response::StringType>&& homePlanet) noexcept;

	void addFriends(std::vector<SharedHero> friends) noexcept;

	const response::StringType& getId() const noexcept;
	const std::optional<response::StringType>& getName() const noexcept;
	std::optional<std::vector<std::shared_ptr<service::Object>>> getFriends() const noexcept;
	std::optional<std::vector<std::optional<Episode>>> getAppearsIn() const noexcept;
	const std::optional<response::StringType>& getHomePlanet() const noexcept;

private:
	const response::StringType id_;
	const std::optional<response::StringType> name_;
	const std::vector<Episode> appearsIn_;
	const std::optional<response::StringType> homePlanet_;

	std::vector<WeakHero> friends_;
};

} // namespace graphql::learn

#endif // HUMANDATA_H
