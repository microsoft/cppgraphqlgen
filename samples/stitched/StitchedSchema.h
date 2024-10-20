// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef STITCHEDSCHEMA_H
#define STITCHEDSCHEMA_H

#include "graphqlservice/GraphQLService.h"

#include "StarWarsSharedTypes.h"
#include "TodaySharedTypes.h"

namespace graphql::stitched {

std::shared_ptr<service::Request> GetService();

} // namespace graphql::stitched

#endif // STITCHEDSCHEMA_H
