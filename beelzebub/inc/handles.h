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
		Fatal                = 0x80000000U,

		Okay                 = 0x00U,
		UnsupportedOperation = 0x01U,
		NotImplemented       = 0x02U,

		ArgumentOutOfRange   = 0x10U,
		ArgumentNull         = 0x11U,
	};
}

#else

typedef struct Handle_t
{
	size_t val;
} __attribute__((packed)) Handle;

typedef u32 Result;

#define RES_FATAL                   (0x80000000U)

#define RES_OKAY                    (0x00U)
#define RES_UNSUPPORTED_OPERATION   (0x01U)
#define RES_NOT_IMPLEMENTED         (0x02U)

#define RES_ARGUMENT_OUT_OF_RANGE   (0x10U)

#endif
