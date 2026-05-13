#pragma once

#include "xash3d_types.h"

#ifndef ALLOC_CHECK
#define ALLOC_CHECK(x)
#endif

typedef struct dlight_s dlight_t;

#include "../public/render_api.h"

// Legacy aliases kept for older client code that still uses the pre-37 names.
#ifndef PARM_TEX_TYPE
#define PARM_TEX_TYPE PARM_TEX_DEPTH
#endif

#ifndef TF_NOPICMIP
#define TF_NOPICMIP (1 << 4)
#endif

#ifndef TF_UNCOMPRESSED
#define TF_UNCOMPRESSED TF_KEEP_SOURCE
#endif

#ifndef TEX_CUSTOM
#define TEX_CUSTOM 0
#endif
