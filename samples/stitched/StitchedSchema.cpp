// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "StitchedSchema.h"

#include "StarWarsData.h"
#include "StarWarsSchema.h"

import GraphQL.Today.Mock;

namespace graphql::stitched {

std::shared_ptr<schema::Schema> GetSchema()
{
	static std::weak_ptr<schema::Schema> s_wpSchema;
	auto schema = s_wpSchema.lock();

	if (!schema)
	{
		auto learnSchema = learn::GetSchema();
		auto todaySchema = today::GetSchema();
		schema = learnSchema->StitchSchema(todaySchema);
		s_wpSchema = schema;
	}

	return schema;
}

class Operations final : public service::Request
{
public:
	explicit Operations(std::shared_ptr<service::Object> query,
		std::shared_ptr<service::Object> mutation, std::shared_ptr<service::Object> subscription);

private:
	std::shared_ptr<service::Object> _query;
	std::shared_ptr<service::Object> _mutation;
	std::shared_ptr<service::Object> _subscription;
};

Operations::Operations(std::shared_ptr<service::Object> query,
	std::shared_ptr<service::Object> mutation, std::shared_ptr<service::Object> subscription)
	: service::Request(
		  {
			  { service::strQuery, query },
			  { service::strMutation, mutation },
			  { service::strSubscription, subscription },
		  },
		  GetSchema())
	, _query(std::move(query))
	, _mutation(std::move(mutation))
	, _subscription(std::move(subscription))
{
}

std::shared_ptr<service::Request> GetService()
{
	auto learnQuery = star_wars::GetQueryObject();
	auto todayQuery = std::static_pointer_cast<service::Object>(
		std::make_shared<today::object::Query>(today::mock_query(today::mock_service())));
	auto stitchedQuery = learnQuery->StitchObject(todayQuery);

	auto learnMutation = star_wars::GetMutationObject();
	auto todayMutation = std::static_pointer_cast<service::Object>(
		std::make_shared<today::object::Mutation>(today::mock_mutation()));
	auto stitchedMutation = learnMutation->StitchObject(todayMutation);

	auto learnSubscription = star_wars::GetSubscriptionObject();
	auto todaySubscription = std::static_pointer_cast<service::Object>(
		std::make_shared<today::object::Subscription>(today::mock_subscription()));
	std::shared_ptr<service::Object> stitchedSubscription;

	if (learnSubscription)
	{
		if (todaySubscription)
		{
			stitchedSubscription = learnSubscription->StitchObject(todaySubscription);
		}
		else
		{
			stitchedSubscription = learnSubscription;
		}
	}
	else
	{
		stitchedSubscription = todaySubscription;
	}

	return std::make_shared<Operations>(stitchedQuery, stitchedMutation, stitchedSubscription);
}

} // namespace graphql::stitched
