MultiClipboard Plugin for Notepad++
-----------------------------------

License: GPL (as impose by Notepad++'s GPL)
Author: LoonyChewy http://www.peepor.net/loonchew

About
-----
This plugin implements multiple (10) text buffers that is filled up via copying
and/or cutting of text. To paste any text from the buffers, use
    Ctrl-V or middle mouse click (normal paste):
        to paste the most recently copied/cut text
    Ctrl-Shift-V or Shift-middle mouse click:
        to pop up a menu with the text buffer entries. Select the desired menu
        item to paste it

Also have option to auto copy select text to clipboard, and to clear the clipboard
buffer.
Alternatively, there is an option to cycle through clipboard items using Ctrl+Shift+V,
instead of displaying the clipboard menu

Known Limitations/Bugs
----------------------
- Unicode text do not display correctly, although they copy and paste correctly
- Plugin somehow do not receive IDM_EDIT_COPY/CUT messages when the hotkey is pressed
  Hardcoded Ctrl+C/V to receive clipboard items, even if user remaps it to something else

Revision History
----------------
version 1.4.1 (2nd Feb 2008)
- Shift-middle-click also implements cycle through clipboard items when "Show Paste List" is unchecked (contributed by Bahman)
- Added version info that can be seen from the dll properties

version 1.4.0beta (26th Jan 2008)
- Patch contributed by Jens Lorenz
- Settings are saved to file, and restored upon loading of Notepad++
- Introduced an option to cycle through clipboard items via successive Ctrl+Shift+V,
  instead of displaying clipboard menu

version 1.3.1 (3rd Jan 2008)
- Unmodified patch contributed by Bahman
- Active selections are always replaced by the previous snippet with any variety of paste
  command,including middle-clicking/Ctrl-Shift-V
- System and Multi-clipboard are always in sync, so that any generic paste uses the last
  thing you copied/autocopied, regardless of whether either copy or paste takes place
  inside or outside of Notepad++.
  + Plus if the snippet is external it's automatically added to the copytextlist when
    pasted inside npp
    (Note: Only upon paste, not upon copy)
- Fix: Middle-clicking not clipboard list menu does not paste first item now; it is ignored

version 1.3 (6th Dec 2007):
- Updated to latest version of Notepad_plus_msgs.h and Scintilla.h
- Enabled text selection auto-copy by default
- Auto-copy text are passed to Windows clipboard buffer
- Added an option to use numbers as menu accelerator for clipboard items

version 1.2 (17th May 2007):
- Added clipboard items to the plugin's menu bar
- Clear clipboard buffer from the plugin's menu bar
- Option to toggle on/off middle mouse click pasting
- Experimental support for copy text on selection
  + Polls text selection at 0.5 seconds frequency and detects text selection changes
  + Detects if text selection overlaps previous one, or is a new selection, and add
    or replace clipboard item accordingly
- Option to toggle on/off auto text selection copying

version 1.1 (1st Apr 2007):
- Added middle mouse click/shift middle mouse click paste functionality

version 1.0 (19th Mar 2007):
- Initial release of this plugin. Implements multiple buffer and Ctrl-Shift-V
  popup menu for pasting, as well as normal Ctrl-V pasting