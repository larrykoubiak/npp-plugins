[Introduction]
This plug-in provides two core functions to Notepad++:
- HTML and XML tag matching, like the built-in highlighting of matching braces
- HTML entity encoding/decoding (example: é to &eacute; and back)

It originated in these requests on the Plugin Development forum on SourceForge.net:
- http://sourceforge.net/forum/message.php?msg_id=4284078
- http://sourceforge.net/forum/message.php?msg_id=4443898


[Installation]
As usual: extract the DLL and accompanying files to the plugins folder under 
%ProgramFiles%\Notepad++ (or wherever you're running Notepad++ from) or %AppData%\Notepad++, and 
(re)start Np++.


[Usage]
- Ctrl+T to select the matching tag;
- Shift+Ctrl+T to select both tags and the entire contents in between. 

- Ctrl+E to Encode all selected non-ASCII characters to their HTML entities;
- Shift+Ctrl+E to dEcode all selected HTML entities.

All options are available under the Plugins menu item 'HTML Tag'; the shortcut keys can be adjusted 
with the shortcut mapper.


[Files]
HTMLTag.dll	            The actual plug-in.
HTMLTag-entities.ini	This file contains the list(s) of entities that will be processed. For now,
                        only the list headed with [HTML 4] will be used.  This can be adjusted if 
						you want to use other entities.
HTMLTag-readme.txt		The file you're reading now.


[History]
- 0.1 - 2007-08-05 20:00
  * Initial publication
- 0.2 - 2007-08-12 18:00
  * Added entity encoding/decoding
  * Tag detection is now case-sensitive in XML files (where the XML highlighter is used; 
    i.e. LangType = L_XML)
  * Uploaded the source to the Npp-plugins projects on SF.net 


[To do]
* Make it play nice with PHP & ASP tags (either highlight those separately or ignore them) 
* Write up a little readme, include that in the distribution zip, and post it on some web page 
* Make highlighting behave like brace highlighting (e.g. highlight current tag name and matching
  tag name when the text cursor is within either tag)
* Once live highlighting is implemented, add options window to specify which languages should 
  support Live Tag highlighting, and enable turning it off altogether
* Take CDATA segments into account


[About]
HTMLTag plug-in for Notepad++
Written by Martijn Coppoolse - http://martijn.coppoolse.com/software/
Using Delphi 2006