// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "graphqlservice/internal/Grammar.h"

#include <tao/pegtl/contrib/analyze.hpp>

#include <cstddef>

using namespace graphql;
using namespace graphql::peg;

using namespace tao::graphqlpeg;

TEST(PegtlCombinedCase, AnalyzeMixedGrammar)
{
	ASSERT_EQ(std::size_t { 0 }, analyze<mixed_document>(true))
		<< "there shouldn't be any infinite loops in the PEG version of the grammar";
}
