# Haviour
An editor for editing unpacked Havok behaviour files (.hkx) used by Skyrim and potentially other (Bethesda) games.

## Quick Guide
*Haviour uses [ImGui Docking Branch](https://github.com/ocornut/imgui/tree/docking) for its UI elements, therefore inherits most of ImGui's control, meaning that even the author might not know some of its behaviours. Please refer to ImGui's wiki for more info on UI navigation.*

*This guide is not going to teach you how to edit behaviour.*

### Files
As stated, Haviour is capable of editing **unpacked** hkx files. To get one, use [hkxcmd](https://github.com/figment/hkxcmd) to **convert** a 32-bit hkx into an unpacked one. After conversion, open the output with notepad and you should be able to see some sort of xml-like structure. If you see gibberish, that means it is still compressed and unusable.

If you are editing behaviours of humanoid characters in Skyrim, the files you are interested are ones under <u>meshes/actors/character/</u> within <u>Update.bsa</u>, specifically the ones in <u>behaviors</u> folder which contain the behaviour graphs.

To open a file, click on **File** menu at the top-left corner. Here you can choose to open a file by clicking **Open**, or create a knew behaviour file from scratch with **New**. After loading/creating a file, Haviour with automatically select the file for edit.

You can load multiple files, and all loaded files are listed below in the **File** menu. Clicking on the filename will set it as the active file for edit, since Haviour can only edit a single file at one time.

Additionally you can unpack and load character files (<u>Update.bsa/meshes/actors/character/characters (female)/default(fe)male.hkx</u>) and skeleton files (<u>Animations.bsa/meshes/actors/character/character assets (female)/skeleton(_female).hkx</u>). A character file contains animation names and character properties, and a skeleton file contains names and hierarchy for bones in the rig. Options to load them are under **Resources** menu, next to **File**. You can also click on the file name to edit them.

If you are done editing, you can save the active file by going to **File** -> **Save** to overwrite original file or to **Save As** to make a new file. After that, if you don't want the file to occupy your PC's memory, close them with **File** -> **Close** to close the active behaviour file you are currently editing. Alternatively, **Close All** closes them all.

### Windows
Haviour provides multiple window interfaces for editing a behaviour file. They are listed under **Window** menu to be toggled on/off, and can be moved around and resized.

### List View
An hkx file, essentially, is a big container of many hkx class **objects**, and these objects are linked by referencing their **ID** e.g. #0010, #1024 in their member fields. This window displays all **objects** within a hkx file, allowing you to sort by name/id and filter through them, as well as to create new objects.

The first row is the filter row. You can filter their names, or filter id by selecting **Filter ID** option, by typing your search text in the **Filter** text box.

The second row is the class row. You can choose to only view objects of a certain class by selecting one of the classes in the **Class** combo box. Selecting **All** displays all objects. You can create a new object by clicking the **+** button next to it. If you are currently only viewing certain class, then it will create an object of that class, otherwise it will prompt you to select one of the available classes in a popup.

At the bottom of the window is the object table. Click on the headers to sort them by ID/name/class in ascending/descending order. Click on the texts to copy them to the clipboard. Click the buttons on the left to edit one of the objects in **Property Editor** (**pencil**) or delete it if it is not referenced by any other objects (**waste bin**).

### Column View
In a behaviour file, you start from then root generator node, and then traverse down the big behaviour tree/DAG to visit all branches of the behaviour. Thus **Column View** is introduced to better display this kind of structure, and therefore only available to behaviour files.

In column view, you select one of the nodes/objects in a certain column, and it will be highlighted in green and its children will be shown in the next column. You can select multiple nodes by holding Shift key when clicking, and select all descedents of a node by holding Ctrl key. *Expanding all children might kill your framerate on bigger files*. If you know the id of the node that you are looking for, you can fill in **Navigate to** textbox, press enter and that node will be selected. Click the **pencil** button to edit one of the nodes/objects in **Property Editor**.

You can adjust the column width and item height with the sliders on the left. If you deselect **Align with Children** option, then all empty spaces in the columns will be omitted. This is for quickly switching between parallel nodes that are far apart in the branches.

### Property Editor
**Property Editor** is where you edit objects within your hkx. To edit an object, either type the id in the **ID** textbox and press Enter, or select one in other windows. You will have your object selection history on the left, and all objects that the current object is referenced by on the right. In the middle is the where most of the edits are happening.

An object may have many types of member and some of them are compound objects as well. Essentially all objects can be disassembled into a combination of the basic types, which are:
- **booleans/integers/real numbers**.
- **strings**.
- **enums**, displayed in combo boxes.
- **flags**, marked with a **flag** button at the right which allows you to view and select each of the flag values.
- **pointers**, pointing to either null or ID of a valid object, has a **right arrow(Go to)** button for editing pointed object, and a **plus(Append)** for creating an object of a compatible class and use its ID as value.
- **indices**, which are essentially integers that implicitly point to some objects/variables, most of which has a **magnifying glass** button for quick selection.
- **arrays** that contains multiple unnamed members of a certain type.

Some objects can have a **variable binding set**, which bind values of their member(s) to global variables or character properties. Members of these objects will have a **chain** (for bound member) or an **unlinked chain** (for unbound member) button to the right. Clicking on it will trigger a popup that display which variable the member is bound to if there is one, and options to quickly select a variable/property to bind it with. For an object that has no binding set option, the button will be disabled and greyed out.

To the right of **ID** textbox are certain actions you can do to the object you are currently editing. **Copy** copies the ID of the object being edited, and if there is an ID in your clipboard and it points to an object of the same class, **Paste** will overwrite all members of the current object with ones of that. **Resets** reset all values to default if the class is registered in the program. **Macros** displays specific actions you can do to the object that may simplify some process. Currently the only macro available is for parsing triggers in hkbClipTriggerArray.

### Global List
Behaviour/Character files will have some values that are shared and used by many objects in the project. They can be viewed and edited in **Global List** window, which is split into left/right sections the content of which will change depending on the active file.

When the active file is a behaviour file, the left section is either the **Variable/Character Property/Attribute List**, depending on which one is selected up above. The right section is the **Event List**.

When the active file is a skeleton file, the left section is the **Character Property List** (different from the one in behaviour files and more akin to a variable list), and the right section is the **Animation List**.

Each list will have a **Filter** textbox that filters both the name and the index at the same time, and a **plus** button that appends a new entry to the bottom of the list. You can edit/delete the entries by clicking on them. If you wish to delete an animation in the **Animation List**, leave the entry empty and it will be automatically removed.

Some lists contains entries that are referenced by index, which is shown on the left of each row. Removing an item in the middle of the list will not automatically reindex rest of the entries. It is done manually by clicking the **#** button above the list that also remaps all references to the old indices. However, when saving a file this process is done automatically.

An entry can only be removed when there is no reference to it, otherwise the ID of one of the referencing objects will be copied to the clipboard. Some lists has a **filter** button that could automatically removed all unreferenced entries. Use it with caution, though, since I may fail to consider certain use cases.

Specially, **Variables** are colour coded according to their data types. The colour scheme can be viewed by hovering over a **question mark** button, and their types can be viewed by hovering over their index.

### **Edit** Menu
There are some other actions you can do in the **Edit** menu.

#### Build Reference List ####
Reference lists are used by the 'Referenced by' panel in **Property Editor**, navigation in **Column View** as well as checks before deletion. When loading a file this list is automatically generated and handled by rest of the program while editing. However, I might f*ck up, so if you're feeling like something wrong with the references, use this to regenerate anew.

#### Reindex Objects ####
Similar to globals, object ID will not change when deleting objects, and new IDs will always start from the greatest ever existed. If you used up too much ID click this to clean up the ID vaccums. This process is automatically done when opening a new file.

#### Global Macros ####
Below aforementioned options are other miscellaenous functions that I don't know where to put. Their usage are self-contained and dispalyed in their own window that I'm not going to write here. It's already too much writing for me!