// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>

import GraphQL.Client;
import GraphQL.Response;
import GraphQL.Service;

import GraphQL.Today.TodaySchema;

import GraphQL.Today.QueryObject;
import GraphQL.Today.MutationObject;
import GraphQL.Today.SubscriptionObject;

import GraphQL.Today.AppointmentEdgeObject;
import GraphQL.Today.AppointmentObject;
import GraphQL.Today.AppointmentConnectionObject;
import GraphQL.Today.CompleteTaskPayloadObject;
import GraphQL.Today.ExpensiveObject;
import GraphQL.Today.FolderEdgeObject;
import GraphQL.Today.FolderObject;
import GraphQL.Today.FolderConnectionObject;
import GraphQL.Today.NestedTypeObject;
import GraphQL.Today.NodeObject;
import GraphQL.Today.PageInfoObject;
import GraphQL.Today.TaskConnectionObject;
import GraphQL.Today.TaskEdgeObject;
import GraphQL.Today.TaskObject;
import GraphQL.Today.UnionTypeObject;

import GraphQL.Today.Mock;

import GraphQL.Benchmark.BenchmarkClient;

using namespace graphql;

using namespace std::literals;

void outputOverview(
	std::size_t iterations, const std::chrono::steady_clock::duration& totalDuration) noexcept
{
	const auto requestsPerSecond =
		((static_cast<double>(iterations)
			 * static_cast<double>(
				 std::chrono::duration_cast<std::chrono::steady_clock::duration>(1s).count()))
			/ static_cast<double>(totalDuration.count()));
	const auto averageRequest =
		(static_cast<double>(
			 std::chrono::duration_cast<std::chrono::microseconds>(totalDuration).count())
			/ static_cast<double>(iterations));

	std::cout << "Throughput: " << requestsPerSecond << " requests/second" << std::endl;

	std::cout << "Overall (microseconds): "
			  << std::chrono::duration_cast<std::chrono::microseconds>(totalDuration).count()
			  << " total, " << averageRequest << " average" << std::endl;
}

void outputSegment(
	std::string_view name, std::vector<std::chrono::steady_clock::duration>& durations) noexcept
{
	std::sort(durations.begin(), durations.end());

	const auto count = durations.size();
	const auto total =
		std::accumulate(durations.begin(), durations.end(), std::chrono::steady_clock::duration {});

	std::cout << name << " (microseconds): "
			  << std::chrono::duration_cast<std::chrono::microseconds>(durations[count / 2]).count()
			  << " median, "
			  << std::chrono::duration_cast<std::chrono::microseconds>(durations.front()).count()
			  << " minimum, "
			  << std::chrono::duration_cast<std::chrono::microseconds>(durations.back()).count()
			  << " maximum, "
			  << (static_cast<double>(
					  std::chrono::duration_cast<std::chrono::microseconds>(total).count())
					 / static_cast<double>(count))
			  << " average" << std::endl;
}

int main(int argc, char** argv)
{
	const std::size_t iterations = [](const char* arg) noexcept -> std::size_t {
		if (arg)
		{
			const int parsed = std::atoi(arg);

			if (parsed > 0)
			{
				return static_cast<std::size_t>(parsed);
			}
		}

		// Default to 100 iterations
		return 100;
	}((argc > 1) ? argv[1] : nullptr);

	std::cout << "Iterations: " << iterations << std::endl;

	const auto mockService = today::mock_service();
	const auto& service = mockService->service;
	std::vector<std::chrono::steady_clock::duration> durationResolve(iterations);
	std::vector<std::chrono::steady_clock::duration> durationParseServiceResponse(iterations);
	std::vector<std::chrono::steady_clock::duration> durationParseResponse(iterations);
	const auto startTime = std::chrono::steady_clock::now();

	try
	{
		using namespace client::query::Query;

		auto query = GetRequestObject();
		const auto& name = GetOperationName();

		for (std::size_t i = 0; i < iterations; ++i)
		{
			const auto startResolve = std::chrono::steady_clock::now();
			auto response = service->resolve({ query, name }).get();
			const auto startParseServiceResponse = std::chrono::steady_clock::now();
			auto serviceResponse = client::parseServiceResponse(std::move(response));
			const auto startParseResponse = std::chrono::steady_clock::now();
			const auto parsed = parseResponse(std::move(serviceResponse.data));
			const auto endParseResponse = std::chrono::steady_clock::now();

			durationResolve[i] = startParseServiceResponse - startResolve;
			durationParseServiceResponse[i] = startParseResponse - startParseServiceResponse;
			durationParseResponse[i] = endParseResponse - startParseResponse;
		}
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	const auto endTime = std::chrono::steady_clock::now();
	const auto totalDuration = endTime - startTime;

	outputOverview(iterations, totalDuration);

	outputSegment("Resolve"sv, durationResolve);
	outputSegment("ParseServiceResponse"sv, durationParseServiceResponse);
	outputSegment("ParseResponse"sv, durationParseResponse);

	return 0;
}
