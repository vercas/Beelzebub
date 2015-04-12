#pragma once

#include <metaprogramming.h>

#ifdef __cplusplus

namespace Beelzebub
{
	struct Handle
	{
		size_t val;
	} __attribute__((packed));

	enum class Result : u32
	{
		Fatal				= 0x70000000,
		Okay				= 0,

		ArgumentOutOfRange	= 0x10,
	};
}

#else

typedef struct Handle_t
{
	size_t val;
} __attribute__((packed)) Handle;

typedef u32 Result;

#define RES_FATAL					(0x70000000U)
#define RES_OKAY					(0U)

#define RES_ARGUMENT_OUT_OF_RANGE	(0x10U)

#endif
