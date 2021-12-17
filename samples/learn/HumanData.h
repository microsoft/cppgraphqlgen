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
	explicit Human(std::string&& id, std::optional<std::string>&& name,
		std::vector<Episode>&& appearsIn, std::optional<std::string>&& homePlanet) noexcept;

	void addFriends(std::vector<SharedHero> friends) noexcept;

	const std::string& getId() const noexcept;
	const std::optional<std::string>& getName() const noexcept;
	std::optional<std::vector<std::shared_ptr<object::Character>>> getFriends() const noexcept;
	std::optional<std::vector<std::optional<Episode>>> getAppearsIn() const noexcept;
	const std::optional<std::string>& getHomePlanet() const noexcept;

private:
	const std::string id_;
	const std::optional<std::string> name_;
	const std::vector<Episode> appearsIn_;
	const std::optional<std::string> homePlanet_;

	std::vector<WeakHero> friends_;
};

} // namespace graphql::learn

#endif // HUMANDATA_H
