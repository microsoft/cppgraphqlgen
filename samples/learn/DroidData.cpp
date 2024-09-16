// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DroidData.h"

#include "HumanData.h"

namespace graphql::learn {

Droid::Droid(std::string&& id, std::optional<std::string>&& name, std::vector<Episode>&& appearsIn,
	std::optional<std::string>&& primaryFunction) noexcept
	: id_ { std::move(id) }
	, name_ { std::move(name) }
	, appearsIn_ { std::move(appearsIn) }
	, primaryFunction_ { std::move(primaryFunction) }
{
}

void Droid::addFriends(
	std::vector<std::variant<std::shared_ptr<Human>, std::shared_ptr<Droid>>> friends) noexcept
{
	friends_.resize(friends.size());

	std::ranges::transform(friends, friends_.begin(), [](const auto& spFriend) noexcept {
		return std::visit(
			[](const auto& hero) noexcept {
				return WeakHero {
					std::weak_ptr<typename std::decay_t<decltype(hero)>::element_type> { hero }
				};
			},
			spFriend);
	});
}

const response::IdType& Droid::getId() const noexcept
{
	return id_;
}

const std::optional<std::string>& Droid::getName() const noexcept
{
	return name_;
}

std::optional<std::vector<std::shared_ptr<object::Character>>> Droid::getFriends() const noexcept
{
	std::vector<std::shared_ptr<object::Character>> result(friends_.size());

	std::ranges::transform(friends_, result.begin(), [](const auto& wpFriend) noexcept {
		return make_hero(wpFriend);
	});
	result.erase(std::remove(result.begin(), result.end(), std::shared_ptr<object::Character> {}),
		result.end());

	return result.empty() ? std::nullopt : std::make_optional(std::move(result));
}

std::optional<std::vector<std::optional<Episode>>> Droid::getAppearsIn() const noexcept
{
	std::vector<std::optional<Episode>> result(appearsIn_.size());

	std::ranges::transform(appearsIn_, result.begin(), [](const auto& entry) noexcept {
		return std::make_optional(entry);
	});

	return result.empty() ? std::nullopt : std::make_optional(std::move(result));
}

const std::optional<std::string>& Droid::getPrimaryFunction() const noexcept
{
	return primaryFunction_;
}

} // namespace graphql::learn
