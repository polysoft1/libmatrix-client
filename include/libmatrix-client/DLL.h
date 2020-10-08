#ifndef LIBMATRIX_DLL_H
#define LIBMATRIX_DLL_H

/**
 * This is used for DLL exporting.
 */
#if defined (WIN32) && defined (BUILD_SHARED_LIBS)
#if defined (_MSC_VER)
#pragma warning(disable: 4251)
#endif
#if defined(LibMatrix_EXPORT)
#define LIBMATRIX_DLL_EXPORT __declspec(dllexport)
 //#pragma message("dll export")
#else
#define LIBMATRIX_DLL_EXPORT __declspec(dllimport)
 //#pragma message("dll import")
#endif
#else
#define LIBMATRIX_DLL_EXPORT
#endif

#endif /* end of include guard: DLLS */
