# fvp
fvp - All in one tool for favorite games

TODO:
Add bin archive support

usage:

	fvp -d input.hcb output.txt
	fvp -split strings.txt
	fvp -c [-ww charlimit] strings.txt script.txt output.hcb
	fvp -decode input [output]
	fvp -encode input [output]

-------------

Use mode "-d" to dump the opcodes for the given hcb file and dump strings to seperate file.

Use mode "-split" to split up the string file into parts. Parts need to be manually declared in the following fashion:

\<part name="Shinku Route" filename="shinku_route.txt"\> <br>
... <br>
Strings <br>
... <br>
\</part\>

Use mode "-c" to rebuild a new hcb file from string file and byte code file. If you split up your string script
into different parts and/or used the aforementioned split mechanism, you can use a build script instead of the full
string file for the first argument. As an example, let's say you divided the common route into chapters. A build script
will then look like this:

\<part filename="chapter_1.txt"\> <br>
\<part filename="chapter_2.txt"\> <br>
\<part filename="chapter_3.txt"\> <br>
\<part filename="chapter_4.txt"\> <br>
...

Both absolute and relative paths are allowed.


Use "-decode" mode to convert NVSG image(s) to PNG. You can either convert a single file or enter a folder
as input. All nvsg images found in that folder will then be automatically converted. You can specify output name
or folder, but this is optional.

Use "-encode" mode to convert PNG image(s) to NVSG. You can either convert a single file or enter a folder
as input. All PNG images found in that folder will then be automatically converted. You can specify output name
or folder, but this is optional.
Note: NVSG images have their ingame position hardcoded. For that purpose, "-decode" mode automatically writes, maintains
and updates a "pos.dat" file that stores extracted position informations. You absolutely must have used the tool to
decode an image you want to re-encode and make sure the "pos.dat" file is in the same folder as the fvp tool.

-------------

A quick rundown of the script syntax...

Comments are marked with a # sign.

Labels can be any one-line string that ends with a colon :
They are case-sensitive.

By default, labels will have an auto-generated name.
You can edit the label names in the script at any time.
Be sure to do a full search-and-replace when renaming labels in a script.

The tool will assume that a function is being declared when the "initstack" op
has been used. A label is required prior to "initstack", or the tool fails.

Labels used by "call" opcodes are not usable by "jmp" and "jmpcond" opcodes.
The converse is also true. If this is a problem, ask me and I can change it.

Function pointers can be used in pushint. Example: pushint LABEL:function_XXX_
This is typically used for "syscall ThreadStart" commands.

Near the end of the text dump, you can edit the "TITLE" line which contains
the text for the game's titlebar.

-----------

authors:
binaryfail - Most of the code related to dumping and rebuilding hcb files
akerou - Rest

-----------

version history:
v1 / 2015june02 - first version

