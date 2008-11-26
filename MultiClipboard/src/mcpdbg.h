#ifndef MCP_DBG_H
#define MCP_DBG_H

#include "PluginInterface.h"
#include "stdio.h"

extern class McpDbg * pdbg;

class McpDbg
{
public:
	void init( NppData * pNppData ) { pnd = pNppData; }
	void printdbg(char * format, ...)
	{
		va_list args;
		va_start (args, format);
		vsprintf (dbgtxt,format, args);
		perror (dbgtxt);
		va_end (args);

		int currentEdit;
		::SendMessage( pnd->_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit );
		HWND hScintilla = (currentEdit == 0) ? pnd->_scintillaMainHandle : pnd->_scintillaSecondHandle;
		::SendMessage( hScintilla, SCI_INSERTTEXT, 0, (LPARAM)(dbgtxt) );
	}
private:
	NppData * pnd;
	char dbgtxt[1024];
};


#endif