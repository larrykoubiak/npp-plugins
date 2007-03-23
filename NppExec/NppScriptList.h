#ifndef _npp_script_list_h_
#define _npp_script_list_h_
//--------------------------------------------------------------------
#include "base.h"
#include "cpp/CTinyStrT.h"
#include "cpp/CTinyBufT.h"
#include "cpp/CListT.h"
#include "cpp/CFileBufT.h"

typedef CStrT<TCHAR> tstr;
typedef CListT<tstr> CNppScript;
typedef CNppScript*  PNppScript;

class CNppScriptList 
{
private:
  CListT<tstr>        _ScriptNames;
  CListT<PNppScript>  _Scripts;
  bool                _bIsModified;

  void free();

public:
  CNppScriptList();
  ~CNppScriptList();
  int   GetScriptCount() const  { 
    return _ScriptNames.GetCount(); 
  }
  void* GetFirstScriptNameItemPtr() const  { 
    return _ScriptNames.GetFirst(); 
  }
  void* GetFirstScriptItemPtr() const  { 
    return _Scripts.GetFirst(); 
  }
  void* GetLastScriptNameItemPtr() const  { 
    return _ScriptNames.GetLast(); 
  }
  void* GetLastScriptItemPtr() const  { 
    return _Scripts.GetLast(); 
  }
  void* GetNextScriptNameItemPtr(const void* item_ptr) const  { 
    return _ScriptNames.GetNext(item_ptr); 
  }
  void* GetNextScriptItemPtr(const void* item_ptr) const  { 
    return _Scripts.GetNext(item_ptr); 
  }
  void* GetPrevScriptNameItemPtr(const void* item_ptr) const  {
    return _ScriptNames.GetPrev(item_ptr); 
  }
  void* GetPrevScriptItemPtr(const void* item_ptr) const  { 
    return _Scripts.GetPrev(item_ptr); 
  }
  bool  GetScriptNameItem(const void* item_ptr, tstr& name) const  {
    return (_ScriptNames.GetItem(item_ptr, name));
  }
  bool  GetScriptItem(const void* item_ptr, PNppScript& pScript) const  {
    return (_Scripts.GetItem(item_ptr, pScript));
  }
  bool  AddScript(const tstr& ScriptName, const CNppScript& newScript);
  bool  DeleteScript(const tstr& ScriptName);
  bool  GetScript(const tstr& ScriptName, CNppScript& outScript);
  bool  IsModified() const  { return _bIsModified; }
  void  LoadFromFile(const TCHAR* cszFileName);
  bool  ModifyScript(const tstr& ScriptName, const CNppScript& newScript);
  void  SaveToFile(const TCHAR* cszFileName);
  void  SetModified(bool bIsModified)  { _bIsModified = bIsModified; }
};

//--------------------------------------------------------------------
#endif

