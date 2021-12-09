// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "HeroData.h"

#include "DroidData.h"
#include "HumanData.h"

#include "CharacterObject.h"

namespace graphql::learn {

std::shared_ptr<object::Character> make_hero(const SharedHero& hero) noexcept
{
	return std::visit(
		[](const auto& hero) noexcept {
			using hero_t = std::decay_t<decltype(hero)>;

			if constexpr (std::is_same_v<std::shared_ptr<Human>, hero_t>)
			{
				return std::make_shared<object::Character>(std::make_shared<object::Human>(hero));
			}
			else if constexpr (std::is_same_v<std::shared_ptr<Droid>, hero_t>)
			{
				return std::make_shared<object::Character>(std::make_shared<object::Droid>(hero));
			}
		},
		hero);
}

std::shared_ptr<object::Character> make_hero(const WeakHero& hero) noexcept
{
	return std::visit(
		[](const auto& hero) noexcept {
			using hero_t = std::decay_t<decltype(hero)>;

			if constexpr (std::is_same_v<std::weak_ptr<Human>, hero_t>)
			{
				return std::make_shared<object::Character>(
					std::make_shared<object::Human>(hero.lock()));
			}
			else if constexpr (std::is_same_v<std::weak_ptr<Droid>, hero_t>)
			{
				return std::make_shared<object::Character>(
					std::make_shared<object::Droid>(hero.lock()));
			}
		},
		hero);
}

} // namespace graphql::learn
