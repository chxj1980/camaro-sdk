// dllmain.h : Declaration of module class.

class CUVCPluginModule :  public ATL::CAtlDllModuleT< CUVCPluginModule >
{
public :
	DECLARE_LIBID(LIBID_UVCPluginLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_UVCPLUGIN, "{73B9F6EF-F648-47E3-B80F-3A9FE71D97D4}")
};

extern class CUVCPluginModule _AtlModule;
