// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef STARWARSDATA_H
#define STARWARSDATA_H

#include "graphqlservice/GraphQLService.h"

namespace graphql::star_wars {

std::shared_ptr<service::Object> GetQueryObject() noexcept;
std::shared_ptr<service::Object> GetMutationObject() noexcept;
std::shared_ptr<service::Object> GetSubscriptionObject() noexcept;

std::shared_ptr<service::Request> GetService() noexcept;

} // namespace graphql::star_wars

#endif // STARWARSDATA_H
