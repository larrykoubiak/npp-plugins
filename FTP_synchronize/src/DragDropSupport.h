#pragma once

#include <windows.h>
#include <ole2.h>
#include <shlobj.h>
#include <stdio.h>

#define BUF_LEN 512

//Based on microsoft classes from DragDrop example files from FTP, altered for personal use

//Class used to allow files to be dragged INTO the window
class CDropTarget : public IDropTarget {
public:	
	CDropTarget();

	/* IUnknown methods */
	STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	/* IDropTarget methods */
	STDMETHOD(DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHOD(DragLeave)();
	STDMETHOD(Drop)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

	/* Client interface methods */
	void setDropCallback(void (TCHAR *));
	void setDragCallback(bool (POINTL));
	void setDragCancelCallback(void (void));
 
private:
	ULONG m_refs;  
	BOOL m_bAcceptFmt;
	void (*dropCallback) (TCHAR *);
	bool (*dragPoint) (POINTL);
	void (*dragCancel) (void);
};

//Class used to allow files to be dragged OUT the window
class CDropSource : public IDropSource {
public:	
	CDropSource();

	/* IUnknown methods */
	STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	/* IDropSource methods */
	STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
	STDMETHOD(GiveFeedback)(DWORD dwEffect);
 
private:
	ULONG m_refs;	 
};  

//Class used when dragging object OUT of window
class CDataObject : public IDataObject {
public:
	CDataObject();	 
   
   /* IUnknown methods */
	STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	/* IDataObject methods */	
	STDMETHOD(GetData)(LPFORMATETC pformatetcIn,  LPSTGMEDIUM pmedium );
	STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium );
	STDMETHOD(QueryGetData)(LPFORMATETC pformatetc );
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
	STDMETHOD(SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc);
	STDMETHOD(DAdvise)(FORMATETC FAR* pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(LPENUMSTATDATA FAR* ppenumAdvise);
	
private:
	ULONG m_refs;   
};

//Class used in conjunction with CDataObject to allow to enumerate all of its dataformats
class CEnumFormatEtc : public IEnumFORMATETC {
public:
	CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats);

	/* IUnknown methods */
	HRESULT __stdcall  QueryInterface (REFIID iid, void ** ppvObject);
	ULONG	__stdcall  AddRef (void);
	ULONG	__stdcall  Release (void);

	/* IEnumFORMATETC methods */	
	HRESULT __stdcall  Next  (ULONG celt, FORMATETC * rgelt, ULONG * pceltFetched);
	HRESULT __stdcall  Skip  (ULONG celt); 
	HRESULT __stdcall  Reset (void);
	HRESULT __stdcall  Clone (IEnumFORMATETC ** ppEnumFormatEtc);

	~CEnumFormatEtc();

private:
	ULONG m_refs;
	ULONG m_nIndex;
	ULONG m_nNumFormats;
	FORMATETC * m_pFormatEtc;
};

IEnumFORMATETC * CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc);