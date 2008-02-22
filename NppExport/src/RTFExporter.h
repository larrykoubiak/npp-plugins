#pragma once

#include "Exporter.h"

#define CF_RTF 			TEXT("Rich Text Format")

//size definitions for memory allocation
#define EXPORT_SIZE_RTF_STATIC		(21+11+5+12+5+3)	//header + fonttbl + fonttbl end + colortbl + colortbl end + eof
#define EXPORT_SIZE_RTF_STYLE		(11+27+27)			//font decl + color decl + color decl
#define EXPORT_SIZE_RTF_SWITCH		(33)				// '\f127\fs56\cb254\cf255\b0\i0\ul0 '

struct ExportData;
class Exporter;

class RTFExporter :
	public Exporter
{
public:
	RTFExporter(void);
	~RTFExporter(void);
	bool exportData(ExportData * ed);
	TCHAR * getClipboardType();
};
