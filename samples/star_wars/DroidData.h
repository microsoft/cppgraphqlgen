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
	explicit Droid(response::StringType id, std::optional<response::StringType> name,
		std::vector<Episode> appearsIn,
		std::optional<response::StringType> primaryFunction) noexcept;

	const response::StringType& id() const noexcept;
	void addFriends(std::vector<SharedHero> friends) noexcept;

	response::StringType getId(service::FieldParams&& params) const noexcept;
	std::optional<response::StringType> getName(service::FieldParams&& params) const noexcept;
	std::optional<std::vector<std::shared_ptr<service::Object>>> getFriends(
		service::FieldParams&& params) const noexcept;
	std::optional<std::vector<std::optional<Episode>>> getAppearsIn(
		service::FieldParams&& params) const noexcept;
	std::optional<response::StringType> getPrimaryFunction(
		service::FieldParams&& params) const noexcept;

private:
	const response::StringType id_;
	const std::optional<response::StringType> name_;
	const std::vector<Episode> appearsIn_;
	const std::optional<response::StringType> primaryFunction_;

	std::vector<WeakHero> friends_;
};

} // namespace graphql::learn

#endif // DROIDDATA_H
