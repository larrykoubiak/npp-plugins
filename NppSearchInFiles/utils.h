// Utils.h

#include <tchar.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#pragma warning ( disable : 4996 )

extern "C" {
int UTL_strcmp    (const char *string1, const char *string2);
int UTL_stricmp   (const char *string1, const char *string2);
int UTL_strncmp   (const char *string1, const char *string2, size_t count );
}

// --- Defines --------------------------------------------------------------------------------------------------------

#define _CRVERIFY(exp)      ((exp) ? TRUE : FALSE)
#define _CRCHECK(exp)

#define INI_SECTION_TARGETS "Targets"


inline void  *UTL_alloc(size_t num, size_t size )                             { return calloc(num, size); }
inline void   UTL_free(void *memblock )                                       { free(memblock); }
inline char  *UTL_strcpy(char *string1, const char *string2 )                 { return strcpy(string1, string2); };
inline char  *UTL_strncpy(char *string1, const char *string2, size_t count )  { return strncpy(string1, string2, count); };
inline int    UTL_vsnprintf( char *buf,size_t num,const char *fmt,va_list args){ return _vsnprintf(buf,num,fmt,args); }
inline char  *UTL_strcat( char *string1, const char *string2 )                 { return strcat(string1, string2 ); };
inline void  *UTL_memmove( void *dest, const void *src, size_t count )         { return memmove(dest, src, count); }
inline void  *UTL_memcpy( void *dest, const void *src, size_t count )          { return memcpy(dest, src, count);}
inline int    UTL_strlen(const char *string )                                 { return (!string) ? 0 : (int)strlen(string); }
inline char  *UTL_strchr( char *string, int c )                          { return strchr(string, c); };
inline char  *UTL_strstr( char *string1, const char *string2 )           { return strstr(string1,string2); };
inline char  *UTL_strrchr( char *string, int c )                         { return strrchr(string, c); };
inline size_t UTL_strcspn( const char *string1, const char *string2 )          { return strcspn(string1, string2); };
inline long   UTL_atol( const char *string )                                   { return atol(string); };
inline int    UTL_atoi( const char *string )                                   { return atoi(string); };
inline char  *UTL_itoa( int value, char *string, int radix)                    { return itoa(value,string,radix); };
inline char  *UTL_ltoa( long value, char *string, int radix )                  { return _ltoa(value,string,radix); };
inline LPTSTR UTL_strupr(LPTSTR lpsz)                                          { return CharUpper(lpsz); };
inline LPTSTR UTL_strlwr(LPTSTR lpsz)                                          { return CharLower(lpsz); };
inline unsigned long UTL_strtoul(const char *nptr, char **endptr, int base )   { return strtoul(nptr,endptr,base); };

inline LPCSTR          UTL_Null(LPCSTR expr, LPCSTR defaultValue = "") { return (LPCSTR)((expr) ? expr : defaultValue);};
inline LPSTR           UTL_Null(LPSTR expr,  LPSTR defaultValue = "") { return (LPSTR)((expr) ? expr : defaultValue);};

/****************************************************************************************************************************/
/*** Buffer que se auto libera si es construido en la pila. (No reserva espacio en la pila) */
class CUTL_BUFFER {   
public:
   CUTL_BUFFER();
   CUTL_BUFFER(LPCSTR str);
   CUTL_BUFFER(UINT size);
   CUTL_BUFFER(const CUTL_BUFFER &other);
   virtual ~CUTL_BUFFER();
public:
   // Funciones.
   BOOL         Realloc(UINT size);
   BOOL         Accept(CUTL_BUFFER &other);
   LPSTR        Detach();
   UINT         strlen() const;
   LPSTR        Copy(LPCSTR string2);
   LPSTR        NCopy(LPCSTR string2, size_t count);
   LPSTR        Cat(LPCSTR string2);
   LPSTR        InsStr(LPCSTR str, UINT numChars, UINT pos, BOOL forceUseNumChars = FALSE);
   LPSTR        InsCar(char ch, UINT pos);
   UINT         RepCar(char replaceCh, char withCh, UINT start=0);
   void         RemoveCar(UINT pos);
   CUTL_BUFFER &Sf(LPCSTR fmt, ... );
   CUTL_BUFFER &SfVa(LPCSTR fmt, va_list args);
   // conversión numérica/alfanumérica
   long         AToL();
   double       AToF();
   int          AToI();
   LPCSTR       IToA(int value, int radix =10);
   LPCSTR       LToA(long value, int radix =10);
   LPCSTR       ULToA(unsigned long value, int radix = 10);
   // Otros métodos
   CUTL_BUFFER &Trim();
   BOOL         Find(LPCSTR string, UINT &found, UINT start = 0) const;
   BOOL         Find(const char ch, UINT &found, UINT start = 0) const;
   BOOL         ReverseFind(const char ch, UINT &found) const;
   BOOL         FindOneOf(LPCSTR set, UINT &found) const;
   LPCSTR       GetSafe() const;

   CUTL_BUFFER &CUTL_BUFFER::Upper()        { if (data) UTL_strupr(data); return *this; }
   CUTL_BUFFER &CUTL_BUFFER::Lower()        { if (data) UTL_strlwr(data); return *this; }

public: // Operadores.  
   char        &operator[](UINT index) const;
   char        &operator[](int index) const;
   operator     LPSTR() const;
   operator     LPCSTR() const;   
   CUTL_BUFFER &operator=(const CUTL_BUFFER &buf);
   CUTL_BUFFER &operator=(LPCSTR str);
   CUTL_BUFFER &operator+=(LPCSTR str);
   
   BOOL         operator!() const;
   BOOL         operator<(LPCSTR str) const;
   BOOL         operator<=(LPCSTR str) const;
   BOOL         operator==(LPCSTR str) const;
   BOOL         operator==(LPSTR str) const;
   BOOL         operator!=(LPCSTR str) const;
   BOOL         operator>=(LPCSTR str) const;
   BOOL         operator>(LPCSTR str) const;
public:
   LPSTR data;

   //--- AREA X --------------------------------------------------------
private:
   UINT length;                               
};

inline UINT CUTL_BUFFER::strlen() const            { return UTL_strlen(data);}
inline LPCSTR       CUTL_BUFFER::GetSafe() const{ return (data) ? data : ""; }
inline CUTL_BUFFER::operator LPSTR() const      { return data; }
inline CUTL_BUFFER::operator LPCSTR() const     { return data; }

inline BOOL CUTL_BUFFER::operator!() const            {return data ? FALSE : TRUE; }
inline BOOL CUTL_BUFFER::operator<(LPCSTR str) const  {return (UTL_strcmp(data, str) < 0); }
inline BOOL CUTL_BUFFER::operator<=(LPCSTR str) const {return (UTL_strcmp(data, str) <= 0);}
inline BOOL CUTL_BUFFER::operator==(LPCSTR str) const {return (UTL_strcmp(data, str) == 0);}
inline BOOL CUTL_BUFFER::operator==(LPSTR str)  const {return (UTL_strcmp(data, str) == 0);}
inline BOOL CUTL_BUFFER::operator!=(LPCSTR str) const {return (UTL_strcmp(data, str) != 0);}
inline BOOL CUTL_BUFFER::operator>=(LPCSTR str) const {return (UTL_strcmp(data, str) >= 0);}
inline BOOL CUTL_BUFFER::operator>(LPCSTR str) const  {return (UTL_strcmp(data, str) > 0); }


class CUT2_INI : public CUTL_BUFFER {
public: //constructores
   CUT2_INI(LPCSTR filename);
   virtual ~CUT2_INI();
public: //Funciones
   int    LoadInt(LPCSTR section, LPCSTR entry, int def=0);
   LPCSTR LoadStr(LPCSTR section, LPCSTR entry, LPCSTR def = NULL);
   long   LoadLong(LPCSTR section, LPCSTR entry, long def =0);
   BOOL   Write(LPCSTR section, LPCSTR entry, LPCSTR str) const;
   BOOL   Write(LPCSTR section, LPCSTR entry, int value) const;
   BOOL   Write(LPCSTR section, LPCSTR entry, long value) const;
   BOOL   Delete(LPCSTR section, LPCSTR entry=NULL) const;
   BOOL   Flush() const;
   LPCSTR LoadEntries(LPCSTR section);
   LPCSTR LoadSections();
   LPCSTR GetEntry(UINT pos) const;
public: // Operadores
   operator int();
   operator DWORD();

   /*--- Area X ------------------------------------*/
private: // miembros
   CUTL_BUFFER m_file;
   int   m_int;   
   long m_long;
};

/***********************************************************************************************/
/*** Clase para manipular fechas */
class CCR3_DATE {
public:
   CCR3_DATE();
   CCR3_DATE(LPCSTR date);
   CCR3_DATE(time_t timeExpr);
   CCR3_DATE(const CCR3_DATE &other);
   virtual ~CCR3_DATE();
   
   // Para operar la información directamente (Usar con cautela)
   operator    LPSTR();
   operator    LPCSTR();   
   operator    time_t();
   // Asignaciones
   CCR3_DATE &operator=(const CCR3_DATE &date);
   CCR3_DATE &operator=(LPCSTR date);   
   CCR3_DATE &operator=(time_t timeExpr);
   void        SysDate();
   BOOL        SetDate(int yy, int mm, int dd);
   BOOL        SetTime(int hh, int mi, int ss, int fs = 0);
   void        ResetTime();
   void        ResetDate();
   void        Clear();
   // Lecturas
   int m_yy, m_mm, m_dd, m_hh, m_mi, m_ss, m_fs;
   char m_info[18];
   // Operaciones
   BOOL        operator<(const CCR3_DATE &date);
   BOOL        operator<=(const CCR3_DATE &date);
   BOOL        operator>(const CCR3_DATE &date);
   BOOL        operator>=(const CCR3_DATE &date);   
   BOOL        operator==(const CCR3_DATE &date);   

   static UINT SizeOf();

public: // Función para permitir que CCR3_DATE sea clave de tablas CUTL_KEYMAP

   // Formateo
   enum formatStyle {DDMMYY=0, DDHHMI=1, DDMMYYHHMISS=2, DDMMYYHHMISSML=3, HHMISS=4, HHMI=5};
   void Format(formatStyle style, CUTL_BUFFER* target);
private:
   BOOL        ChkDate(int yy, int mm, int dd);
   BOOL        ChkTime(int hh, int mi, int ss, int ml);
};

/****************************************************************************************************************************/
/*** Clase para manipular rutas de ficheros (paths) */
class CUTL_PATH {
public: // Constantes
   enum SPECIALDIR {DIR_CURRENT, DIR_WINDOWS, DIR_SYSTEM, DIR_MODULE, DIR_TEMP};
   enum MAX_VALS {
      MAX_FILEPATHLENGTH = _MAX_PATH,
      MAX_FILENAMELENGTH = 8,
      MAX_FILEEXTLENGTH  = 3
   };
public: // Constructores
	CUTL_PATH();
	CUTL_PATH(const CUTL_PATH &other);
	CUTL_PATH(LPCSTR path);
	CUTL_PATH(SPECIALDIR specialDir);
	virtual ~CUTL_PATH();

public: //métodos de carga y/o configuración
	void Set(LPCSTR path);
	void SetDrive(char driveLeter);
	void SetDriveDirectory(LPCSTR pDriveDirectory);
	void SetDirectory(LPCSTR pDirectory, BOOL ensureAbsolute = FALSE);
	void SetName(LPCSTR pName);
	void SetNameExtension(LPCSTR pNameExtension);
	void SetExtension(LPCSTR pExtension);
	void AppendDirectory(LPCSTR pSubDirectory);
	void UpDirectory(CUTL_BUFFER* pLastDirectory = NULL);
	void SetComponents (LPCSTR drive, LPCSTR directory, LPCSTR name, LPCSTR extension);

public: //métodos automáticos de crecaión de rutas
	void CurrentDirectory();
	void WindowsDirectory();
	void SystemDirectory();
	void TempDirectory();

	void PrivateProfile();
	void LocalProfile(LPCSTR iniName, LPCSTR extension = NULL);

	void MakeRoot ();

	void Clear();

public: //métodos de consulta de datos
	void GetDrive(CUTL_BUFFER &drive) const;
	void GetDriveDirectory(CUTL_BUFFER &driveDirectory) const;
	void GetDirectory(CUTL_BUFFER &directory) const;
	void GetName(CUTL_BUFFER &name) const;
	void GetNameExtension(CUTL_BUFFER &nameExtension) const;
	void GetExtension(CUTL_BUFFER &extension) const;
	void GetComponents (CUTL_BUFFER *pDrive = NULL, CUTL_BUFFER *pDir = NULL, CUTL_BUFFER *pName = NULL, CUTL_BUFFER *pExt = NULL) const;
	BOOL GetFullyQualified(CUTL_BUFFER& fullyQualified) const;    
	BOOL GetTime(CCR3_DATE &date) const;

public: //métodos de actuación sobre la unidad
	BOOL IsRemovableDrive() const;
	BOOL IsCDRomDrive() const;
	BOOL IsNetworkDrive() const;
	BOOL IsRAMDrive() const;

	LONG DriveTotalSpaceBytes() const;
	LONG DriveFreeSpaceBytes() const;
	LONG GetDriveClusterSize() const;


public: //métodos de actuación sobre el directorio
	BOOL DirectoryExists() const;
	BOOL IsDirectoryEmpty() const;

	BOOL CreateDirectory(BOOL createIntermediates =TRUE);
	BOOL RemoveDirectory();
	BOOL RemoveDirectoryContent();
	BOOL ChangeDirectory();

public: //métodos de actuación sobre el fichero
   BOOL Exists() const;
   LONG GetSize() const; 

   BOOL Delete(BOOL evenIfReadOnly =TRUE);
   BOOL Rename(LPCSTR newPath);	

   BOOL FindFirst(DWORD attributes = _A_NORMAL);
   BOOL FindNext();

public: //operadores
	BOOL       IsEmpty()  const;
   BOOL       IsValid () const;
   BOOL       IsWild ()  const;
   
   CUTL_PATH& operator= (const CUTL_PATH &other);
   CUTL_PATH& operator= (LPCSTR path);
              operator LPCSTR() const;
   BOOL       operator == (const CUTL_PATH &other) const;
    

   /*--- Area X------------------------------------------------*/

protected: //otros métodos
   void        xInit();
   void        xClose();

   BOOL        xGetDiskFreeSpace(LPDWORD pSectorsPerCluster, LPDWORD pBytesPerSector, LPDWORD pFreeClusters, LPDWORD pClusters) const;


private: //miembros
   CUTL_BUFFER m_path;
   DWORD			m_findFileAttributes;
	HANDLE      m_hFindFile;
};


/**** Clase para traducir strings con sentencias de escape */
class CUTL_PARSE {
public:
   CUTL_PARSE();
   CUTL_PARSE(LPCSTR srcStream, LPCSTR charSet = NULL, char escape = NULL);
   virtual ~CUTL_PARSE();
public:
   enum token {T_STRING, T_ESCAPE, T_CHARSET, T_EOF, T_ERROR, T_NOTOKEN};
   //////////////////////////////////////////////////////////
   // Formato de los tokens
   // type      | NumArgs | Args
   //--------------------------------------------------------
   // T_STRING  |    1    | (Str) cadena
   // T_CHARSET |    1    | (chr) caracter
   // T_ESCAPE  | depende | depende
   // T_EOF     |    0    |
   // T_ERROR   |    0    | El mensaje de error queda registrado en CORE
   // T_NOTOKEN |    0    | 
   //////////////////////////////////////////////////////////
   
   void   SetCharSet(LPCSTR charSet, char escape = NULL);
   void   SetStream(LPCSTR srcStream);
   token  NextToken();
   token  LookAhead();
   void   Restart();
   // Para obtener la información del último token leido
   token  Type()                 { return m_tokenType; };
   UINT   NumArgs()              { return m_tokenNumArgs; };
   // lectura de argumentos con conversión
   LPSTR StrArg(UINT pos =1);
   DWORD  LongHexArg(UINT pos =1);
   long   LongArg(UINT pos =1);
   int    IntArg(UINT pos =1);
   char   ChrArg(UINT pos =1);

   //--- AREA X --------------------------------------------------------
private: 
   token xSetError();

private: // Miembros
   CUTL_BUFFER m_charSet;
   char        m_escape;
   // stream
   CUTL_BUFFER m_stream;
   // posición en el stream
   UINT        m_offset;
   // información del último token leido
   token       m_tokenType;
   UINT        m_tokenNumArgs;
   CUTL_BUFFER m_tokenArgs;  
};


//#ifdef UTILS

/*--- Constantes ------------------------------------------------------------*/

const char    PATH_DRIVE_DELIMITER  = ':';
const char    PATH_DIR_DELIMITER    = '\\';
const char    PATH_EXT_DELIMITER    = '.';

LPCSTR const  PATH_INI_EXT          = "ini";
LPCSTR const  PATH_WILD_NAME_EXT    = "*.*";
LPCSTR const  PATH_WILD_SET         = "?*";


void        StripLeadingChar(CUTL_BUFFER& text, char leadingCh);
void        StripLeadingBackslash (CUTL_BUFFER& directory);
void        StripTrailingChar(CUTL_BUFFER& text, char traillingCh);
void        StripTrailingBackslash (CUTL_BUFFER& directory);
void        EnsureTrailingBackslash (CUTL_BUFFER& directory);
void        EnsureLeadingBackslash(CUTL_BUFFER& directory);
BOOL        AttributesMatch(DWORD targetAttributes, DWORD fileAttributes);

//#endif