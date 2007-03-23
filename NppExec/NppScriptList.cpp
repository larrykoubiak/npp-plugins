#include "NppScriptList.h"

CNppScriptList::CNppScriptList()
{
  _bIsModified = false;
}

CNppScriptList::~CNppScriptList()
{
  free();
}

void CNppScriptList::free()
{
  void*       p;
  CNppScript* pScript;

  p = _Scripts.GetFirst();
  while (p)
  {
    _Scripts.GetItem(p, pScript);
    if (pScript)
    {
      pScript->DeleteAll();
      delete pScript;
      _Scripts.SetItem(p, NULL);
    }
    p = _Scripts.GetNext(p);
  }
}

bool CNppScriptList::AddScript(const tstr& ScriptName, const CNppScript& newScript)
{
  if (!ModifyScript(ScriptName, newScript))
  {
    if (_ScriptNames.Add(ScriptName))
    {
      CNppScript* pScript = new CNppScript;
      if (pScript)
      {      
        void* p;
        tstr  S;

        p = newScript.GetFirst();
        while (p)
        {
          newScript.GetItem(p, S);
          pScript->Add(S);
          p = newScript.GetNext(p);
        }
        if (_Scripts.Add(pScript))
        {
          // script is added -> list is modified
          _bIsModified = true;
          return true;
        }
        else
        {
          pScript->DeleteAll();
          delete pScript;
          _ScriptNames.DeleteLast();
          return false;
        }

      }
      else
      {
        _ScriptNames.DeleteLast();
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    // ModifyScript() modifies _bIsModified here
    return true;
  }
}

bool CNppScriptList::DeleteScript(const tstr& ScriptName)
{
  void* p;
  void* p1;
  tstr  S;
  bool  bRet = false;

  p = _ScriptNames.GetFirst();
  p1 = _Scripts.GetFirst();
  while (p && p1)
  {
    _ScriptNames.GetItem(p, S);
    if (S == ScriptName)
    {
      CNppScript* pScript;

      _Scripts.GetItem(p1, pScript);
      if (pScript)
      {
        pScript->DeleteAll();
        delete pScript;
      }
      _Scripts.Delete(p1);
      _ScriptNames.Delete(p);

      // script is deleted -> list is modified
      _bIsModified = true;
      bRet = true;
      p = NULL; // break condition
    }
    else
    {
      p = _ScriptNames.GetNext(p);
      p1 = _Scripts.GetNext(p1);
    }
  }
  
  return bRet;
}

bool CNppScriptList::GetScript(const tstr& ScriptName, CNppScript& outScript)
{
  void* p;
  void* p1;
  tstr  S;
  bool bRet = false;

  outScript.DeleteAll();

  p = _ScriptNames.GetFirst();
  p1 = _Scripts.GetFirst();
  while (p && p1)
  {
    _ScriptNames.GetItem(p, S);
    if (S == ScriptName)
    {
      CNppScript* pScript;

      _Scripts.GetItem(p1, pScript);
      if (pScript)
      {
        p1 = pScript->GetFirst();
        while (p1)
        {
          pScript->GetItem(p1, S);
          //MessageBox(NULL, S.c_str(), "Script line", MB_OK); // OK
          outScript.Add(S);
          p1 = pScript->GetNext(p1);
        }
      }
      bRet = true;
      p = NULL;
    }
    else
    {
      p = _ScriptNames.GetNext(p);
      p1 = _Scripts.GetNext(p1);
    }
  }
  
  return bRet;
}

void CNppScriptList::LoadFromFile(const TCHAR* cszFileName)
{
  CFileBufT<TCHAR> fbuf;

  free();
  _ScriptNames.DeleteAll();
  _Scripts.DeleteAll();
  _bIsModified = false;
  
  if (fbuf.LoadFromFile(cszFileName))
  {
    CStrT<TCHAR> S;
    CStrT<TCHAR> ScriptName;
    int          i;
    int          iScript;
    CNppScript*  pScript;
    
    iScript = 0;
    pScript = NULL;
    while (fbuf.GetLine(S) >= 0)
    {
      i = 0;
      while ((i < S.length()) && (S[i] == ' ' || S[i] == '\t'))
      {
        i++;
      }
      if ((S.GetAt(i) == ':') && (S.GetAt(i+1) == ':'))
      {
        // it is a script name
        ScriptName = S.GetData() + i + 2;
        i = ScriptName.length() - 1;
        while ((i >= 0) && (ScriptName[i] == ' ' || ScriptName[i] == '\t'))
        {
          ScriptName.Delete(i);
          i--;
        }
        _ScriptNames.Add(ScriptName);
        iScript++;
        pScript = new CNppScript;
        _Scripts.Add(pScript);
      }
      else
      {
        // it is a script line
        if (pScript)
        {
          pScript->Add(S);
        }
      }
    }
  }
}

bool CNppScriptList::ModifyScript(const tstr& ScriptName, const CNppScript& newScript)
{
  void* p;
  void* p1;
  tstr  S;
  tstr  S1;
  bool  bModified = false;

  p = _ScriptNames.GetFirst();
  p1 = _Scripts.GetFirst();
  while (p && p1)
  {
    _ScriptNames.GetItem(p, S);
    if (S == ScriptName)
    {
      // ScriptName is matched
      CNppScript* pScript = NULL;

      _Scripts.GetItem(p1, pScript);
      if (pScript)
      {
        // Now let's check if newScript is different than pScript
        p = pScript->GetFirst();
        p1 = newScript.GetFirst();
        while (p1)
        {
          // Walking through newScript
          newScript.GetItem(p1, S1);
          if (p)
          {
            pScript->GetItem(p, S);
            if (S1 != S)
            {
              // list is modified
              _bIsModified = true;
              pScript->SetItem(p, S1);
            }
            p = pScript->GetNext(p);
          }
          else
          {
            // list is modified
            _bIsModified = true;
            pScript->Add(S1);
          }
          p1 = newScript.GetNext(p1);
        }
        if (p)
        {
          // pScript is greater than newScript
          // list is modified
          _bIsModified = true;
          while (((p1 = pScript->GetLast()) != NULL) && (p1 != p))
          {
            pScript->DeleteLast();
          }
          pScript->Delete(p);
          p = NULL; // break condition
        }
      }
      else
      {
        pScript = new CNppScript;
        if (pScript)
        {
          // list is modified
          _bIsModified = true;
          _Scripts.SetItem(p1, pScript);
          p = newScript.GetFirst();
          while (p)
          {
            newScript.GetItem(p, S);
            pScript->Add(S);
            p = newScript.GetNext(p);
          }
        }
      }
      bModified = true;
    }
    else
    {
      p = _ScriptNames.GetNext(p);
      p1 = _Scripts.GetNext(p1);
    }
  }

  return bModified;
}

void CNppScriptList::SaveToFile(const TCHAR* cszFileName)
{
  void*            p;
  void*            pname;
  void*            pline;
  tstr             ScriptName;
  tstr             ScriptLine;
  CNppScript*      pScript;
  CFileBufT<TCHAR> fbuf;

  fbuf.GetBufPtr()->Reserve(_Scripts.GetCount() * 128);
  p = _Scripts.GetFirst();
  pname = _ScriptNames.GetFirst();
  while (p && pname)
  {
    _ScriptNames.GetItem(pname, ScriptName);
    ScriptName.Insert(0, "::", 2);
    ScriptName += "\r\n";
    fbuf.GetBufPtr()->Append(ScriptName.c_str(), ScriptName.length());
    _Scripts.GetItem(p, pScript);
    if (pScript)
    {
      pline = pScript->GetFirst();
      while (pline)
      {
        pScript->GetItem(pline, ScriptLine);
        ScriptLine += "\r\n";
        fbuf.GetBufPtr()->Append(ScriptLine.c_str(), ScriptLine.length());
        pline = pScript->GetNext(pline);
      }
    }
    p = _Scripts.GetNext(p);
    pname = _ScriptNames.GetNext(pname);
  }

  fbuf.SaveToFile(cszFileName);
  // list is saved -> clear modification flag
  _bIsModified = false;
}
