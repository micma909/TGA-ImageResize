# TGA-ImageResize

An excercise I did in reading and resizing .tga images without using any pre-existing frameworks or libraries. The main class 'TargaHandler.cpp' uses a MemoryManager to pre-allocate memory for multiple resize operations. The app is built as a console application is run by providing the name of a input-file and its output destination as well as either one or two scaling factors like so:

> OriginalImage.tga ResizedOutput.tga 0.5 0.3 (i.e an 1920x1080 becomes 960x324)

If provided only one scaling factor the application assumes uniform scaling in X & Y. 
