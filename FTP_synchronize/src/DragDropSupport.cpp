#include "stdafx.h"
#include "DragDropSupport.h"
#include "Logging.h"

#ifdef UNICODE
#define CustomClipBoardFormatName L"FTP_Sync_Fileobject"
#else
#define CustomClipBoardFormatName "FTP_Sync_Fileobject"
#endif

UINT CustomClipBoardFormatId;

//---------------------------------------------------------------------DropTarget---------------------------------------------------------------------

CDropTarget::CDropTarget() {
	m_refs = 1; 
	m_bAcceptFmt = FALSE;
	CustomClipBoardFormatId = RegisterClipboardFormat(CustomClipBoardFormatName);
}   

//---------------------------------------------------------------------
//					IUnknown Methods
//---------------------------------------------------------------------
STDMETHODIMP CDropTarget::QueryInterface(REFIID iid, void FAR* FAR* ppv) {
	if(iid == IID_IUnknown || iid == IID_IDropTarget) {
		*ppv = this;
		AddRef();
		return NOERROR;
	}
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CDropTarget::AddRef(void) {
	return ++m_refs;
}

STDMETHODIMP_(ULONG) CDropTarget::Release(void) {
	if(--m_refs == 0) {
		delete this;
		return 0;
	}
	return m_refs;
}  

//---------------------------------------------------------------------
//					IDropTarget Methods
//---------------------------------------------------------------------  
STDMETHODIMP CDropTarget::DragEnter(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
	//printToLog("DragEnter!\n");
	FORMATETC fmtetc;
	   
	fmtetc.ptd	  = NULL;
	fmtetc.dwAspect = DVASPECT_CONTENT;  
	fmtetc.lindex   = -1;
	fmtetc.tymed	= TYMED_HGLOBAL;
	
	//If the source contains our custom format, we shouldnt accept it
	fmtetc.cfFormat	= CustomClipBoardFormatId; 
	m_bAcceptFmt = (NOERROR == pDataObj->QueryGetData(&fmtetc)) ? FALSE : TRUE;	

	// Does the drag source provide CF_HDROP, which is the only format we accept.
	fmtetc.cfFormat	= CF_HDROP;
	m_bAcceptFmt = (NOERROR == pDataObj->QueryGetData(&fmtetc) && m_bAcceptFmt) ? TRUE : FALSE;	
	
	//do pdwEffect thing
	if (m_bAcceptFmt)
		*pdwEffect= DROPEFFECT_COPY;
	else
		*pdwEffect= DROPEFFECT_NONE;
	return NOERROR;
}

STDMETHODIMP CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
	//printToLog("DragOver!\n");
	//do pdwEffect thing
	if (m_bAcceptFmt) {
		if (this->dragPoint(pt)) {
			*pdwEffect= DROPEFFECT_COPY;
		} else
			*pdwEffect= DROPEFFECT_NONE;
	} else
		*pdwEffect= DROPEFFECT_NONE;
	return NOERROR;
}

STDMETHODIMP CDropTarget::DragLeave() {
	//printToLog("DragLeave!\n");
	this->dragCancel();
	m_bAcceptFmt = FALSE;   
	return NOERROR;
}

STDMETHODIMP CDropTarget::Drop(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) {
	printToLog("Drop! %d %d\n", pt.x, pt.y);
	FORMATETC fmtetc;   
	STGMEDIUM medium;   
	HGLOBAL hDropFiles;
	DROPFILES * pDf;
	HRESULT hr;
	TCHAR * firstFile;
	 
	if (m_bAcceptFmt && this->dragPoint(pt)) {	  //check if valid drop
		fmtetc.cfFormat = CF_HDROP;		//accept HDROP only for now
		fmtetc.ptd = NULL;
		fmtetc.dwAspect = DVASPECT_CONTENT;  
		fmtetc.lindex = -1;
		fmtetc.tymed = TYMED_HGLOBAL;	   
		
		// User has dropped on us. Get the CF_HDROP data from drag source
		hr = pDataObj->GetData(&fmtetc, &medium);
		if (FAILED(hr)) {
			*pdwEffect = DROPEFFECT_NONE;
			return hr;
		}
		
		// Parse the data and release it.
		hDropFiles = medium.hGlobal;
		pDf = (DROPFILES *) GlobalLock(hDropFiles);

		firstFile = (TCHAR*)((char*)pDf + (pDf->pFiles));
		this->dropCallback(firstFile);

		GlobalUnlock(hDropFiles);
		ReleaseStgMedium(&medium);
		*pdwEffect = DROPEFFECT_COPY;
	}
	*pdwEffect = DROPEFFECT_NONE;
	return NOERROR;	  
}

void CDropTarget::setDropCallback(void (*callBack)(TCHAR *)) {
	dropCallback = callBack;
}

void CDropTarget::setDragCallback(bool (*callBack)(POINTL)) {
	dragPoint = callBack;
}

void CDropTarget::setDragCancelCallback(void (*callBack)(void)) {
	dragCancel = callBack;
}


//---------------------------------------------------------------------DropSource---------------------------------------------------------------------



//---------------------------------------------------------------------
//					CDropSource Constructor
//---------------------------------------------------------------------		
CDropSource::CDropSource() {
	CustomClipBoardFormatId = RegisterClipboardFormat(CustomClipBoardFormatName);
	m_refs = 1;  
}   

//---------------------------------------------------------------------
//					IUnknown Methods
//---------------------------------------------------------------------
STDMETHODIMP CDropSource::QueryInterface(REFIID iid, void FAR* FAR* ppv) {
	if(iid == IID_IUnknown || iid == IID_IDropSource)
	{
	  *ppv = this;
	  ++m_refs;
	  return NOERROR;
	}
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CDropSource::AddRef(void) {
	return ++m_refs;
}


STDMETHODIMP_(ULONG) CDropSource::Release(void) {
	if(--m_refs == 0)
	{
	  delete this;
	  return 0;
	}
	return m_refs;
}  

//---------------------------------------------------------------------
//					IDropSource Methods
//---------------------------------------------------------------------  
STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) {  
	 if (fEscapePressed)
		return ResultFromScode(DRAGDROP_S_CANCEL);
	else if (!(grfKeyState & MK_LBUTTON))
		return ResultFromScode(DRAGDROP_S_DROP);
	else
		return NOERROR;				  
}

STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect) {
	return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}

//---------------------------------------------------------------------DataObject---------------------------------------------------------------------

//---------------------------------------------------------------------
//					CDataObject Constructor
//---------------------------------------------------------------------		
CDataObject::CDataObject() {
   m_refs = 1;	
}   

//---------------------------------------------------------------------
//					IUnknown Methods
//---------------------------------------------------------------------
STDMETHODIMP CDataObject::QueryInterface(REFIID iid, void FAR* FAR* ppv) {
	if(iid == IID_IUnknown || iid == IID_IDataObject) {
	  *ppv = this;
	  AddRef();
	  return NOERROR;
	}
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CDataObject::AddRef(void) {
	return ++m_refs;
}

STDMETHODIMP_(ULONG) CDataObject::Release(void) {
	if(--m_refs == 0) {
	  delete this;
	  return 0;
	}
	return m_refs;
}  

//---------------------------------------------------------------------
//					IDataObject Methods	
//  
// The following methods are NOT supported for data transfer using the
// clipboard or drag-drop: 
//
//	  IDataObject::SetData	-- return E_NOTIMPL
//	  IDataObject::DAdvise	-- return OLE_E_ADVISENOTSUPPORTED
//				 ::DUnadvise
//				 ::EnumDAdvise
//	  IDataObject::GetCanonicalFormatEtc -- return E_NOTIMPL
//					 (NOTE: must set pformatetcOut->ptd = NULL)
//---------------------------------------------------------------------  
STDMETHODIMP CDataObject::GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium) {   
	HGLOBAL hText; 
	LPSTR pszText;
	
	pmedium->tymed = NULL;
	pmedium->pUnkForRelease = NULL;
	pmedium->hGlobal = NULL;
	
	// This method is called by the drag-drop target to obtain the text
	// that is being dragged.
	if (pformatetc->cfFormat == CF_TEXT && pformatetc->dwAspect == DVASPECT_CONTENT && pformatetc->tymed == TYMED_HGLOBAL) {
		hText = GlobalAlloc(GMEM_SHARE | GMEM_ZEROINIT, 12);	
		if (!hText)
			return ResultFromScode(E_OUTOFMEMORY);
		pszText = (LPSTR)GlobalLock(hText);
		strcpy(pszText, "hello world");
		GlobalUnlock(hText);
		
		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->hGlobal = hText; 
 
		return ResultFromScode(S_OK);
	}
	return ResultFromScode(DATA_E_FORMATETC);
}
   
STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium) {
	return ResultFromScode(DATA_E_FORMATETC);	
}	 

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC pformatetc) {   
	// This method is called by the drop target to check whether the source
	// provides data is a format that the target accepts.
	if (pformatetc->cfFormat == CF_TEXT 
		&& pformatetc->dwAspect == DVASPECT_CONTENT
		&& pformatetc->tymed & TYMED_HGLOBAL)
		return ResultFromScode(S_OK); 
	else return ResultFromScode(S_FALSE);
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut) { 
	pformatetcOut->ptd = NULL; 
	return ResultFromScode(E_NOTIMPL);
}		

STDMETHODIMP CDataObject::SetData(LPFORMATETC pformatetc, STGMEDIUM *pmedium, BOOL fRelease) {   
	// A data transfer object that is used to transfer data
	//	(either via the clipboard or drag/drop does NOT
	//	accept SetData on ANY format.
	return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc) { 
	// A standard implementation is provided by OleStdEnumFmtEtc_Create
	// which can be found in \ole2\samp\ole2ui\enumfetc.c in the OLE 2 SDK.
	// This code from ole2ui is copied to the enumfetc.c file in this sample.
	
	SCODE sc = S_OK;
	FORMATETC pfmtetc[2];
	
	pfmtetc[0].cfFormat = CF_TEXT;
	pfmtetc[0].dwAspect = DVASPECT_CONTENT;
	pfmtetc[0].tymed = TYMED_HGLOBAL;
	pfmtetc[0].ptd = NULL;
	pfmtetc[0].lindex = -1;

	pfmtetc[1].cfFormat = CustomClipBoardFormatId;
	pfmtetc[1].dwAspect = DVASPECT_CONTENT;
	pfmtetc[1].tymed = TYMED_NULL;
	pfmtetc[1].ptd = NULL;
	pfmtetc[1].lindex = -1;

	if (dwDirection == DATADIR_GET){
		*ppenumFormatEtc = CreateEnumFormatEtc(2, pfmtetc);
		if (*ppenumFormatEtc == NULL)
			sc = E_OUTOFMEMORY;
	} else if (dwDirection == DATADIR_SET) {
		sc = E_NOTIMPL;
	} else {
		sc = E_INVALIDARG;
	}

	return ResultFromScode(sc);
}

STDMETHODIMP CDataObject::DAdvise(FORMATETC FAR* pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD FAR* pdwConnection) { 
	return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}
   

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection) { 
	return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP CDataObject::EnumDAdvise(LPENUMSTATDATA FAR* ppenumAdvise) {
	return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}


//---------------------------------------------------------------------EnumFORMATETC---------------------------------------------------------------------
//
//	"Drop-in" replacement for SHCreateStdEnumFmtEtc. Called by CDataObject::EnumFormatEtc
//
IEnumFORMATETC * CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc) {
	return new CEnumFormatEtc(pFormatEtc, nNumFormats);
}

//
//	Helper function to perform a "deep" copy of a FORMATETC
//
static void DeepCopyFormatEtc(FORMATETC *dest, FORMATETC *source) {
	// copy the source FORMATETC into dest
	*dest = *source;
	
	if(source->ptd)
	{
		// allocate memory for the DVTARGETDEVICE if necessary
		dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

		// copy the contents of the source DVTARGETDEVICE into dest->ptd
		*(dest->ptd) = *(source->ptd);
	}
}

//
//	Constructor 
//
CEnumFormatEtc::CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats) {
	m_refs   = 1;
	m_nIndex      = 0;
	m_nNumFormats = nNumFormats;
	m_pFormatEtc  = new FORMATETC[nNumFormats];
	
	// copy the FORMATETC structures
	for(int i = 0; i < nNumFormats; i++)
	{	
		DeepCopyFormatEtc(&m_pFormatEtc[i], &pFormatEtc[i]);
	}
}

//
//	Destructor
//
CEnumFormatEtc::~CEnumFormatEtc() {
	if(m_pFormatEtc)
	{
		for(ULONG i = 0; i < m_nNumFormats; i++)
		{
			if(m_pFormatEtc[i].ptd)
				CoTaskMemFree(m_pFormatEtc[i].ptd);
		}

		delete[] m_pFormatEtc;
	}
}


//---------------------------------------------------------------------
//					IUnknown Methods
//---------------------------------------------------------------------
STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID iid, void FAR* FAR* ppv) {
	if(iid == IID_IUnknown || iid == IID_IEnumFORMATETC) {
	  *ppv = this;
	  AddRef();
	  return NOERROR;
	}
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef(void) {
	return ++m_refs;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release(void) {
	if(--m_refs == 0) {
	  delete this;
	  return 0;
	}
	return m_refs;
}


//---------------------------------------------------------------------
//	IEnumFORMATETC::Next
//
//	If the returned FORMATETC structure contains a non-null "ptd" member, then
//  the caller must free this using CoTaskMemFree (stated in the COM documentation)
//---------------------------------------------------------------------
STDMETHODIMP CEnumFormatEtc::Next(ULONG celt, FORMATETC *pFormatEtc, ULONG * pceltFetched)
{
	ULONG copied  = 0;

	// validate arguments
	if(celt == 0 || pFormatEtc == 0)
		return E_INVALIDARG;

	// copy FORMATETC structures into caller's buffer
	while(m_nIndex < m_nNumFormats && copied < celt)
	{
		DeepCopyFormatEtc(&pFormatEtc[copied], &m_pFormatEtc[m_nIndex]);
		copied++;
		m_nIndex++;
	}

	// store result
	if(pceltFetched != 0) 
		*pceltFetched = copied;

	// did we copy all that was requested?
	return (copied == celt) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Skip
//
STDMETHODIMP CEnumFormatEtc::Skip(ULONG celt)
{
	m_nIndex += celt;
	return (m_nIndex <= m_nNumFormats) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Reset
//
STDMETHODIMP CEnumFormatEtc::Reset(void)
{
	m_nIndex = 0;
	return S_OK;
}

//
//	IEnumFORMATETC::Clone
//
STDMETHODIMP CEnumFormatEtc::Clone(IEnumFORMATETC ** ppEnumFormatEtc) {

	*ppEnumFormatEtc = CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc);

	if(*ppEnumFormatEtc != NULL) {
		// manually set the index state
		((CEnumFormatEtc *) *ppEnumFormatEtc)->m_nIndex = m_nIndex;
		return S_OK;
	} else {
		return E_OUTOFMEMORY;
	}
}