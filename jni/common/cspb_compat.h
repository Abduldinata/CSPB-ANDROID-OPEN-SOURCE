#ifndef CSPB_COMPAT_H
#define CSPB_COMPAT_H

/**
 * CSPB Compatibility Header
 * Digunakan untuk menjembatani SDK Valve 2002 lama dengan SDK Xash3D Modern.
 */

#include "xash3d_types.h"

// 1. Core Color Types (Moved here to ensure availability for render_api.h and dlight.h)
typedef struct
{
	byte	r, g, b;
} color24;

typedef struct
{
	unsigned	r, g, b, a;
} colorVec;

#include "render_api.h"

// 2. Math Bridging (Ensuring external visibility while suppressing modern macros)
#ifndef HAVE_VEC3_ORIGIN
#define HAVE_VEC3_ORIGIN
#endif

#ifndef __cplusplus
// C Files: Suppress engine math inlines and use legacy mathlib.h definitions
#ifndef HAVE_VECTORCOMPARE
#define HAVE_VECTORCOMPARE
#endif
#ifndef HAVE_VECTORNORMALIZE
#define HAVE_VECTORNORMALIZE
#endif
#ifndef HAVE_VECTORMA
#define HAVE_VECTORMA
#endif
#ifndef HAVE_VECTORSCALE
#define HAVE_VECTORSCALE
#endif
#ifndef HAVE_ANGLEMOD
#define HAVE_ANGLEMOD
#endif
#ifndef HAVE_ANGLEVECTORS
#define HAVE_ANGLEVECTORS
#endif
#ifndef HAVE_BOXONPLANESIDE
#define HAVE_BOXONPLANESIDE
#endif
#ifndef HAVE_DOTPRODUCT
#define HAVE_DOTPRODUCT
#endif
#ifndef HAVE_CROSSPRODUCT
#define HAVE_CROSSPRODUCT
#endif
#ifndef HAVE_VECTORLENGTH
#define HAVE_VECTORLENGTH
#endif
#ifndef HAVE_VECTORDISTANCE
#define HAVE_VECTORDISTANCE
#endif
#ifndef HAVE_VECTORINVERSE
#define HAVE_VECTORINVERSE
#endif
#ifndef HAVE_VECTORNEGATE
#define HAVE_VECTORNEGATE
#endif
#endif // __cplusplus

#ifndef MPLANE_T_DEFINED
#define MPLANE_T_DEFINED
typedef struct mplane_s
{
	vec3_t  normal;
	float   dist;
	byte    type;                   // for texture axis selection and fast side tests
	byte    signbits;               // signx + signy<<1 + signz<<1
	byte    pad[2];
} mplane_t;
#endif

// 3. dlight_t redefinition
typedef struct dlight_s dlight_t;

// 2. PARM_TEX_TYPE (Di SDK baru berubah menjadi PARM_TEX_DEPTH atau lainnya)
#ifndef PARM_TEX_TYPE
#define PARM_TEX_TYPE 11 // Mapping ke nilai lama (11) yang sekarang adalah PARM_TEX_DEPTH
#endif

// 3. Texture Flags (TF_*) yang hilang di SDK baru
#ifndef TF_NOPICMIP
#define TF_NOPICMIP (1<<4) // Abaikan r_picmip
#endif

#ifndef TF_UNCOMPRESSED
#define TF_UNCOMPRESSED (1<<1) // Identik dengan TF_KEEP_SOURCE di SDK baru
#endif

#include "dlight.h"

// 4. CS 1.6 extensions
#ifndef FTENT_IGNOREGRAVITY
#define FTENT_IGNOREGRAVITY 0x00200000
#endif

// 5. MAX_MODEL_NAME
#ifndef MAX_MODEL_NAME
#define MAX_MODEL_NAME 64
#endif

// 6. Content type aliases (Fixing missing 'S' in old mod code)
#ifndef CONTENT_WATER
#define CONTENT_WATER CONTENTS_WATER
#endif
#ifndef CONTENT_SLIME
#define CONTENT_SLIME CONTENTS_SLIME
#endif
#ifndef CONTENT_LAVA
#define CONTENT_LAVA CONTENTS_LAVA
#endif
#ifndef CONTENT_EMPTY
#define CONTENT_EMPTY CONTENTS_EMPTY
#endif
#ifndef CONTENT_SOLID
#define CONTENT_SOLID CONTENTS_SOLID
#endif
#ifndef CONTENT_SKY
#define CONTENT_SKY CONTENTS_SKY
#endif

#endif // CSPB_COMPAT_H
