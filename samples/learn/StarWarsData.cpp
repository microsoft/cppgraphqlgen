// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "StarWarsData.h"

#include "DroidData.h"
#include "HumanData.h"
#include "MutationData.h"
#include "QueryData.h"
#include "ReviewData.h"

#include "StarWarsObjects.h"

using namespace std::literals;

namespace graphql::star_wars {

std::shared_ptr<service::Request> GetService() noexcept
{
	auto luke = std::make_shared<learn::Human>("1000"s,
		std::make_optional("Luke Skywalker"s),
		std::vector { learn::Episode::NEW_HOPE, learn::Episode::EMPIRE, learn::Episode::JEDI },
		std::make_optional("Tatooine"s));

	auto vader = std::make_shared<learn::Human>("1001"s,
		std::make_optional("Darth Vader"s),
		std::vector { learn::Episode::NEW_HOPE, learn::Episode::EMPIRE, learn::Episode::JEDI },
		std::make_optional("Tatooine"s));

	auto han = std::make_shared<learn::Human>("1002"s,
		std::make_optional("Han Solo"s),
		std::vector { learn::Episode::NEW_HOPE, learn::Episode::EMPIRE, learn::Episode::JEDI },
		std::nullopt);

	auto leia = std::make_shared<learn::Human>("1003"s,
		std::make_optional("Leia Organa"s),
		std::vector { learn::Episode::NEW_HOPE, learn::Episode::EMPIRE, learn::Episode::JEDI },
		std::make_optional("Alderaan"s));

	auto tarkin = std::make_shared<learn::Human>("1004"s,
		std::make_optional("Wilhuff Tarkin"s),
		std::vector { learn::Episode::NEW_HOPE },
		std::nullopt);

	auto threepio = std::make_shared<learn::Droid>("2000"s,
		std::make_optional("C-3PO"s),
		std::vector { learn::Episode::NEW_HOPE, learn::Episode::EMPIRE, learn::Episode::JEDI },
		std::make_optional("Protocol"s));

	auto artoo = std::make_shared<learn::Droid>("2001"s,
		std::make_optional("R2-D2"s),
		std::vector { learn::Episode::NEW_HOPE, learn::Episode::EMPIRE, learn::Episode::JEDI },
		std::make_optional("Astromech"s));

	luke->addFriends({
		{ han },
		{ leia },
		{ threepio },
		{ artoo },
	});

	vader->addFriends({
		{ tarkin },
	});

	han->addFriends({
		{ luke },
		{ leia },
		{ artoo },
	});

	leia->addFriends({
		{ luke },
		{ han },
		{ threepio },
		{ artoo },
	});

	tarkin->addFriends({
		{ vader },
	});

	threepio->addFriends({
		{ luke },
		{ han },
		{ leia },
		{ artoo },
	});

	artoo->addFriends({
		{ luke },
		{ han },
		{ leia },
	});

	std::map<learn::Episode, learn::SharedHero> heroes {
		{ learn::Episode::NEW_HOPE, { artoo } },
		{ learn::Episode::EMPIRE, { luke } },
		{ learn::Episode::JEDI, { artoo } },
	};

	std::map<response::StringType, std::shared_ptr<learn::Human>> humans {
		{ luke->id(), luke },
		{ vader->id(), vader },
		{ han->id(), han },
		{ leia->id(), leia },
		{ tarkin->id(), tarkin },
	};

	std::map<response::StringType, std::shared_ptr<learn::Droid>> droids {
		{ threepio->id(), threepio },
		{ artoo->id(), artoo },
	};

	auto query =
		std::make_shared<learn::Query>(std::move(heroes), std::move(humans), std::move(droids));
	auto mutation = std::make_shared<learn::Mutation>();
	auto service = std::make_shared<learn::Operations>(std::move(query), std::move(mutation));

	return service;
}

} // namespace graphql::star_wars
