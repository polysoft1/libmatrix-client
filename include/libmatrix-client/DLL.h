#ifndef DLL
#define DLL

/**
 * This is used for DLL exporting.
 */
#if defined (WIN32) && defined (BUILD_SHARED_LIBS)
#if defined (_MSC_VER)
#pragma warning(disable: 4251)
#endif
#if defined(DLL_EXPORT)
#define DLL_EXPORT __declspec(dllexport)
 //#pragma message("dll export")
#else
#define DLL_EXPORT __declspec(dllimport) __stdcall
 //#pragma message("dll import")
#endif
#else
#define DLL_EXPORT
#endif

#endif /* end of include guard: DLLS */
