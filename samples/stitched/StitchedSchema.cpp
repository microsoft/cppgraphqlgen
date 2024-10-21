// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "StitchedSchema.h"

#include "StarWarsData.h"
#include "StarWarsSchema.h"

import GraphQL.Today.Mock;

namespace graphql::stitched {

std::shared_ptr<const service::Request> GetService()
{
	return star_wars::GetService()->stitch(today::mock_service()->service);
}

} // namespace graphql::stitched
