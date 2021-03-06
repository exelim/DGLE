/**
\author		Korotkov Andrey aka DRON
\date		30.04.2012 (c)Korotkov Andrey

This file is a part of DGLE project and is distributed
under the terms of the GNU Lesser General Public License.
See "DGLE.h" for more details.
*/

#ifndef _RESOURCEMANAGER_H
#define _RESOURCEMANAGER_H

#include "Common.h"
//#include "LightAttribs.h"

class CBObjDummy;
class CBitmapFontDummy;
class CSSampleDummy;
class CMusicDummy;

struct TFileFormat
{
	std::string ext;
	std::string discr;
	E_ENG_OBJ_TYPE type;
	void *pParametr;
	bool (DGLE_API *pLoadProc)(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr);
};

struct TDefaultRes
{
	E_ENG_OBJ_TYPE type;
	IEngBaseObj *pBaseObj;
	TDefaultRes(E_ENG_OBJ_TYPE _type, IEngBaseObj *_pObj):type(_type), pBaseObj(_pObj){}
};

struct TResource
{
	char *pcName;
	uint32 nameHash;
	IEngBaseObj *pObj;
	
	TResource(const char *pcFullName, IEngBaseObj *pObject):
	pObj(pObject)
	{
		if (strlen(pcFullName) == 0)
		{
			pcName = NULL;
			nameHash = 0;
		}
		else
		{
			pcName = new char[strlen(pcFullName) + 1];
			strcpy(pcName, pcFullName);
			nameHash = GetCRC32((uint8*)pcFullName, (uint32)strlen(pcFullName) * sizeof(char));
		}
	}
};

//class CBasePfpMaterial;

class CResourceManager : public CInstancedObj, public IResourceManager
{
	// Don't forget to change this value and method when enum E_ENG_OBJ_TYPE is being changed.
	static const uint _sc_EngObjTypeCount = EOT_GUI_FORMS + 1;
	static void _s_GetObjTypeName(E_ENG_OBJ_TYPE type, std::string &name);

	TWinHandle				_stWnd;
	int						_iProfilerState;

	ICoreRenderer			*_pCoreRenderer;

	std::string				_strFileFormatsDescs;
	std::vector<TFileFormat>_clFileFormats;

	std::vector<TResource>	_resList;
	std::vector<TDefaultRes>_defRes;

	ITexture				*_pDefTex;
	IMaterial				*_pDefMaterial;
	IBitmapFont				*_pDefBmpFnt;
	IMesh					*_pDefMesh;

	CBObjDummy				*_pBObjDummy;
	CBitmapFontDummy		*_pDefBmFntDummy;
	CSSampleDummy			*_pDefSSmpDummy;
	
	inline DGLE_RESULT _Load(const char *pcFileName, IFile *pFile, uint uiFFIdx, IEngBaseObj *&prObj, uint uiLoadFlags);
	inline uint _GetFFIdx(const char *pcFileName, E_ENG_OBJ_TYPE eObjType, IEngBaseObj *&prObj);
	inline uint8 _GetBytesPerPixel(E_TEXTURE_DATA_FORMAT &format);
	uint _GenerateDecompressedTextureData(const uint8 *pDataIn, uint8 *&prDataOut, uint uiWidth, uint uiHeight, E_TEXTURE_DATA_FORMAT &format, E_TEXTURE_CREATION_FLAGS &eCreationFlags);
	bool _SwabRB(uint8 *pData, uint uiWidth, uint uiHeight, E_TEXTURE_DATA_FORMAT &format, E_CORE_RENDERER_DATA_ALIGNMENT eAlignment);
	uint _GenerateScaleImage(const uint8 *pDataIn, uint uiWidth, uint uiHeight, uint8 *&prDataOut, uint uiNewWidth, uint uiNewHeight, E_TEXTURE_DATA_FORMAT format, E_CORE_RENDERER_DATA_ALIGNMENT eAlignment);
	uint _GenerateMipMapData(const uint8 *pDataIn, uint uiWidth, uint uiHeight, uint8 *&prDataOut, E_TEXTURE_DATA_FORMAT format, E_CORE_RENDERER_DATA_ALIGNMENT eAlignment);

	bool _CreateTexture(ITexture *&prTex, const uint8 *pData, uint uiWidth, uint uiHeight, E_TEXTURE_DATA_FORMAT eDataFormat, E_TEXTURE_CREATION_FLAGS eCreationFlags, E_TEXTURE_LOAD_FLAGS eLoadFlags);
	bool _LoadTextureBMP(IFile *pFile, ITexture *&prTex, E_TEXTURE_LOAD_FLAGS eFlags);
	bool _LoadTextureTGA(IFile *pFile, ITexture *&prTex, E_TEXTURE_LOAD_FLAGS eFlags);
	bool _LoadTextureDTX(IFile *pFile, ITexture *&prTex, E_TEXTURE_LOAD_FLAGS eFlags);

	bool _LoadFontDFT(IFile *pFile, IBitmapFont *&prFnt);
	
	bool _CreateSound(ISoundSample *&prSndSample, uint uiSamplesPerSec, uint uiBitsPerSample, bool bStereo, const uint8 *pData, uint32 ui32DataSize);
	bool _LoadSoundWAV(IFile *pFile, ISoundSample *&prSSample);

	bool _CreateMesh(IMesh *&prMesh, const uint8 *pData, uint uiDataSize, uint uiNumVerts, uint uiNumFaces, E_MESH_CREATION_FLAGS eCreationFlags, E_MESH_LOAD_FLAGS eLoadFlags);
	bool _LoadDMDFile(IFile *pFile, IEngBaseObj *&prObj, E_MESH_LOAD_FLAGS eLoadFlags);

	void _ProfilerEventHandler() const;
	void _ListResources() const;

	static void DGLE_API _s_ConListFileFormats(void *pParametr, const char *pcParam);
	static void DGLE_API _s_ConListResources(void *pParametr, const char *pcParam);
	static bool DGLE_API _s_LoadTextureBMP(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr);
	static bool DGLE_API _s_LoadTextureTGA(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr); 
	static bool DGLE_API _s_LoadTextureDTX(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr); 
	static bool DGLE_API _s_LoadFontDFT(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr);
	static bool DGLE_API _s_LoadDMDFile(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr);
	static bool DGLE_API _s_LoadSoundWAV(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr);
	static void DGLE_API _s_ProfilerEventHandler(void *pParametr, IBaseEvent *pEvent);

public:

	CResourceManager(uint uiInstIdx);
	~CResourceManager();

	inline IBitmapFont* pISystemFont(){return (IBitmapFont*)_pDefBmpFnt;}
	void FreeAllResources();

	DGLE_RESULT DGLE_API CreateTexture(ITexture *&prTex, const uint8 *pData, uint uiWidth, uint uiHeight, E_TEXTURE_DATA_FORMAT eDataFormat, E_TEXTURE_CREATION_FLAGS eCreationFlags, E_TEXTURE_LOAD_FLAGS eLoadFlags, const char *pcName, bool bAddResourse);
	DGLE_RESULT DGLE_API CreateMaterial(IMaterial *&prMaterial, const char *pcName, bool bAddResourse);
	DGLE_RESULT DGLE_API CreateMesh(IMesh *&prMesh, const uint8 *pData, uint uiDataSize, uint uiNumVerts, uint uiNumFaces, E_MESH_CREATION_FLAGS eCreationFlags, E_MESH_LOAD_FLAGS eLoadFlags, const char *pcName, bool bAddResourse);
	DGLE_RESULT DGLE_API CreateSound(ISoundSample *&prSndSample, uint uiSamplesPerSec, uint uiBitsPerSample, bool bStereo, const uint8 *pData, uint32 ui32DataSize, const char *pcName, bool bAddResourse);

	DGLE_RESULT DGLE_API RegisterFileFormat(const char* pcExtension, E_ENG_OBJ_TYPE eObjType, const char *pcDiscription, bool (DGLE_API *pLoadProc)(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags, void *pParametr), void *pParametr);
	DGLE_RESULT DGLE_API UnregisterFileFormat(const char* pcExtension);
	DGLE_RESULT DGLE_API RegisterDefaultResource(E_ENG_OBJ_TYPE eObjType, IEngBaseObj *pObj);
	DGLE_RESULT DGLE_API UnregisterDefaultResource(E_ENG_OBJ_TYPE eObjType, IEngBaseObj *pObj);
	DGLE_RESULT DGLE_API GetRegisteredExtensions(char* pcTxt, uint &uiCharsCount);
	DGLE_RESULT DGLE_API GetExtensionDescription(const char *pcExtension, char *pcTxt, uint &uiCharsCount);
	DGLE_RESULT DGLE_API GetExtensionType(const char *pcExtension, E_ENG_OBJ_TYPE &eType);
	
	DGLE_RESULT DGLE_API GetResourceByFileName(const char *pcFileName, IEngBaseObj *&prObj);
	DGLE_RESULT DGLE_API GetDefaultResource(E_ENG_OBJ_TYPE eObjType, IEngBaseObj *&prObj);
	
	DGLE_RESULT DGLE_API Load(const char *pcFileName, IEngBaseObj *&prObj, uint uiLoadFlags);
	DGLE_RESULT DGLE_API Load2(IFile *pFile, IEngBaseObj *&prObj, uint uiLoadFlags);
	
	DGLE_RESULT DGLE_API FreeResource(IEngBaseObj *&prObj);
	DGLE_RESULT DGLE_API AddResource(const char *pcName, IEngBaseObj *pObj);
	DGLE_RESULT DGLE_API RemoveResource(IEngBaseObj *pObj, bool &bCanDelete);

	DGLE_RESULT DGLE_API GetType(E_ENGINE_SUB_SYSTEM &eSubSysType);

	IDGLE_BASE_IMPLEMENTATION(IResourceManager, INTERFACE_IMPL(IEngineSubSystem, INTERFACE_IMPL_END))
};
#endif //_RESOURCEMANAGER_H