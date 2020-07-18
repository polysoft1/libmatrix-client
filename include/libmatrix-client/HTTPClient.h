#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

// All included headers should typedef their class to make it HTTPSession

#ifdef USE_CUSTOM_HTTP_SESSION

	#include "CustomHTTPClient.h"

#elif defined(USE_CPPRESTSDK)

	#include "CPPRESTSDKClient.h"

#else

	#error A HTTP Session must be specified at compile time!

#endif

#endif
