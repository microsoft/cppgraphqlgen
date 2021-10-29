// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "HumanData.h"

#include "DroidData.h"

namespace graphql::learn {

Human::Human(response::StringType id, std::optional<response::StringType> name,
	std::vector<Episode> appearsIn, std::optional<response::StringType> homePlanet) noexcept
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
					return WeakHero { std::weak_ptr<std::decay_t<decltype(hero)>::element_type> {
						hero } };
				},
				spFriend);
		});
}

service::FieldResult<response::StringType> Human::getId(service::FieldParams&& params) const
{
	return { id_ };
}

service::FieldResult<std::optional<response::StringType>> Human::getName(
	service::FieldParams&& params) const
{
	return { name_ };
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<service::Object>>>> Human::
	getFriends(service::FieldParams&& params) const
{
	std::vector<std::shared_ptr<service::Object>> result(friends_.size());

	std::transform(friends_.begin(),
		friends_.end(),
		std::back_inserter(result),
		[](const auto& wpFriend) noexcept {
			return std::visit(
				[](const auto& hero) noexcept {
					return std::static_pointer_cast<service::Object>(hero.lock());
				},
				wpFriend);
		});
	result.erase(std::remove_if(result.begin(),
					 result.end(),
					 [](const auto& entry) noexcept {
						 return !entry;
					 }),
		result.end());

	return { result.empty() ? std::nullopt : std::make_optional(std::move(result)) };
}

service::FieldResult<std::optional<std::vector<std::optional<Episode>>>> Human::getAppearsIn(
	service::FieldParams&& params) const
{
	std::vector<std::optional<Episode>> result(appearsIn_.size());

	std::transform(appearsIn_.begin(),
		appearsIn_.end(),
		result.begin(),
		[](const auto& entry) noexcept {
			return std::make_optional(entry);
		});

	return { result.empty() ? std::nullopt : std::make_optional(std::move(result)) };
}

service::FieldResult<std::optional<response::StringType>> Human::getHomePlanet(
	service::FieldParams&& params) const
{
	return { homePlanet_ };
}

} // namespace graphql::learn
