// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DroidData.h"

#include "HumanData.h"

namespace graphql::learn {

Droid::Droid(response::StringType id, std::optional<response::StringType> name,
	std::vector<Episode> appearsIn, std::optional<response::StringType> primaryFunction) noexcept
	: id_ { std::move(id) }
	, name_ { std::move(name) }
	, appearsIn_ { std::move(appearsIn) }
	, primaryFunction_ { std::move(primaryFunction) }
{
}

const response::StringType& Droid::id() const noexcept
{
	return id_;
}

void Droid::addFriends(
	std::vector<std::variant<std::shared_ptr<Human>, std::shared_ptr<Droid>>> friends) noexcept
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

response::StringType Droid::getId() const noexcept
{
	return id_;
}

std::optional<response::StringType> Droid::getName() const noexcept
{
	return name_;
}

std::optional<std::vector<std::shared_ptr<service::Object>>> Droid::getFriends() const noexcept
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

std::optional<std::vector<std::optional<Episode>>> Droid::getAppearsIn() const noexcept
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

std::optional<response::StringType> Droid::getPrimaryFunction() const noexcept
{
	return primaryFunction_;
}

} // namespace graphql::learn
