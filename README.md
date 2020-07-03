# framepacker

**framepacker** is a freeware which implements a texture bin packing algorithm. It's similar to texture tools like the TexturePacker. You are free to use, copy, modify or distribute it under the MIT license.

It implements a bin packer algorithm refer to the [Binary Tree Bin Packing Algorithm](http://codeincomplete.com/posts/2011/5/7/bin_packing/).

## Compile

A C++ 11 compiler is required to compile the source code. The `framepacker::packer` template class in `framepacker.hpp` is the algorithm implementation. And it offers a specialization in `framepacker.cpp` using stb image.

## Usage

It works as a command line tool:

* framepacker FILE_LIST [-o output_file] [OPTIONS] - Packs some images
* FILE_LIST
	* := file *  : Eg. file1.png file2.png
* OPTIONS
	* := -p N    : Padding, default to 1
	* := -s MxN  : Expected texture size, eg. 256x512, may enlarge result
	* := -t      : Disable rotation
	* := -w      : Disable forcing texture to POT (Power of 2)
	* := -m      : Disable alpha trim

Eg. "**framepacker** foo.png bar.png -o out", it generates a packed `out.png` image and another `out.json` meta data which includes packing information to look up slices from the image.

## Performance

**framepacker** is fast, it packs 200 textures in less than 0.15s on common desktop machines at 2015.
