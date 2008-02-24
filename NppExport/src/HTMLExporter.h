#pragma once

#include "Exporter.h"

#define CF_HTML			TEXT("HTML Format")

//size definitions for memory allocation
#define EXPORT_SIZE_HTML_STATIC		(266)			//including default style params
#define EXPORT_SIZE_HTML_STYLE		(175)			//bold color bgcolor
#define EXPORT_SIZE_HTML_SWITCH		(34)			//<span ...></span>
#define EXPORT_SIZE_HTML_CLIPBOARD	(105+22+20)		//CF_HTML data
#define EXPORT_SIZE_HTML_UTF8		(65)			//UTF8 meta tag
struct ExportData;
class Exporter;

class HTMLExporter :
	public Exporter
{
public:
	HTMLExporter(void);
	~HTMLExporter(void);
	bool exportData(ExportData * ed);
	TCHAR * getClipboardType();
};
