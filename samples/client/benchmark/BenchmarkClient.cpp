// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "BenchmarkClient.h"

#include "graphqlservice/internal/SortedMap.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace graphql::client {
namespace benchmark {

const std::string& GetRequestText() noexcept
{
	static const auto s_request = R"gql(
		# Copyright (c) Microsoft Corporation. All rights reserved.
		# Licensed under the MIT License.
		
		query {
		  appointments {
		    pageInfo {
		      hasNextPage
		    }
		    edges {
		      node {
		        id
		        when
		        subject
		        isNow
		      }
		    }
		  }
		}
	)gql"s;

	return s_request;
}

const peg::ast& GetRequestObject() noexcept
{
	static const auto s_request = []() noexcept {
		auto ast = peg::parseString(GetRequestText());

		// This has already been validated against the schema by clientgen.
		ast.validated = true;

		return ast;
	}();

	return s_request;
}

} // namespace benchmark

using namespace benchmark;

template <>
query::Query::Response::appointments_AppointmentConnection::pageInfo_PageInfo Response<query::Query::Response::appointments_AppointmentConnection::pageInfo_PageInfo>::parse(response::Value&& response)
{
	query::Query::Response::appointments_AppointmentConnection::pageInfo_PageInfo result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(hasNextPage)js"sv)
			{
				result.hasNextPage = ModifiedResponse<bool>::parse(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

template <>
query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge::node_Appointment Response<query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge::node_Appointment>::parse(response::Value&& response)
{
	query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge::node_Appointment result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(id)js"sv)
			{
				result.id = ModifiedResponse<response::IdType>::parse(std::move(member.second));
				continue;
			}
			if (member.first == R"js(when)js"sv)
			{
				result.when = ModifiedResponse<response::Value>::parse<TypeModifier::Nullable>(std::move(member.second));
				continue;
			}
			if (member.first == R"js(subject)js"sv)
			{
				result.subject = ModifiedResponse<std::string>::parse<TypeModifier::Nullable>(std::move(member.second));
				continue;
			}
			if (member.first == R"js(isNow)js"sv)
			{
				result.isNow = ModifiedResponse<bool>::parse(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

template <>
query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge Response<query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge>::parse(response::Value&& response)
{
	query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(node)js"sv)
			{
				result.node = ModifiedResponse<query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge::node_Appointment>::parse<TypeModifier::Nullable>(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

template <>
query::Query::Response::appointments_AppointmentConnection Response<query::Query::Response::appointments_AppointmentConnection>::parse(response::Value&& response)
{
	query::Query::Response::appointments_AppointmentConnection result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(pageInfo)js"sv)
			{
				result.pageInfo = ModifiedResponse<query::Query::Response::appointments_AppointmentConnection::pageInfo_PageInfo>::parse(std::move(member.second));
				continue;
			}
			if (member.first == R"js(edges)js"sv)
			{
				result.edges = ModifiedResponse<query::Query::Response::appointments_AppointmentConnection::edges_AppointmentEdge>::parse<TypeModifier::Nullable, TypeModifier::List, TypeModifier::Nullable>(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

namespace query::Query {

const std::string& GetOperationName() noexcept
{
	static const auto s_name = R"gql()gql"s;

	return s_name;
}

Response parseResponse(response::Value&& response)
{
	Response result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(appointments)js"sv)
			{
				result.appointments = ModifiedResponse<query::Query::Response::appointments_AppointmentConnection>::parse(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

[[nodiscard("unnecessary call")]] const std::string& Traits::GetRequestText() noexcept
{
	return benchmark::GetRequestText();
}

[[nodiscard("unnecessary call")]] const peg::ast& Traits::GetRequestObject() noexcept
{
	return benchmark::GetRequestObject();
}

[[nodiscard("unnecessary call")]] const std::string& Traits::GetOperationName() noexcept
{
	return Query::GetOperationName();
}

[[nodiscard("unnecessary conversion")]] Traits::Response Traits::parseResponse(response::Value&& response)
{
	return Query::parseResponse(std::move(response));
}

} // namespace query::Query
} // namespace graphql::client
