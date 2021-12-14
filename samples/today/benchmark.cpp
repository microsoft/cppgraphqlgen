// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayMock.h"

#include "graphqlservice/JSONResponse.h"

#include <chrono>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace graphql;

using namespace std::literals;

namespace {

response::IdType binAppointmentId;
response::IdType binTaskId;
response::IdType binFolderId;

} // namespace

std::shared_ptr<today::Operations> buildService()
{
	std::string fakeAppointmentId("fakeAppointmentId");
	binAppointmentId.resize(fakeAppointmentId.size());
	std::copy(fakeAppointmentId.cbegin(), fakeAppointmentId.cend(), binAppointmentId.begin());

	std::string fakeTaskId("fakeTaskId");
	binTaskId.resize(fakeTaskId.size());
	std::copy(fakeTaskId.cbegin(), fakeTaskId.cend(), binTaskId.begin());

	std::string fakeFolderId("fakeFolderId");
	binFolderId.resize(fakeFolderId.size());
	std::copy(fakeFolderId.cbegin(), fakeFolderId.cend(), binFolderId.begin());

	auto query = std::make_shared<today::Query>(
		[]() -> std::vector<std::shared_ptr<today::Appointment>> {
			return { std::make_shared<today::Appointment>(std::move(binAppointmentId),
				"tomorrow",
				"Lunch?",
				false) };
		},
		[]() -> std::vector<std::shared_ptr<today::Task>> {
			return { std::make_shared<today::Task>(std::move(binTaskId), "Don't forget", true) };
		},
		[]() -> std::vector<std::shared_ptr<today::Folder>> {
			return { std::make_shared<today::Folder>(std::move(binFolderId), "\"Fake\" Inbox", 3) };
		});
	auto mutation = std::make_shared<today::Mutation>(
		[](today::CompleteTaskInput&& input) -> std::shared_ptr<today::CompleteTaskPayload> {
			return std::make_shared<today::CompleteTaskPayload>(
				std::make_shared<today::Task>(std::move(input.id),
					"Mutated Task!",
					*(input.isComplete)),
				std::move(input.clientMutationId));
		});
	auto subscription = std::make_shared<today::Subscription>();
	auto service = std::make_shared<today::Operations>(query, mutation, subscription);

	return service;
}

void outputOverview(
	size_t iterations, const std::chrono::steady_clock::duration& totalDuration) noexcept
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
	const size_t iterations = [](const char* arg) noexcept -> size_t {
		if (arg)
		{
			const int parsed = std::atoi(arg);

			if (parsed > 0)
			{
				return static_cast<size_t>(parsed);
			}
		}

		// Default to 100 iterations
		return 100;
	}((argc > 1) ? argv[1] : nullptr);

	std::cout << "Iterations: " << iterations << std::endl;

	auto service = buildService();
	std::vector<std::chrono::steady_clock::duration> durationParse(iterations);
	std::vector<std::chrono::steady_clock::duration> durationValidate(iterations);
	std::vector<std::chrono::steady_clock::duration> durationResolve(iterations);
	std::vector<std::chrono::steady_clock::duration> durationToJson(iterations);
	const auto startTime = std::chrono::steady_clock::now();

	try
	{
		for (size_t i = 0; i < iterations; ++i)
		{
			const auto startParse = std::chrono::steady_clock::now();
			auto query = peg::parseString(R"gql(query {
				appointments {
					pageInfo { hasNextPage }
					edges {
						node {
							id
							when
							subject
							isNow
						}
					}
				}
			})gql"sv);
			const auto startValidate = std::chrono::steady_clock::now();

			service->validate(query);

			const auto startResolve = std::chrono::steady_clock::now();
			auto response = service->resolve({ query }).get();
			const auto startToJson = std::chrono::steady_clock::now();

			response::toJSON(std::move(response));

			const auto endToJson = std::chrono::steady_clock::now();

			durationParse[i] = startValidate - startParse;
			durationValidate[i] = startResolve - startValidate;
			durationResolve[i] = startToJson - startResolve;
			durationToJson[i] = endToJson - startToJson;
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

	outputSegment("Parse"sv, durationParse);
	outputSegment("Validate"sv, durationValidate);
	outputSegment("Resolve"sv, durationResolve);
	outputSegment("ToJSON"sv, durationToJson);

	return 0;
}
