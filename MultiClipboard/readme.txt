MultiClipboard 2
================

Author: LoonyChewy
Website: http://www.peepor.net/loonchew/index.php?p=multiclipboard

Summary
-------
This is a plugin for Notepad++. It keep tracks of the most recently copied text in Notepad++
and/or the OS clipboard, and allows the user to retrieve and paste these text back to their
documents.

Features
--------
+ Natively supports unicode
+ Keeps text that is copied to the OS clipboard, either
   1. when copied from within Notepad++ (the current option)
   2. from any program currently running
   3. or from any program, but only when text is pasted into Notepad++
+ A sidebar with a list to display all the stored text
   1. Shift the relative position of the text
   2. Delete the text
   3. Paste the text by double-clicking it, or clicking the paste icon
   4. An edit box in the sidebar to edit the current selected text in the list
+ A pop-up paste menu that displays the stored text selecting text to paste
   1. Ctrl-Shift-V (customisable) to activate
   2. Customisable width
   3. Numeric mnemonic
+ Option to use cyclic clipboard ring instead of paste menu
   1. Ctrl-Shift-V (customisable) repeatedly to cyclic through, release keyboard to select
+ Option to use middle mouse button click to paste last stored text from clipboard
   1. Shift-Middle click to show paste menu
+ Option to auto copy selected text from document to clipboard

Useful Tips
-----------
To show the multiclipboard paste menu via (right click) context menu, add the following line to %Notepad++Directory%\contextMenu.xml 
<Item PluginEntryName="MultiClipboard" pluginCommandItemName="MultiClipboard Paste"/>

Known Issues
------------
- When the document is converted from one encoding to another in Notepad++, the text is added
  to the MultiClipboard

Version History
---------------
20 Feb 2009 - MultiClipboard 2.0 Preview 3
-----------
1. Added cyclic clipboard paste as an alternative to paste menu on Ctrl-Shift-V
2. Added auto copying of selected text to clipboard
3. Added middle click paste and shift-middle click to show paste menu
4. Paste menu pop-up location is at caret when activated by keyboard, and at mouse cursor
   when activated by mouse
(Internal code changes)
5. Refactored the code for the settings dialog (no user functionalities changed)
6. Refactored the code to delete the IView class and inherit all MVC classes from IController
   as the two classes are too similar
7. Added a Init( ... ) function to IController class to encapsulate all initialisations
   within itself


22 Nov 2008 - MultiClipboard 2.0 Preview 2
-----------
1. Added NativeLang support
2. Updated UI look and feel and icons to match those of other Notepad++ plugins, modify listbox and editbox fonts (to courier new, non bold) and tab stop width for the latter (4 chars)
3. After deleting a clipviewer item, the next item in the list will be selected
4. Edit box will be enabled only when a valid list item is selected
5. After clicking or double-clicking a list item, input focus will go back to the active scintilla view
6. Added an options dialog, and these settings are saved to xml file for persistent across sessions
7. Upgraded to unicode version. From now on this plugin will only work with unicode version of Notepad++ (version 5.1 unicode and above)
8. Added the original paste menu that is activated via Ctrl-Shift-V