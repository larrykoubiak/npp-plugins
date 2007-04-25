// Utils.cpp


#define UTILS

#include "stdafx.h"

//---------------------------------------------------------------------------------------------------------------------
//--- clase CUTL_BUFFER --------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
CUTL_BUFFER::CUTL_BUFFER() { data= NULL; length=0; }

CUTL_BUFFER::CUTL_BUFFER(LPCSTR str) {
   data= NULL; length=0;
   Copy(str);
}

CUTL_BUFFER::CUTL_BUFFER(UINT size) {
   if (size > 0) length = _CRVERIFY(data = (LPSTR) UTL_alloc(size, sizeof(char))) ? size : 0;
   else {
      data = NULL;
      length = 0;
   }
}

CUTL_BUFFER::CUTL_BUFFER(const CUTL_BUFFER &other) {
   data = NULL;
   length =0;
   _CRVERIFY(Realloc(other.length));
   if (data) UTL_strcpy(data, other.data);
}

CUTL_BUFFER::~CUTL_BUFFER() {
   if (data) UTL_free(data);
}

BOOL CUTL_BUFFER::Realloc(UINT size) {
   if (data) UTL_free(data);
   data = NULL;
   length = 0;
   if (!size) return TRUE;
   if (!(data = (LPSTR) UTL_alloc(size, sizeof(char)))) return false;    
   length = size;
   return TRUE;
}

BOOL CUTL_BUFFER::Accept(CUTL_BUFFER &other) {
   if (data) UTL_free(data);
   //asigno el contenido del otro
   data = other.data;
   length = other.length;
   //desvinculo al otro
   other.data = NULL;
   other.length = 0;  
   return TRUE;
}

LPSTR CUTL_BUFFER::Detach() {
   LPSTR temp = data;
   data = NULL;
   length = 0;
   return temp;
}

LPSTR CUTL_BUFFER::NCopy(LPCSTR string2, size_t count) {
   _CRCHECK(!data || data != string2);
   if ((UINT) length <= count) _CRVERIFY(Realloc((UINT)count +1));
   UTL_strncpy(data, UTL_Null(string2), count);
   data[count] = '\0';
   return data;
}

LPSTR CUTL_BUFFER::Copy(LPCSTR string2) {
   if (data == string2) return data;
   if (!string2) _CRVERIFY(Realloc(0));
   else {
      _CRVERIFY(Realloc(UTL_strlen(string2) +1));
      UTL_strcpy(data, string2);
   }
   return data;
}

LPSTR CUTL_BUFFER::Cat(LPCSTR string2) {
   _CRCHECK(!data || data != string2);
   if (!data) {
      _CRVERIFY(Realloc(UTL_strlen(string2) +1));
      UTL_strcpy(data, UTL_Null(string2));
   }   
   else if (Len() + UTL_strlen(string2) + 1 > length) {
      LPSTR p = (LPSTR) UTL_alloc(1, length = (Len() + UTL_strlen(string2) + 1));
      if (!p) return data;    
      UTL_strcpy(p, data);
      UTL_strcat(p, UTL_Null(string2));
      UTL_free(data);
      data = p;
   }
   else {
      UTL_strcat(data, UTL_Null(string2));
   }   
   return data;
}

LPSTR CUTL_BUFFER::InsStr(LPCSTR str, UINT numChars, UINT pos, BOOL forceUseNumChars) {
   _CRVERIFY(pos <= Len());
   
   if (!forceUseNumChars) numChars = min(numChars, (UINT)UTL_strlen(str));
   if (!numChars) return data;
   if (!data) _CRVERIFY(Realloc(numChars+1));
   UINT newLen = Len() + numChars +1; 
   if (length < newLen) {
      // se expande el buffer + 10 para permitir algunas llamadas reiteradas sin cambiar de buffer
      LPSTR p = (LPSTR) UTL_alloc(1, length = newLen);
      if (!p) return data;    
      UTL_strcpy(p, data);
      UTL_free(data);
      data = p;
   } 
   UTL_memmove(data + pos+numChars, data + pos, Len()-pos+1);
   UTL_strncpy(data + pos, UTL_Null(str), numChars);
   return data;
}

LPSTR CUTL_BUFFER::InsCar(char ch, UINT pos) {
   _CRVERIFY(pos <= Len());
   if (!data) _CRVERIFY(Realloc(2));
   if (length == Len() +1) {
      // se expande el buffer + 10 para permitir algunas llamadas reiteradas sin cambiar de buffer
      LPSTR p = (LPSTR) UTL_alloc(1, length += 10);
      if (!p) return data;    
      UTL_strcpy(p, data);
      UTL_free(data);
      data = p;
   }
   UTL_memmove(data + pos+1, data + pos, Len()-pos+1);
   data[pos] = ch;
   return data;
}

UINT CUTL_BUFFER::RepCar(char replaceCh, char withCh, UINT start) {
   //_CRCHECK(start < Len());
   if (!data) return 0;
   
   LPSTR p = data + start;
   UINT count = 0;
   while (p = UTL_strchr(p, replaceCh)) {
      *(p++) = withCh;
      count++;
   }
   return count;
}

void CUTL_BUFFER::RemoveCar(UINT pos) {
   UINT len = Len();

   if (!_CRVERIFY(pos < len )) return;
   if (!_CRVERIFY(data)) return;
   UTL_memmove(data + pos, data + pos+1, len-pos);
}

CUTL_BUFFER &CUTL_BUFFER::Sf(LPCSTR fmt, ... ) {
   _CRCHECK(!data || data != fmt);
   va_list marker;
   BOOL enough = FALSE;
   static UINT step = 512;
   int total;
   
   va_start(marker, fmt );
   if (!data) Realloc(step);
   do {
      if ((total = UTL_vsnprintf(data, length-1, fmt, marker)) == -1) Realloc(length + step);
      else {
         data[total] = '\0';
         enough = TRUE;
      }
   } while (!enough);
   va_end( marker );
   return *this;
}

CUTL_BUFFER &CUTL_BUFFER::SfVa(LPCSTR fmt, va_list args) {
   _CRCHECK(!data || data != fmt);
   BOOL enough = FALSE;
   static UINT step = 512;
   
   if (!data) Realloc(step);
   do {
      if (UTL_vsnprintf(data, length, fmt, args) == -1) Realloc(length + step);
      else enough = TRUE;
   } while (!enough);
   return *this;
}

long   CUTL_BUFFER::AToL() { return atol(UTL_Null(data)); }
double CUTL_BUFFER::AToF() { return atof(UTL_Null(data)); }
int    CUTL_BUFFER::AToI() { return atoi(UTL_Null(data)); }

LPCSTR CUTL_BUFFER::IToA(int value, int radix) {
   if (length <= 40) Realloc(40);
   return itoa(value, data,radix);
}

LPCSTR CUTL_BUFFER::LToA(long value, int radix) {
   if (length <= 40) Realloc(40);
   return _ltoa(value, data,radix);
}

LPCSTR CUTL_BUFFER::ULToA(unsigned long value, int radix) {
   if (length <= 40) Realloc(40);
   return _ultoa(value, data,radix);
}

CUTL_BUFFER &CUTL_BUFFER::Trim() {
   if (data && Len()) {
      // Elimina los espacios a la derecha
      UINT i = Len() -1;
      for (;data[i] == ' '; i--);
      data[i+1] = '\0';
      // Elimina los espacios a la izquierda
      for (i = 0; data[i] == ' '; i++);
      UTL_memmove(data, data + i, Len() - i +1);
   }
   return *this;   
}

BOOL CUTL_BUFFER::Find(LPCSTR string, UINT &found, UINT start) const {
   if (!data || start >= Len()) return FALSE;
   _CRCHECK(string);
   LPCSTR p = UTL_strstr(data + start, string);
   if (p) {
      found = (UINT) (p - data);
      return TRUE;
   }
   else return FALSE;
}

BOOL CUTL_BUFFER::Find(const char ch, UINT &found, UINT start) const {
   if (!data || start >= Len()) return FALSE;
   LPCSTR p = UTL_strchr(data + start, ch);
   if (p) {
      found = (UINT) (p - data);
      return TRUE;
   }
   else return FALSE;
}

BOOL CUTL_BUFFER::ReverseFind(const char ch, UINT &found) const {
   if (!data) return FALSE;
   LPCSTR p = UTL_strrchr(data, ch);
   if (p) {
      found = (UINT) (p - data);
      return TRUE;
   }
   else return FALSE;
}

BOOL CUTL_BUFFER::FindOneOf(LPCSTR set, UINT &found) const {
   if (!data) return FALSE;
   found = (UINT)UTL_strcspn(data, UTL_Null(set));
   return found < (UINT)UTL_strlen(data) ? TRUE : FALSE;
}


/*--- métodos contra ventanas ----------------------------------------------------------------------------------------------*/
char &CUTL_BUFFER::operator[](UINT index) const {
   static char x = '\0';
   
   if (index >= length) return x;
   return data[index];
}

char &CUTL_BUFFER::operator[](int index) const {
   static char x = '\0';

   if (index < 0 || UINT(index) >= length) return x;
   return data[index];
}

CUTL_BUFFER &CUTL_BUFFER::operator=(const CUTL_BUFFER &buf) {
   if (this != &buf) {
      _CRVERIFY(Realloc(buf.length));
      UTL_strncpy(data, buf.data, length);   
   }
   return *this;
}

CUTL_BUFFER &CUTL_BUFFER::operator=(LPCSTR str) {
   if (str == data) return *this; 
   if (!str) 
      _CRVERIFY(Realloc(0));
   else {
      _CRVERIFY(Realloc(UTL_strlen(str) +1));
      UTL_strcpy(data, str);
   }
   return *this;
}

CUTL_BUFFER &CUTL_BUFFER::operator+=(LPCSTR str) {
   _CRCHECK(!data || data != str);
   if (str) Cat(str);
   return *this;
}

//---------------------------------------------------------------------------------------------------------------------
//--- clase CUT2_INI --------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

//--- Constructores ---------------------------------------------------------------------------------------------------
CUT2_INI::CUT2_INI(LPCSTR fileName) { 
   m_int  = 0;
   m_long = 0L;
   m_file = fileName;
}

CUT2_INI::~CUT2_INI() { 
   if (m_file.Len()) Flush();
}

int CUT2_INI::LoadInt(LPCSTR section, LPCSTR entry, int def) {
   if (!_CRVERIFY(m_file.data)) return 0; //Falta definir el fichero con SetFileName
   _CRCHECK(section && entry);
   m_int = ::GetPrivateProfileInt(section, entry, def, m_file);
   return m_int;
}

long CUT2_INI::LoadLong(LPCSTR section, LPCSTR entry, long def) {
   char convert[26];

   if (!_CRVERIFY(m_file.data)) return 0; //Falta definir el fichero con SetFileName
   _CRCHECK(section && entry);
   if (!::GetPrivateProfileString(section, entry, "", convert, 25, m_file)) return def;
   return m_long = UTL_atol(convert);
}

LPCSTR CUT2_INI::LoadStr(LPCSTR section, LPCSTR entry, LPCSTR def) {
   int size;
   CUTL_BUFFER buf(512);                          

   if (!_CRVERIFY(m_file.data)) return 0; //Falta definir el fichero con SetFileName
   _CRCHECK(section && entry);
   size = ::GetPrivateProfileString(section, entry, UTL_Null(def), LPSTR(buf), 512, m_file);
   CUTL_BUFFER::operator=(buf);    
   return (size) ? data : NULL;
}

BOOL CUT2_INI::Write(LPCSTR section, LPCSTR entry, LPCSTR str) const {
   if (!_CRVERIFY(m_file.data)) return FALSE; //Falta definir el fichero con SetFileName
   _CRCHECK(section && entry);
   return ::WritePrivateProfileString(section, entry, str, m_file.data);
}

BOOL CUT2_INI::Write(LPCSTR section, LPCSTR entry, int value) const {
   char convert[10];
   
   if (!_CRVERIFY(m_file.data)) return FALSE; //Falta definir el fichero con SetFileName
   _CRCHECK(section && entry);
   UTL_itoa(value, convert, 10);
   return ::WritePrivateProfileString(section, entry, convert, m_file.data);
}

BOOL CUT2_INI::Write(LPCSTR section, LPCSTR entry, long value) const {
   char convert[26];
   
   if (!_CRVERIFY(m_file.data)) return FALSE; //Falta definir el fichero con SetFileName
   _CRCHECK(section && entry);
   UTL_ltoa(value, convert, 10);
   return ::WritePrivateProfileString(section, entry, convert, m_file.data);
}

BOOL CUT2_INI::Delete(LPCSTR section, LPCSTR entry) const {
   if (!_CRVERIFY(m_file.data)) return FALSE; //Falta definir el fichero con SetFileName
   _CRCHECK(section);
   return ::WritePrivateProfileString(section, entry, NULL, m_file.data);
}

BOOL CUT2_INI::Flush() const {
   if (!_CRVERIFY(m_file.data)) return FALSE; //Falta definir el fichero con SetFileName
   return ::WritePrivateProfileString(NULL, NULL, NULL, m_file.data);
}                                            

LPCSTR CUT2_INI::LoadEntries(LPCSTR section) {
   int size;
   CUTL_BUFFER buf(32*1024);
 
   if (!_CRVERIFY(m_file.data)) return 0; // Falta definir el fichero con SetFileName
   _CRCHECK(section);
   size = ::GetPrivateProfileString(section, NULL, "", LPSTR(buf), 32*1024, m_file);
   Realloc(size+2);
   UTL_memcpy(data, LPSTR(buf), size+2);      
   return (size) ? data : NULL;
}

LPCSTR CUT2_INI::LoadSections() {
   int size;
   CUTL_BUFFER buf(1024);

   if (!_CRVERIFY(m_file.data)) return 0; //Falta definir el fichero con SetFileName
   size = ::GetPrivateProfileString(NULL, NULL, "", LPSTR(buf), 1024, m_file);
   Realloc(size+2);
   UTL_memcpy(data, LPSTR(buf), size+2);      
   return (size) ? data : NULL;
}

LPCSTR CUT2_INI::GetEntry(UINT pos) const {
   int i = 0, j;
   
   for (i=0; pos && (j = UTL_strlen((LPSTR) data + i)); i += j+1, pos--); 
   return UTL_strlen((LPSTR) data + i) ? (LPSTR) data + i : NULL;
}


//---------------------------------------------------------------------------------------------------------------------
//--- clase CUTL_PATH -------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

//--- Constructores -------------------------------------------------------------------------------------------------------

CUTL_PATH::CUTL_PATH() {
   xInit();
}

CUTL_PATH::CUTL_PATH(const CUTL_PATH &other) {
   xInit();
   m_path = other.m_path;
}

CUTL_PATH::CUTL_PATH(LPCSTR path) {
   xInit();
   m_path = path;
}

CUTL_PATH::CUTL_PATH(SPECIALDIR specialDir) {
   xInit();
   switch (specialDir) {
      case DIR_CURRENT: CurrentDirectory(); break;
      case DIR_WINDOWS: WindowsDirectory(); break;
      case DIR_SYSTEM:  SystemDirectory();  break;
      case DIR_TEMP:    TempDirectory();    break;
      default: _CRCHECK(FALSE);
   }
}

CUTL_PATH::~CUTL_PATH() {
   xClose();
}


//--- métodos de carga y/o configuración ----------------------------------------------------------------------------------

void CUTL_PATH::Set(LPCSTR path) {
   m_path = path;
}

void CUTL_PATH::SetDrive(char driveLeter) {
   CUTL_BUFFER drive(2);
   CUTL_BUFFER directory;
   CUTL_BUFFER name;
   CUTL_BUFFER extension;
   drive[0] = driveLeter;
   GetComponents(NULL, &directory, &name, &extension);
   SetComponents(drive, directory, name, extension);
}

void CUTL_PATH::SetDriveDirectory(LPCSTR pDriveDirectory) {
   CUTL_BUFFER driveDirectory = pDriveDirectory;
   CUTL_BUFFER name;
   CUTL_BUFFER extension;

   EnsureTrailingBackslash(driveDirectory);

   GetComponents(NULL, NULL, &name, &extension);
   SetComponents(NULL, driveDirectory, name, extension);
}

void CUTL_PATH::SetDirectory(LPCSTR pDirectory, BOOL ensureAbsolute) {
   CUTL_BUFFER drive;
   CUTL_BUFFER directory = pDirectory;
   CUTL_BUFFER name;
   CUTL_BUFFER extension;

   if (ensureAbsolute) EnsureLeadingBackslash(directory);
   EnsureTrailingBackslash(directory);		
   GetComponents(&drive, NULL, &name, &extension);
   SetComponents(drive, directory, name, extension);
}

void CUTL_PATH::SetName(LPCSTR pName) {
   CUTL_BUFFER drive;
   CUTL_BUFFER directory;
   CUTL_BUFFER extension;

   GetComponents (&drive, &directory, NULL, &extension);
   SetComponents (drive, directory, pName, extension);
}

void CUTL_PATH::SetNameExtension(LPCSTR pNameExtension) {
   CUTL_BUFFER drive;
   CUTL_BUFFER directory;

   GetComponents (&drive, &directory, NULL, NULL);
   SetComponents (drive, directory, pNameExtension, NULL);	
}

void CUTL_PATH::SetExtension(LPCSTR pExtension) {
   CUTL_BUFFER drive;
   CUTL_BUFFER directory;
   CUTL_BUFFER name;

   GetComponents (&drive, &directory, &name, NULL);
   SetComponents (drive, directory, name, pExtension);
}

void CUTL_PATH::AppendDirectory(LPCSTR pSubDirectory) {
   CUTL_BUFFER drive;
   CUTL_BUFFER directory;
   CUTL_BUFFER subDirectory = pSubDirectory;
   CUTL_BUFFER name;
   CUTL_BUFFER extension;

   if (!subDirectory.Len()) return;
   // strip out any preceeding backslash		
   StripLeadingBackslash(subDirectory);
   EnsureTrailingBackslash(subDirectory);
   GetComponents(&drive, &directory, &name, &extension);
   EnsureTrailingBackslash(directory);
   SetComponents(drive, directory.Cat(subDirectory), name, extension);
}

void CUTL_PATH::UpDirectory(CUTL_BUFFER* pLastDirectory) {
   CUTL_BUFFER directory;
   UINT        delimiter;

   GetDirectory(directory);	
   StripTrailingBackslash(directory);
   if(!directory.Len()) return;
   _CRVERIFY(directory.ReverseFind(PATH_DIR_DELIMITER, delimiter));
   if (pLastDirectory != NULL) {
      *pLastDirectory = LPCSTR(directory) + delimiter;
      StripLeadingBackslash(*pLastDirectory);
   }
   if (delimiter >= 0)
      directory[delimiter] = '\0';
   SetDirectory(directory);
}

void CUTL_PATH::SetComponents (LPCSTR drive, LPCSTR directory, LPCSTR name, LPCSTR extension) {
   m_path.Realloc(_MAX_PATH);
   _makepath(m_path, drive, directory, name, extension);
}


//--- métodos automáticos de crecaión de rutas ----------------------------------------------------------------------------

void CUTL_PATH::CurrentDirectory() {
   CUTL_BUFFER driveDirectory(_MAX_PATH);

   _getcwd(driveDirectory, _MAX_PATH);
   Clear();
   SetDriveDirectory(driveDirectory);
}

void CUTL_PATH::WindowsDirectory() {
   CUTL_BUFFER driveDirectory(_MAX_PATH);

   GetWindowsDirectory(driveDirectory, _MAX_PATH);
   Clear();
   SetDriveDirectory(driveDirectory);
}

void CUTL_PATH::SystemDirectory() {
   CUTL_BUFFER driveDirectory(_MAX_PATH);

   GetSystemDirectory(driveDirectory, _MAX_PATH);
   Clear();
   SetDriveDirectory(driveDirectory);
}

void CUTL_PATH::TempDirectory() {
   m_path.Realloc(_MAX_PATH);
   ::GetTempPath(_MAX_PATH, m_path);
   SetNameExtension("");	
}

void CUTL_PATH::MakeRoot() {
   SetDirectory("");
   SetNameExtension("");
}

void CUTL_PATH::Clear() {
   m_path = NULL;
}


//--- métodos de consulta de datos ----------------------------------------------------------------------------------------

void CUTL_PATH::GetDrive(CUTL_BUFFER &drive) const {
   GetComponents(&drive, NULL, NULL, NULL);
}

void CUTL_PATH::GetDriveDirectory(CUTL_BUFFER &driveDirectory) const {
   CUTL_BUFFER drive, directory;

   GetComponents(&drive, &directory);
   driveDirectory = drive;
   if (drive.Len()) driveDirectory += directory.InsCar(PATH_DRIVE_DELIMITER, 0);
}

void CUTL_PATH::GetDirectory(CUTL_BUFFER &directory) const {
   GetComponents(NULL, &directory, NULL, NULL);
}

void CUTL_PATH::GetName(CUTL_BUFFER &name) const {
   GetComponents(NULL, NULL, &name, NULL);
}

void CUTL_PATH::GetNameExtension(CUTL_BUFFER &nameExtension) const {
   CUTL_BUFFER name, ext;

   GetComponents(NULL, NULL, &name, &ext);
   nameExtension = name;                                     
   if (ext.Len())
   nameExtension += ext.InsCar(PATH_EXT_DELIMITER, 0);
}

void CUTL_PATH::GetExtension(CUTL_BUFFER &extension) const {
   GetComponents(NULL, NULL, NULL, &extension);
}

void CUTL_PATH::GetComponents(CUTL_BUFFER *pDrive, CUTL_BUFFER *pDir, CUTL_BUFFER *pName, CUTL_BUFFER *pExt) const {
   if (!m_path) {
      if (pDrive) pDrive->Realloc(0);
      if (pDir)   pDir->Realloc(0);
      if (pName)  pName->Realloc(0);
      if (pExt)   pExt->Realloc(0);
      return;
   }
   if (pDrive) pDrive->Realloc(_MAX_DRIVE + 1);
   if (pDir)   pDir->Realloc(  _MAX_DIR + 1);
   if (pName)  pName->Realloc( _MAX_FNAME + 1);
   if (pExt)   pExt->Realloc(  _MAX_EXT + 1);
   _splitpath(m_path, 
      (pDrive ? LPSTR(*pDrive) : NULL),
      (pDir   ? LPSTR(*pDir)   : NULL),
      (pName  ? LPSTR(*pName)  : NULL),
      (pExt   ? LPSTR(*pExt)   : NULL)
   );
	// DOS's _splitpath returns "d:", we return "d"
	if (pDrive) StripTrailingChar (*pDrive, PATH_DRIVE_DELIMITER);
	// DOS's _splitpath returns "\dir\subdir\", we return "\dir\subdir"	
	if (pDir)   StripTrailingBackslash (*pDir);
	// DOS's _splitpath returns ".ext", we return "ext"	
	if (pExt)   StripLeadingChar (*pExt, PATH_EXT_DELIMITER);		
}

BOOL CUTL_PATH::GetFullyQualified(CUTL_BUFFER& fullyQualified) const {
   _CRVERIFY(fullyQualified.Realloc(_MAX_PATH +1));
   return _fullpath(fullyQualified, m_path.GetSafe(), _MAX_PATH +1) ? TRUE : FALSE;
}

BOOL CUTL_PATH::GetTime(CCR3_DATE &date) const { 
   struct _stat s;

   if (!_stat(m_path, &s)) {
      struct tm *tReg = localtime(&(s.st_mtime));
      if (tReg == NULL) return FALSE;
      _CRVERIFY(date.SetDate(tReg->tm_year + 1900, tReg->tm_mon +1, tReg->tm_mday));
      _CRVERIFY(date.SetTime(tReg->tm_hour, tReg->tm_min, tReg->tm_sec));
	  return TRUE;
   }
   return FALSE;	
}

//--- métodos de actuación sobre la unidad --------------------------------------------------------------------------------

BOOL CUTL_PATH::IsRemovableDrive() const {
   CUTL_PATH rootPath = *this;
   rootPath.MakeRoot ();
   return (GetDriveType(rootPath) == DRIVE_REMOVABLE);
}

BOOL CUTL_PATH::IsCDRomDrive() const {
   CUTL_PATH rootPath = *this;
   rootPath.MakeRoot ();
   return (GetDriveType(rootPath) == DRIVE_CDROM);
}

BOOL CUTL_PATH::IsNetworkDrive() const {
   CUTL_PATH rootPath = *this;
   rootPath.MakeRoot ();
   return (GetDriveType(rootPath) == DRIVE_REMOTE);
}

BOOL CUTL_PATH::IsRAMDrive() const {
   CUTL_PATH rootPath = *this;
   rootPath.MakeRoot ();
   return (GetDriveType(rootPath) == DRIVE_RAMDISK);
}

LONG CUTL_PATH::DriveTotalSpaceBytes() const {
   DWORD	nSectorsPerCluster;
   DWORD	nBytesPerSector;
   DWORD	nFreeClusters;
   DWORD	nClusters;

   if (!xGetDiskFreeSpace(&nSectorsPerCluster, &nBytesPerSector, &nFreeClusters, &nClusters)) return 0;
   return nClusters * nSectorsPerCluster * nBytesPerSector;
}

LONG CUTL_PATH::DriveFreeSpaceBytes() const {
   DWORD	nSectorsPerCluster;
   DWORD	nBytesPerSector;
   DWORD	nFreeClusters;
   DWORD	nClusters;

   if (!xGetDiskFreeSpace(&nSectorsPerCluster, &nBytesPerSector, &nFreeClusters, &nClusters)) return 0;
   return nFreeClusters * nSectorsPerCluster * nBytesPerSector;
}

LONG CUTL_PATH::GetDriveClusterSize() const {
   DWORD	nSectorsPerCluster;
   DWORD	nBytesPerSector;
   DWORD	nFreeClusters;
   DWORD	nClusters;

   if (!xGetDiskFreeSpace(&nSectorsPerCluster, &nBytesPerSector, &nFreeClusters, &nClusters)) return 0;
   return nSectorsPerCluster * nBytesPerSector;
}


//--- métodos de actuación sobre el directorio ----------------------------------------------------------------------------

BOOL CUTL_PATH::DirectoryExists() const {
   /*
	   Win32 Effect:		To determine if the directory exists, we need to
						   create a test path with a wildcard (*.*) extension
						   and see if FindFirstFile returns anything.  We don't
						   use CPath::FindFirst() because that routine parses out
						   '.' and '..', which fails for empty directories.
   */
   CUTL_PATH testPath = *this;
   WIN32_FIND_DATA findData;
   BOOL gotFile;

   testPath.SetNameExtension(PATH_WILD_NAME_EXT);
   HANDLE hFindFile = FindFirstFile(testPath, &findData);	//find anything in the path
   gotFile = (hFindFile != INVALID_HANDLE_VALUE);
   if (hFindFile != NULL) FindClose(hFindFile); //make sure to close the file
   return gotFile; 
}

BOOL CUTL_PATH::IsDirectoryEmpty() const {
	CUTL_PATH fileSpec = *this;
	
	fileSpec.SetNameExtension(PATH_WILD_NAME_EXT);
	return !fileSpec.FindFirst();
}

BOOL CUTL_PATH::CreateDirectory(BOOL createIntermediates) {
   CUTL_BUFFER pathText(m_path);
   BOOL result;
   UINT delimiter;

   StripTrailingBackslash(pathText);
   result = (_mkdir(pathText) ==  0);
   if (!result)
      result = ChangeDirectory();
   if (!result && createIntermediates) {
      if (!pathText.ReverseFind(PATH_DIR_DELIMITER, delimiter)) return FALSE;
      pathText[delimiter] = '\0';
      CUTL_PATH subPath = pathText;

      if (subPath.CreateDirectory()) 
         return CreateDirectory (FALSE);
      else 
         return FALSE;
   }
   return result;
}

//-------------------------------------------------------------
// Pre     :
// Post    : Return TRUE if deleted OK
// Globals :
// I/O     :
// Task    : Remove the directory. Even if it is not empty
//-------------------------------------------------------------
BOOL CUTL_PATH::RemoveDirectory() {
   // Delete the directory's content
   if(!RemoveDirectoryContent()) return FALSE;

   // Make sure there is no enumeration in progress,
   // otherwise we we'll get an error (sharing violation) because
   // that search keeps an open handle for this directory
   xClose();
    
    // Deleting this directory (and only if it's empty)
	CUTL_BUFFER driveDirectory;

   if (m_path[0] == '\\' && m_path[1] == '\\') // Es un UNC
      driveDirectory = m_path.GetSafe();
   else
      GetDriveDirectory(driveDirectory);
	if ((driveDirectory[driveDirectory.Len() - 1] == '\\' || driveDirectory[driveDirectory.Len() - 1] == '/')) driveDirectory[driveDirectory.Len() - 1] = '\0';
   return ::RemoveDirectory(driveDirectory) ? TRUE : FALSE; 
}

//-------------------------------------------------------------
// Pre     :
// Post    : Return TRUE if deleted OK
// Globals :
// I/O     :
// Task    : Delete everything in the directory
//-------------------------------------------------------------
BOOL CUTL_PATH::RemoveDirectoryContent() {
   // Deleting the directory's content
   // Iterate the content of the directory and delete it
   CUTL_PATH   iterator(*this);
   BOOL        bResult = TRUE;

   // Deleting all contained files
   iterator.SetNameExtension("*.*");
   BOOL bIterating = iterator.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY);
   while(bIterating) {
     bResult = iterator.Delete(TRUE);
     if(!bResult) break;
     iterator.SetNameExtension("*.*");
     bIterating = iterator.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY);
   }
   if(!bResult) return FALSE;
   // Deleting all contained directories
   iterator.SetNameExtension("*.*");
   bIterating = iterator.FindFirst(_A_HIDDEN | _A_SUBDIR);
   while(bIterating) {
     bResult = iterator.RemoveDirectory();
     if(!bResult) break;
     iterator.SetNameExtension("*.*");
     iterator.UpDirectory();
     bIterating = iterator.FindFirst(_A_HIDDEN | _A_SUBDIR);
   }
   return bResult;
}

BOOL CUTL_PATH::ChangeDirectory() {
   CUTL_BUFFER driveDirectory;
   GetDriveDirectory(driveDirectory);
	return (_chdir(driveDirectory) == 0);
}

//--- métodos de actuación sobre el fichero -------------------------------------------------------------------------------

BOOL CUTL_PATH::Exists() const {
   WIN32_FIND_DATA findData;
   HANDLE searchHandle = FindFirstFile(m_path, &findData);
   BOOL result = (searchHandle != INVALID_HANDLE_VALUE);
   FindClose(searchHandle);
   return result; 
}

LONG CUTL_PATH::GetSize() const {
   struct _stat s;
   if (_stat(m_path, &s) != 0) return 0;
   return (s.st_size);
}

BOOL CUTL_PATH::Delete(BOOL evenIfReadOnly) {
   //REMOVE will not actually remove read only files
   //so we might want to set the readonly property
   //off
   if(evenIfReadOnly && _access(m_path, _S_IWRITE) != -1) { //does the file have write access
      if(_chmod(m_path, _S_IWRITE) == -1) 
         return FALSE;
   }
   return !remove(m_path);
}

BOOL CUTL_PATH::Rename(LPCSTR newPath) {
   return !rename(m_path, newPath);
}

BOOL CUTL_PATH::FindFirst(DWORD attributes) {
/* Effect:		Find the first file that meets this path and the specified attributes.
  	Arguments:	dwAttributes
				   These constants specify the current attributes 
				   of the file or directory specified by the function.
				   The attributes are represented by the following 
				   manifest constants:

				   _A_ARCH		Archive. Set whenever the file is changed, and cleared by the BACKUP 
							      command.
				   _A_HIDDEN   Hidden file. Not normally seen with the DIR command, unless the /AH option 
							      is used. Returns information about normal files as well as files with this attribute.
				   _A_NORMAL   Normal. File can be read or written to without restriction.
				   _A_RDONLY   Read-only. File cannot be opened for writing, and a file with the same name cannot be created. 
				   _A_SUBDIR   Subdirectory.
				   _A_SYSTEM   System file. Not normally seen with the DIR command, unless the /AS option is used.

				   Multiple constants can be combined with the OR operator (|).

	Note:		   These attributes do not follow a simple additive logic. Note that _A_NORMAL is 0x00, so it effectively cannot be
				   removed from the attribute set. You will therefore always get normal files, and may also get Archive, Hidden, etc.,
				   if you specify those attributes. 

  See Also:	   FindNextFile, FindFirstSubdirectory.
*/
   BOOL gotFile;
   BOOL wantSubdirectory = (_A_SUBDIR & attributes);
   WIN32_FIND_DATA findData;

   // Close handle to any previous enumeration
   xClose();

   m_findFileAttributes = attributes;
   m_hFindFile = FindFirstFile(m_path, &findData);
   gotFile = (m_hFindFile != INVALID_HANDLE_VALUE);
   while (gotFile) {
		// ii. compare candidate to attributes
		if (!AttributesMatch(m_findFileAttributes, findData.dwFileAttributes)) goto GetAnother;
		if (wantSubdirectory && (findData.cFileName[0] == '.')) goto GetAnother;
		// iii. found match; prepare result
		if (_A_SUBDIR & m_findFileAttributes) StripTrailingBackslash(m_path);
		SetNameExtension(findData.cFileName);
      if (_A_SUBDIR & attributes)
         EnsureTrailingBackslash(m_path);
		return TRUE;
		// iv. not found match; get another
	GetAnother:
		gotFile = FindNextFile(m_hFindFile, &findData);
	}
	return FALSE;
}

BOOL CUTL_PATH::FindNext() {
   WIN32_FIND_DATA findData;

   _CRCHECK(m_hFindFile);
   while (FindNextFile(m_hFindFile, &findData) != FALSE) {
      if (AttributesMatch(m_findFileAttributes, findData.dwFileAttributes)) {
         if (_A_SUBDIR & m_findFileAttributes) {
            UpDirectory();
            AppendDirectory(findData.cFileName);
         }
         else
            SetNameExtension (findData.cFileName);
         return TRUE;
      }
   }
   return FALSE;
}


//operadores --------------------------------------------------------------------------------------------------------------

BOOL CUTL_PATH::IsEmpty()  const {
   return !m_path;
}

BOOL CUTL_PATH::IsValid () const {
/* Effect:        Determine whether lpszFileName is valid. A filename
                  is valid if it contains only legal characters, doesn't
                  have repeated contiguous subdirectory delimiters, has at 
                  most one drive delimiter, has at most one extension 
                  delimiter, and all components fit within maximum sizes.

                  This routine does *not* determine if a file exists, or
                  even if it could exist relative to the user's directory
                  hierarchy.  Its tests are for lexical correctness only.

   See Also:      CPath::Exists.
*/
   SECURITY_ATTRIBUTES sa;

   HANDLE fHandle = CreateFile(
      m_path,                // pointer to name of the file 
      0,                     // access (read-write) mode 
      0,                     // share mode 
      &sa,                   // pointer to security descriptor 
      OPEN_EXISTING,         // how to create 
      FILE_ATTRIBUTE_NORMAL, // file attributes 
      NULL                   // handle to file with attributes to copy  
   );
   BOOL result = (fHandle != INVALID_HANDLE_VALUE);

   if (result) CloseHandle(fHandle);
   return result;
}

BOOL CUTL_PATH::IsWild ()  const {
   UINT pos;
   return m_path.FindOneOf(PATH_WILD_SET, pos);
}

CUTL_PATH& CUTL_PATH::operator= (const CUTL_PATH &other) {
   if (this == &other) return *this;
   m_path = other.m_path;
   return *this;
}

CUTL_PATH& CUTL_PATH::operator= (LPCSTR path) {
   m_path = path;
   return *this;
}

CUTL_PATH::operator LPCSTR() const {
   return (LPCSTR) m_path;
}

BOOL CUTL_PATH::operator == (const CUTL_PATH &other) const {
	CUTL_BUFFER fullyQualified1, fullyQualified2;
	
	GetFullyQualified(fullyQualified1);
	other.GetFullyQualified(fullyQualified2);
	return (UTL_stricmp(fullyQualified1, fullyQualified2) == 0);
}

    
//--- otros métodos -------------------------------------------------------------------------------------------------------

void CUTL_PATH::xInit() {
   m_findFileAttributes = 0;
   m_hFindFile = NULL;
}

void CUTL_PATH::xClose() {
   if(m_hFindFile != NULL) {
      FindClose(m_hFindFile);
      m_hFindFile =NULL;
   }
}

BOOL CUTL_PATH::xGetDiskFreeSpace(LPDWORD pSectorsPerCluster, LPDWORD pBytesPerSector, LPDWORD pFreeClusters, LPDWORD pClusters) const {
	CUTL_PATH rootPath = *this;

	rootPath.MakeRoot ();
	return ::GetDiskFreeSpace(rootPath,  pSectorsPerCluster, pBytesPerSector, pFreeClusters, pClusters);
}


//---------------------------------------------------------------------------------------------------------------------
//--- clase CCR3_DATE -------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
CCR3_DATE::CCR3_DATE() {
   Clear();
}

CCR3_DATE::CCR3_DATE(time_t timeExpr) {
   Clear();
   CCR3_DATE::operator=(timeExpr);
}

CCR3_DATE::CCR3_DATE(LPCSTR date) {
   if (!UTL_strlen(date)) {
      Clear();
   }
   else {
      _CRCHECK(UTL_strlen(date) == 17);
      UTL_strcpy(m_info, "00000000000000000");
      UTL_strncpy(m_info, date, UTL_strlen(date));
      CUTL_BUFFER buf(5);
      UTL_strncpy(buf, m_info,    4); buf[4] = '\0'; m_yy = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+4,  2); buf[2] = '\0'; m_mm = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+6,  2); buf[2] = '\0'; m_dd = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+8,  2); buf[2] = '\0'; m_hh = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+10, 2); buf[2] = '\0'; m_mi = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+12, 2); buf[2] = '\0'; m_ss = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+14, 3); buf[3] = '\0'; m_fs = UTL_atoi(buf);
   }                                            
}

CCR3_DATE::CCR3_DATE(const CCR3_DATE &other) {
   UTL_strcpy(m_info, other.m_info);
   m_yy = other.m_yy;
   m_mm = other.m_mm;
   m_dd = other.m_dd;
   m_hh = other.m_hh;
   m_mi = other.m_mi;
   m_ss = other.m_ss;
   m_fs = other.m_fs;
}

CCR3_DATE::~CCR3_DATE() {}

CCR3_DATE &CCR3_DATE::operator=(const CCR3_DATE &date) {
   if (this != &date) {
      UTL_strcpy(m_info, date.m_info);
      m_yy = date.m_yy;
      m_mm = date.m_mm;
      m_dd = date.m_dd;
      m_hh = date.m_hh;
      m_mi = date.m_mi;
      m_ss = date.m_ss;
      m_fs = date.m_fs;
   }
   return *this;
}

CCR3_DATE &CCR3_DATE::operator=(LPCSTR date) {
   if (!UTL_strlen(date)) {
      UTL_strcpy(m_info, "00000000000000000");
      m_yy = m_mm = m_dd = m_hh = m_mi = m_ss = m_fs = 0;
   }
   else {
      _CRCHECK(UTL_strlen(date) == 17);
      UTL_strcpy(m_info, "00000000000000000");
      UTL_strncpy(m_info, date, UTL_strlen(date));
      CUTL_BUFFER buf(5);
      UTL_strncpy(buf, m_info,    4); buf[4] = '\0'; m_yy = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+4,  2); buf[2] = '\0'; m_mm = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+6,  2); buf[2] = '\0'; m_dd = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+8,  2); buf[2] = '\0'; m_hh = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+10, 2); buf[2] = '\0'; m_mi = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+12, 2); buf[2] = '\0'; m_ss = UTL_atoi(buf);
      UTL_strncpy(buf, m_info+14, 3); buf[3] = '\0'; m_fs = UTL_atoi(buf);
   }                                             
   return *this;
}

CCR3_DATE &CCR3_DATE::operator=(time_t timeExpr) {
    struct tm *tReg;
    CUTL_BUFFER target;
   
    tReg = localtime(&timeExpr);   
    target.Sf("%04d%02d%02d%02d%02d%02d%02d000",  // según la estructura TIMEREG 
        m_yy = (tReg->tm_year + 1900), 
        m_mm = tReg->tm_mon +1, 
        m_dd = tReg->tm_mday,
        m_hh = tReg->tm_hour,
        m_mi = tReg->tm_min,
        m_ss = tReg->tm_sec,
        m_fs = 0
    );
    UTL_strcpy(m_info, target);
    return *this;
}

void CCR3_DATE::SysDate() {
   CUTL_BUFFER target;
   SYSTEMTIME  st;

   ::ZeroMemory(&st, sizeof(SYSTEMTIME));
   ::GetLocalTime(&st);
    target.Sf("%04d%02d%02d%02d%02d%02d%03d",  /* según la estructura TIMEREG */
              m_yy = st.wYear, 
              m_mm = st.wMonth, 
              m_dd = st.wDay,
              m_hh = st.wHour,
              m_mi = st.wMinute,
              m_ss = st.wSecond,
              m_fs = st.wMilliseconds
    );
    UTL_strcpy(m_info, target);
}

BOOL CCR3_DATE::SetDate(int yy, int mm, int dd) {
   if ((yy || mm || dd) && !ChkDate(yy,mm,dd)) return FALSE;
   sprintf(m_info, "%04d%02d%02d%s", yy, mm, dd, m_info+8);
   m_yy = yy;
   m_mm = mm;
   m_dd = dd;
   return TRUE;
}

BOOL CCR3_DATE::SetTime(int hh, int mi, int ss, int fs) {
   if (!ChkTime(hh,mi,ss, fs)) return FALSE;
   sprintf(m_info+8, "%02d%02d%02d%03d", 
      m_hh = hh, 
      m_mi = mi, 
      m_ss = ss, 
      m_fs = fs
   );
   return TRUE;
}

void CCR3_DATE::ResetDate() {
   UTL_strncpy(m_info, "00000000", 8);
   m_yy = m_mm = m_dd = 0;
}

void CCR3_DATE::ResetTime() {
   UTL_strncpy(m_info+8, "000000000", 9);
   m_hh = m_mi = m_ss = m_fs = 0;
}

void CCR3_DATE::Clear() {      
   UTL_strcpy(m_info, "00000000000000000");
   m_yy = m_mm = m_dd = m_hh = m_mi = m_ss = m_fs = 0;
}

BOOL CCR3_DATE::operator<(const CCR3_DATE &date) {
   return (UTL_strcmp(m_info, date.m_info) < 0) ? TRUE : FALSE;
}

BOOL CCR3_DATE::operator<=(const CCR3_DATE &date) {
   return (UTL_strcmp(m_info, date.m_info) <= 0) ? TRUE : FALSE;
}

BOOL CCR3_DATE::operator>(const CCR3_DATE &date) {
   return (UTL_strcmp(m_info, date.m_info) > 0) ? TRUE : FALSE;
}

BOOL CCR3_DATE::operator>=(const CCR3_DATE &date) {
   return (UTL_strcmp(m_info, date.m_info) >= 0) ? TRUE : FALSE;
}

BOOL CCR3_DATE::operator==(const CCR3_DATE &date) {
   return (UTL_strcmp(m_info, date.m_info) == 0) ? TRUE : FALSE;
}

UINT CCR3_DATE::SizeOf() { return 17; }

CCR3_DATE::operator time_t() {
   struct tm tReg;
   
   tReg.tm_year = m_yy - 1900;
   tReg.tm_mon  = m_mm -1;
   tReg.tm_mday = m_dd;
   tReg.tm_hour = m_hh; 
   tReg.tm_min  = m_mi; 
   tReg.tm_sec  = m_ss;
   return mktime(&tReg);
}

/*--- Formateo de fechas -------------------------------------------------------------------------------------------------*/

void CCR3_DATE::Format(formatStyle style, CUTL_BUFFER* target) {
   target->Realloc(50);
   
   switch (style) {
      case DDMMYY:
         target->Sf("%c%c/%c%c/%c%c", m_info[6],m_info[7],m_info[4],m_info[5],m_info[2],m_info[3]);
         break;
      case DDHHMI:
         target->Sf("%c%c %c%c:%c%c",m_info[6], m_info[7], m_info[8], m_info[9], m_info[10], m_info[11]);
         break;       
      case DDMMYYHHMISS:
         target->Sf("%c%c/%c%c/%c%c %c%c:%c%c:%c%c",          
            m_info[6],  m_info[7],
            m_info[4],  m_info[5],
            m_info[2],  m_info[3],
            m_info[8],  m_info[9],
            m_info[10], m_info[11],
            m_info[12], m_info[13]
         );
         break;               
      case DDMMYYHHMISSML:
         target->Sf("%c%c/%c%c/%c%c %c%c:%c%c:%c%c,%c%c%c",           
            m_info[6],  m_info[7],
            m_info[4],  m_info[5],
            m_info[2],  m_info[3],
            m_info[8],  m_info[9],
            m_info[10], m_info[11],
            m_info[12], m_info[13],
            m_info[14], m_info[15], m_info[16]
         );
         break;   
      case HHMISS:
         target->Sf("%c%c:%c%c:%c%c", m_info[8], m_info[9], m_info[10], m_info[11], m_info[12], m_info[13]);
         break;
      case HHMI:
         target->Sf("%c%c:%c%c",m_info[8], m_info[9], m_info[10], m_info[11]);
         break;       
      default:
         _CRCHECK(FALSE);
   }
}

BOOL CCR3_DATE::ChkDate(int yy, int mm, int dd) {
   if (!dd) return FALSE;
   switch (mm){
      case 1:
      case 3:
      case 5:
      case 7:
      case 8:
      case 10:
      case 12:
         if (dd > 31) return FALSE;
         break;
      case 4:
      case 6:
      case 9:
      case 11:
         if (dd > 30) return FALSE;
         break;
      case 2:
		   if ((((yy % 4) == 0) && ((yy % 100) != 0)) || ((yy % 400) == 0)) { // Si el año es bisiesto 
            if (dd > 29) return FALSE;
         }
         else if (dd > 28) return FALSE;
         break;
      default:
         return FALSE;
   }
   return TRUE;
}

BOOL CCR3_DATE::ChkTime(int hh, int mi, int ss, int fs) {
   return (hh >= 24 || mi >= 60 || ss >= 60 || fs >= 1000) ? FALSE : TRUE;
}

/****************************************************************************************************************************/
/* CUTL_PARSE */

CUTL_PARSE::CUTL_PARSE() { 
   m_charSet = NULL;
   m_escape = NULL;
   SetStream(NULL); 
}

CUTL_PARSE::CUTL_PARSE(LPCSTR srcStream, LPCSTR charSet, char escape) { 
   SetCharSet(charSet, escape);
   SetStream(srcStream);
}

CUTL_PARSE::~CUTL_PARSE() {}

void CUTL_PARSE::SetCharSet(LPCSTR charSet, char escape) {
   m_escape = escape;
   m_charSet.Sf("%s%c", UTL_Null(charSet), escape);
}

void CUTL_PARSE::SetStream(LPCSTR srcStream) {
   m_stream = srcStream;
   m_offset = 0;   
   m_tokenType = T_NOTOKEN;
   m_tokenNumArgs = 0;
   m_tokenArgs = NULL;
}

CUTL_PARSE::token CUTL_PARSE::NextToken() {
   if (!m_stream.data) return T_ERROR;
   if (m_tokenType == T_EOF) return T_ERROR;
   m_tokenNumArgs = 0;
   m_tokenArgs = NULL;
   m_tokenType = T_NOTOKEN;
   UINT i;
   CUTL_BUFFER tmp;
   do {
      //FRAGMENTO DE STRING
      if ((i = (UINT)UTL_strcspn((LPSTR)m_stream +m_offset, m_charSet)) > 0) {
         m_tokenType = T_STRING;
         _CRVERIFY(tmp.Realloc(i +1));
         tmp.NCopy((LPSTR)m_stream +m_offset, i);
         m_tokenArgs += (LPCSTR) tmp;
         m_offset += i;
      }
      //CONDICION DE CONTINUIDAD DE UN STRING
      if (m_stream[m_offset] && m_stream[m_offset] == m_escape && m_stream[m_offset+1] == m_escape) {
         m_tokenType = T_STRING;
         tmp.Sf("%c", m_escape);
         m_tokenArgs += tmp;
         m_offset += 2; 
         continue;
      }
      if (m_tokenType == T_STRING) {
         m_tokenNumArgs = 1;
         return T_STRING;
      }   
      // FIN DE CADENA
      if (!m_stream[m_offset]) return m_tokenType = T_EOF;
      // SECUENCIA DE ESCAPE
      if (m_stream[m_offset] == m_escape) {
         //Busca el final del comando
         if (!m_stream.Find(';', i, m_offset+1)) {
            m_tokenArgs = 0;
            m_tokenNumArgs = 1;
            return T_ERROR;
         }
         _CRVERIFY(tmp.Realloc(i - m_offset));
         tmp.NCopy((LPSTR)m_stream +m_offset+1, i - m_offset-1);
         m_tokenArgs += (LPCSTR) tmp;
         m_offset = i+1;
         //Cuenta y marca los parámetros
         m_tokenNumArgs = 1 + m_tokenArgs.RepCar(',', '\0');
         return m_tokenType = T_ESCAPE;
      }
      //CHARSET
      m_tokenNumArgs = 1;
      m_tokenArgs.Realloc(2); m_tokenArgs[0] = m_stream[m_offset++];
      return m_tokenType = T_CHARSET;
   } while (TRUE);
   return (T_ERROR); //Para evitar warnings
}

CUTL_PARSE::token CUTL_PARSE::LookAhead() {
   if (!m_stream.data) return T_ERROR;
   if (m_tokenType == T_EOF) return T_ERROR;
   //Fragmentos de String
   if (UTL_strcspn((LPSTR)m_stream +m_offset, m_charSet) > 0) return T_STRING;
   if (m_stream[m_offset] && m_stream[m_offset] == m_escape && m_stream[m_offset+1] == m_escape) return T_STRING;
   //Fin de Cadena
   if (!m_stream[m_offset]) return T_EOF;
   //Secuencia de Escape
   if (m_stream[m_offset] == m_escape) return T_ESCAPE;
   //Charset
   return T_CHARSET;
}

void CUTL_PARSE::Restart() {
   m_offset = 0;   
   m_tokenType = T_NOTOKEN;
   m_tokenNumArgs = 0;
   m_tokenArgs = NULL;
}

LPSTR CUTL_PARSE::StrArg(UINT pos) {
   if (!pos || pos > m_tokenNumArgs) return NULL;
   int i = 0;
   while (--pos) i += UTL_strlen((LPSTR) m_tokenArgs + i) +1; 
   return (LPSTR)m_tokenArgs +i;
}                                                                                       

DWORD CUTL_PARSE::LongHexArg(UINT pos) {
   if (!pos || pos > m_tokenNumArgs) return 0L;
   int i = 0;
   while (--pos) i += UTL_strlen((LPSTR) m_tokenArgs + i) +1; 
   return UTL_strtoul((LPSTR)m_tokenArgs +i, NULL, 16);
}

long CUTL_PARSE::LongArg(UINT pos) {
   if (!pos || pos > m_tokenNumArgs) return 0;
   int i = 0;
   while (--pos) i += UTL_strlen((LPSTR) m_tokenArgs + i) +1; 
   return UTL_atol((LPSTR)m_tokenArgs +i);
}

int CUTL_PARSE::IntArg(UINT pos) {
   if (!pos || pos > m_tokenNumArgs) return 0;
   int i = 0;
   while (--pos) i += UTL_strlen((LPSTR) m_tokenArgs + i) +1; 
   return UTL_atoi((LPSTR)m_tokenArgs +i);
}

char CUTL_PARSE::ChrArg(UINT pos) {
   if (!pos || pos > m_tokenNumArgs) return '\0';
   int i = 0;
   while (--pos) i += UTL_strlen((LPSTR) m_tokenArgs + i) +1; 
   return m_tokenArgs[i];
}

CUTL_PARSE::token CUTL_PARSE::xSetError() {
   m_tokenNumArgs = 0;
   m_tokenArgs = NULL;
   m_stream = NULL;
   m_offset = 0;   
   return m_tokenType = T_ERROR;
}

//*************************************************************************************************************************
//*************************************************************************************************************************
//*************************************************************************************************************************
//*** Area LOCAL **********************************************************************************************************
//*************************************************************************************************************************
//*************************************************************************************************************************

void StripLeadingChar(CUTL_BUFFER& text, char leadingCh) {
   UINT len = text.Len();

   if (!len) return;
   if (text[0] == leadingCh) text.RemoveCar(0);
}

void StripLeadingBackslash (CUTL_BUFFER& directory) {
   UINT len = directory.Len();

   if (len <= 1) return;
   if (directory[0] == PATH_DIR_DELIMITER) directory.RemoveCar(0);
}	

void StripTrailingChar(CUTL_BUFFER& text, char traillingCh) {
	UINT len = text.Len();
	
	if (!len) return;
	if (text[len -1] == traillingCh) text[len -1] = '\0';
}


void StripTrailingBackslash (CUTL_BUFFER& directory) {
	UINT len = directory.Len();
	
	// if Directory is of the form '\', don't do it.
	if (len <= 1) return;
	if (directory[len -1] == PATH_DIR_DELIMITER) 
      directory[len -1] = '\0';
}


void EnsureTrailingBackslash (CUTL_BUFFER& directory) {
	UINT len = directory.Len();

	if (!len || directory[len -1] != PATH_DIR_DELIMITER) 
      directory.InsCar(PATH_DIR_DELIMITER, len);
}
	

void EnsureLeadingBackslash(CUTL_BUFFER& directory) {
	if (!directory || directory[0] != PATH_DIR_DELIMITER)
		directory.InsCar(PATH_DIR_DELIMITER, 0);
}

BOOL AttributesMatch(DWORD targetAttributes, DWORD fileAttributes) { 
	// The only thing we care about is if this is a folder or not
	if (targetAttributes & _A_SUBDIR) 
		return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	else 
		return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? false : true;

/*
	// Calculamos los atributos que se nos piden 
	DWORD targetAttributesNewAPI;

	if (targetAttributes & _A_NORMAL) 
		targetAttributesNewAPI |= FILE_ATTRIBUTE_NORMAL;
	if (targetAttributes & _A_ARCH) 
		targetAttributesNewAPI |= FILE_ATTRIBUTE_ARCHIVE;
	if (targetAttributes & _A_HIDDEN) 
		targetAttributesNewAPI |= FILE_ATTRIBUTE_HIDDEN;
	if (targetAttributes & _A_RDONLY) 
		targetAttributesNewAPI |= FILE_ATTRIBUTE_READONLY;
	if (targetAttributes & _A_SUBDIR) 
		targetAttributesNewAPI |= FILE_ATTRIBUTE_DIRECTORY;
	if (targetAttributes & _A_SYSTEM) 
		targetAttributesNewAPI |= FILE_ATTRIBUTE_SYSTEM;

	if (targetAttributesNewAPI == FILE_ATTRIBUTE_NORMAL)
		return (!(FILE_ATTRIBUTE_DIRECTORY & targetAttributesNewAPI));
	else
		return ((targetAttributesNewAPI & fileAttributes) && ((FILE_ATTRIBUTE_DIRECTORY & targetAttributesNewAPI) == (FILE_ATTRIBUTE_DIRECTORY & fileAttributes)));
	*/
	/*
	if (targetAttributes == _A_NORMAL)
		return (!(_A_SUBDIR & fileAttributes));
	else
		return ((targetAttributes & fileAttributes) && ((_A_SUBDIR & targetAttributes) == (_A_SUBDIR & fileAttributes)));
	*/
}


// Funciones locales de apoyo

int UTL_strcmp(const char *string1, const char *string2) {
   if (!string1 && !string2) return 0;
   if (!string1) return -1;
   if (!string2) return 1;
   return lstrcmp(string1, string2);
}

int UTL_stricmp(const char *string1, const char *string2) {
   if (!string1 && !string2) return 0;
   if (!string1) return -1;
   if (!string2) return 1;
   return lstrcmpi(string1, string2);
}

int UTL_strncmp(const char *string1, const char *string2, size_t count ) {
   if (!string1 && !string2) return 0;
   if (!string1) return -1;
   if (!string2) return 1;
   LPSTR a1,a2;
   int result;
   _CRVERIFY(a1 = (LPSTR) UTL_alloc(1, count +1));
   _CRVERIFY(a2 = (LPSTR) UTL_alloc(1, count +1));
   UTL_strncpy(a1, string1, count); a1[count] = '\0';
   UTL_strncpy(a2, string2, count); a2[count] = '\0';
   result = lstrcmp(a1, a2);
   UTL_free(a1);
   UTL_free(a2);
   return result;
}

