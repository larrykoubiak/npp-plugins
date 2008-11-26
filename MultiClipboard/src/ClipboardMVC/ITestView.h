#include "ModelViewController.h"
#include "ClipboardList.h"
#include "stdio.h"


class ITestView : public IView
{
public:
	virtual void OnModelModified()
	{
#if 0
		ClipboardList * pList = (ClipboardList*)GetModel();
		FILE * f = fopen( "ITestView.txt", "wb" );
		char bom[2] = { 0xff, 0xfe };
		fwrite( bom, 2, 1, f );
		for ( unsigned int i = 0; i < pList->GetNumText(); ++i )
		{
			//char * pText = 0;
			//size_t textlen = 0;
			std::wstring text( pList->GetText( i ) );
			/*textlen = wcstombs( 0, text.c_str(), 0 ) + 1;
			if ( textlen < 0 )
			{
				continue;
			}
			pText = new char[textlen];
			memset( pText, 0, textlen );
			textlen = wcstombs( pText, text.c_str(), textlen );
			fprintf( f, "%s\n", pText );
			delete [] pText;*/
			const wchar_t * ptext = text.c_str();
			char a[100];
			memset( a, 0, 100 );
			memcpy( a, text.c_str(), sizeof( wchar_t ) * text.size() );
			fwrite( text.c_str(), sizeof( wchar_t ), text.size(), f );
			fwrite( L"\r\n", sizeof( wchar_t ), 2, f );
		}
		fclose( f );
#endif
	}
};