What does this program:

This sources contains a dockable function list for Notepad++ version 5.1 UNICODE.
With this plugin you could easy browse between functions in a file.

_______________________________________________________________________________
Intro

Hello Notepad++/Function List user. I hope you enjoy my plug-ins for Notepad++
and you are working less time with this fantastic editor and the plug-ins.

With this new version I try to add more possibilities and useful gadgets in
Function List. I would like to make it more clear in use. Please help me to
make it more stable and help me and the community to make new parsing rules.

In the next chapters I show you how to install and how to use it. I would show
you the restrictions and the advantages versus the old version.


_______________________________________________________________________________
Install

This is the easiest part for me to explain and the easiest part for you to
install ;).

!!! NOTE: BE SURE THAT YOU USE NOTEPAD++ VERSION 5.1 UNICODE !!!

You will find following files in the zipped archive:
- readme.txt                    [this text file]
- FunctionList.dll              [plugin]
- C++.flb                       [Function List Bitmap (flb) file]
- FunctionListRules.xml         [XML parsing rules database]

Copy the *.dll into the "Plugins" folder on Notepad++ install folder.
Both other files copy into the "Plugins/Config" folder. This could be in the
install folder of Notepad++ or in the %APPDATA% folder. This depends on your
installation. Did you install Notepad++ 5.1 with the installer -> copy it
into the "%APPDATA%/Plugins/Config/" folder.


_______________________________________________________________________________
What is new

You have added some new awesome features. The function list could now be 
displayed as a tree or as a list (like the old version of Function List). 
Change it in menu, icon bar or via right mouse click. 
You can create own groups of parsing rules for every language - yes for every!
A base set of rules, converted from the old Function List, is available. You
can modify/change it as you like. Make suggestion and sent them to me. I add
them into the next release of Function List. Add new rules...
It is possible to assign for every group of rules an icon. Therefore you need
only to create your own icon bitmap file. I created a small example file for
C++.
A filter is now available in list. Type in what your are searching for...


_______________________________________________________________________________
How to make parsing rules

This is not so easy to explain. At first you need to know how to work with RE
(Regular Expressions).
Open "Plugins->Function List->Language Parsing Rules...". You see a new dialog
to change/modify the rules. Here you can change for each language the rules.
To understand how does a parsing works, please have a review to the other
parsing rules and to the help dialog box. Open it by click on help within the
"Language Parsing Rules" box.
The tree can show at maximum one group with a set of currently one subgroups.
For example:

O INCLUDE  
O CLASS  
|-O Class name  
| |-O ENUM  
| | |-- Enum 1  
| | '-- Enum 2  
| '-O METHODE  
|   |-- Metohde 1  
|   '-- Methode 2 

Therefore you only need to change the combo box in "Group Properties". Change
the subgroup combo box of CLASS to METHODE.

BTW: If you added your own User Defined Language you will also see the language
in the left list at bottom.


_______________________________________________________________________________
How to assign icons to a group

To show you an example I added the "C++.flb" file. To make it workable open the
"Language Parsing Rules" dialog box and select the language C++. Click on the
right top side the check box "Bitmap List" and select the "C++.flb" file.
Now you will see the assigned icons in the group list.
If you would like to create your own icons, you need only to rename the file
into "*.bmp". The format of the file must be ([x mod 16] * 16).
Please help to enlarge the database of bitmap icon files.


_______________________________________________________________________________
Current Limitations

The function list parsing rules contains not all languages. It is up to you
to extend the rules for your favorite one. To test it the function list works
have a look into the plugins menu under: 
"Function List->Language Parsing Rules..." language 'C' or 'C++'. Here you
should find some rules.


_______________________________________________________________________________
The Project

For generating the dll file use VC++ 7. The project is included.
If you create a new project for VC 8 or MinGW please sent me the
complete project. I will integrate it.

The plugins side is:
https://sourceforge.net/projects/npp-plugins


Have fun

Jens Lorenz
jens.plugin.npp@gmx.de
