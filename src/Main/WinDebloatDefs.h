// include this before including ANY windows API headers
#pragma once

#ifdef NOMINMAX
#undef NOMINMAX
#endif
#ifdef NOWINRES
#undef NOWINRES
#endif
#ifdef NOSERVICE
#undef NOSERVICE
#endif
#ifdef NOMCX
#undef NOMCX
#endif
#ifdef NOCRYPT
#undef NOCRYPT
#endif
#ifdef NOMETAFILE
#undef NOMETAFILE
#endif
#ifdef MMNOSOUND
#undef MMNOSOUND
#endif
#ifdef VC_EXTRALEAN
#undef VC_EXTRALEAN
#endif
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#define NOMINMAX
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOCRYPT
#define NOMETAFILE
#define MMNOSOUND

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
