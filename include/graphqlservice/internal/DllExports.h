// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef DLLEXPORTS_H
#define DLLEXPORTS_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLCLIENT_DLL
		#define GRAPHQLCLIENT_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLCLIENT_DLL
		#define GRAPHQLCLIENT_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLCLIENT_DLL

	#ifdef IMPL_GRAPHQLPEG_DLL
		#define GRAPHQLPEG_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLPEG_DLL
		#define GRAPHQLPEG_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLPEG_DLL

	#ifdef IMPL_GRAPHQLRESPONSE_DLL
		#define GRAPHQLRESPONSE_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLRESPONSE_DLL
		#define GRAPHQLRESPONSE_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLRESPONSE_DLL

	#ifdef IMPL_GRAPHQLSERVICE_DLL
		#define GRAPHQLSERVICE_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLSERVICE_DLL
		#define GRAPHQLSERVICE_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLSERVICE_DLL

	#ifdef IMPL_JSONRESPONSE_DLL
		#define JSONRESPONSE_EXPORT __declspec(dllexport)
	#else // !IMPL_JSONRESPONSE_DLL
		#define JSONRESPONSE_EXPORT __declspec(dllimport)
	#endif // !IMPL_JSONRESPONSE_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLCLIENT_EXPORT
	#define GRAPHQLPEG_EXPORT
	#define GRAPHQLRESPONSE_EXPORT
	#define GRAPHQLSERVICE_EXPORT
	#define JSONRESPONSE_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#endif // DLLEXPORTS_H
