// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "HumanData.h"

#include "DroidData.h"

namespace graphql::learn {

Human::Human(response::StringType&& id, std::optional<response::StringType>&& name,
	std::vector<Episode>&& appearsIn, std::optional<response::StringType>&& homePlanet) noexcept
	: id_ { std::move(id) }
	, name_ { std::move(name) }
	, appearsIn_ { std::move(appearsIn) }
	, homePlanet_ { std::move(homePlanet) }
{
}

const response::StringType& Human::id() const noexcept
{
	return id_;
}

void Human::addFriends(std::vector<SharedHero> friends) noexcept
{
	friends_.resize(friends.size());

	std::transform(friends.begin(),
		friends.end(),
		friends_.begin(),
		[](const auto& spFriend) noexcept {
			return std::visit(
				[](const auto& hero) noexcept {
					return WeakHero {
						std::weak_ptr<typename std::decay_t<decltype(hero)>::element_type> { hero }
					};
				},
				spFriend);
		});
}

response::StringType Human::getId() const noexcept
{
	return id_;
}

std::optional<response::StringType> Human::getName() const noexcept
{
	return name_;
}

std::optional<std::vector<std::shared_ptr<service::Object>>> Human::getFriends() const noexcept
{
	std::vector<std::shared_ptr<service::Object>> result(friends_.size());

	std::transform(friends_.begin(),
		friends_.end(),
		result.begin(),
		[](const auto& wpFriend) noexcept {
			return std::visit(
				[](const auto& hero) noexcept {
					using hero_t = std::decay_t<decltype(hero)>;

					if constexpr (std::is_same_v<std::weak_ptr<Human>, hero_t>)
					{
						return std::static_pointer_cast<service::Object>(
							std::make_shared<object::Human>(hero.lock()));
					}
					else if constexpr (std::is_same_v<std::weak_ptr<Droid>, hero_t>)
					{
						return std::static_pointer_cast<service::Object>(
							std::make_shared<object::Droid>(hero.lock()));
					}
				},
				wpFriend);
		});
	result.erase(std::remove_if(result.begin(),
					 result.end(),
					 [](const auto& entry) noexcept {
						 return !entry;
					 }),
		result.end());

	return result.empty() ? std::nullopt : std::make_optional(std::move(result));
}

std::optional<std::vector<std::optional<Episode>>> Human::getAppearsIn() const noexcept
{
	std::vector<std::optional<Episode>> result(appearsIn_.size());

	std::transform(appearsIn_.begin(),
		appearsIn_.end(),
		result.begin(),
		[](const auto& entry) noexcept {
			return std::make_optional(entry);
		});

	return result.empty() ? std::nullopt : std::make_optional(std::move(result));
}

std::optional<response::StringType> Human::getHomePlanet() const noexcept
{
	return homePlanet_;
}

} // namespace graphql::learn
