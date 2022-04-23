// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef DROIDDATA_H
#define DROIDDATA_H

#include "HeroData.h"

#include "DroidObject.h"

namespace graphql::learn {

class Human;

class Droid
{
public:
	explicit Droid(std::string&& id, std::optional<std::string>&& name,
		std::vector<Episode>&& appearsIn, std::optional<std::string>&& primaryFunction) noexcept;

	void addFriends(std::vector<SharedHero> friends) noexcept;

	const response::IdType& getId() const noexcept;
	const std::optional<std::string>& getName() const noexcept;
	std::optional<std::vector<std::shared_ptr<object::Character>>> getFriends() const noexcept;
	std::optional<std::vector<std::optional<Episode>>> getAppearsIn() const noexcept;
	const std::optional<std::string>& getPrimaryFunction() const noexcept;

private:
	const response::IdType id_;
	const std::optional<std::string> name_;
	const std::vector<Episode> appearsIn_;
	const std::optional<std::string> primaryFunction_;

	std::vector<WeakHero> friends_;
};

} // namespace graphql::learn

#endif // DROIDDATA_H
