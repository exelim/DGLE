/**
\author		Korotkov Andrey aka DRON
\date		xx.xx.2012 (c)Korotkov Andrey

This file is a part of DGLE project and is distributed
under the terms of the GNU Lesser General Public License.
See "DGLE.h" for more details.
*/

#pragma once

#include "DGLE.h"

#define USE_GLEW_HEADER
#define OPENGL_LEGACY_BASE_OBJECTS
#include "DGLE_CoreRenderer.h"

using namespace DGLE;

// Platform specific //

#ifdef PLATFORM_WINDOWS

typedef HMODULE TDynLibHandle;

#define PLUGIN_FILE_EXTENSION ".dll"

#if defined(_MSC_VER)
#	ifdef _WIN64
//#		pragma warning(disable : 4267)
//#		pragma warning(disable : 4244)
#	endif
#endif

#else //PLATFORM_WINDOWS

#define MAX_PATH 260
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((uint32)(uchar)(ch0) | ((uint32)(uchar)(ch1) << 8) | ((uint32)(uchar)(ch2) << 16) | ((uint32)(uchar)(ch3) << 24))

#endif

// Standart includes //

//#include <new>
//#include <new.h>
//#include <exception>
#include <assert.h>
#include <string>
//#include <list>
#include <vector>
//#include <set>
#include <map>
//#include <bitset>
#include <algorithm>
#include <fstream>
#include <strstream>
#include <limits>
#include <type_traits>
#include <io.h>

#include "Utils.h"
#include "FuncDelegate.h"

#define ENGINE_PLATFORM_BASE
#include "PlatformAPI.h"

class CCore;
class CConsole;

struct TEngInstance
{
	E_GET_ENGINE_FLAGS
				eGetEngFlags;
	CConsole	*pclConsole;
	CCore		*pclCore;

	TEngInstance():
		eGetEngFlags(GEF_DEFAULT),
		pclConsole(NULL),
		pclCore(NULL)
	{}
};

TEngInstance* EngineInstance(uint uiInstIdx);

class CInstancedObj
{
	const uint _uiInstIdx;

protected:

	CInstancedObj(uint uiInstIdx):_uiInstIdx(uiInstIdx){}

public:

	inline uint InstIdx() const 
	{
		return _uiInstIdx;
	}
	
	inline CConsole* Console() const 
	{
		return EngineInstance(_uiInstIdx)->pclConsole;
	}

	inline CCore* Core() const 
	{
		return EngineInstance(_uiInstIdx)->pclCore;
	}
};

#include "Core.h"
#include "Console.h"

// Build options //

//Uncomment define below to exclude OpenGL legacy builtin renderer
//If NO_BUILTIN_RENDERER is uncommented you also must exclude glew.c from build.
//#define NO_BUILTIN_RENDERER

//Uncomment define below to exclude builtin sound realisation
//#define NO_BUILTIN_SOUND

//Uncomment define below to exclude builtin input realisation
//#define NO_BUILTIN_INPUT

// Platform specific build options //

#ifdef PLATFORM_WINDOWS

//Uncomment define below to build engine without using any from DirectX
//#define NO_DIRECTX

#	ifndef _DEBUG
//Uncomment define below to use PDB files based exception filter and get call stack on fatals
//#	define PDB_DEBUG
#		if defined(DGLE_USE_COM) && !defined(NO_DIRECTX)
#			define DXDIAG_VIDEO_INFO
#		endif
#	endif

#endif //PLATFORM_WINDOWS

// Defines //

#define DGLE_VERSION (string(_DGLE_VER_" (")+string(__TIMESTAMP__)+string(")")).c_str()

#ifdef _DEBUG

/*//DEBUG_NEW
#	include <stdlib.h>
#	include <crtdbg.h>
#	ifndef DEBUG_NEW
#		define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#		define new DEBUG_NEW
#	endif*/

#endif

// Externs //

void LogWrite(uint uiInstIdx, const char *pcTxt, E_LOG_TYPE eType, const char *pcSrcFileName, int iSrcLineNumber);

// Helper class //


// Helper functions //
/*
template<class Class, class Intarface>
inline bool CmpInterfaceTargets(const Intarface *left, const Intarface *right) throw(...)
{
	return *static_cast<const Class *>(left) == *static_cast<const Class *>(right);
}*/

//Macroses//

//#define RENDERGL Core()->pRenderGL()
//#define GL_SMAN RENDERGL->pSMan()
#define PTHIS(cl_name) (reinterpret_cast<cl_name *>(pParametr))
#define CON(cl_name, txt) PTHIS(cl_name)->Console()->Write(txt, false)
#define LOG(txt, type) LogWrite(this->InstIdx(), std::string(txt).c_str(), type, GetFileName(__FILE__).c_str(), __LINE__)
#define LOG2(cl_name, txt, type) LogWrite(PTHIS(cl_name)->InstIdx(), std::string(txt).c_str(), type, GetFileName(__FILE__).c_str(), __LINE__)

#define OUTPUT_HR_MESSAGE(message, hr, type) \
{\
	/* termination 0 counded in _countof()*/\
	char str[_countof(message ". hr: ") + numeric_limits<DGLE_RESULT>::digits10] = message ". hr: ";\
	_itoa(hr, str + _countof(message ". hr: ") - 1, 10);\
	LOG(str, type);\
}

#define IENGBASEOBJ_IMPLEMENTATION(object_type) \
	DGLE_RESULT DGLE_API Free()\
	{\
		bool can_delete;\
		Core()->pResMan()->RemoveResource(this, can_delete);\
		if (can_delete)\
		{\
			delete this;\
			return S_OK;\
		}\
		else\
			return S_FALSE;\
	}\
	DGLE_RESULT DGLE_API GetType(E_ENG_OBJ_TYPE &eObjType)\
	{\
		eObjType = object_type;\
		return S_OK;\
	}\
	DGLE_RESULT DGLE_API GetUnknownType(uint &uiObjUnknownType)\
	{\
		uiObjUnknownType = -1;\
		return S_FALSE;\
	}

/*
template<class Interface, typename Type, typename ReturnType = Type>
struct CComGetWrapper: public binary_function<Interface *, DGLE_RESULT (DGLE_API Interface::*)(Type &), ReturnType>
{
	result_type operator ()(first_argument_type object, second_argument_type method) const
	{
		Type data;
		CHECK_RES((object->*method)(data));
		return data;
	}
};*/
/*
inline const char *GlError2String(GLenum error)
{
	switch (error)
	{
	case GL_NO_ERROR:			return "GL_NO_ERROR";
	case GL_INVALID_ENUM:		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:	return "GL_INVALID_OPERATION";
	case GL_STACK_OVERFLOW:		return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW:	return "GL_STACK_UNDERFLOW";
	case GL_OUT_OF_MEMORY:		return "GL_OUT_OF_MEMORY";
	default:					return "UNKNOWN ERROR";
	}
}
*/
/*
// TODO: try C++ 0x typeof for operator () signature
#define LAZY(handler) \
class LAZY##handler\
{\
	bool _modified;\
public:\
	name(): _modified(true) {}\
	void operator ()()\
	{\
		handler();\
	}\
}\
*/