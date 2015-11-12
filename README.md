**Copyright (C) 2015 [Wang Renxin](https://cn.linkedin.com/pub/wang-renxin/43/494/20). All rights reserved.**

**framepacker** is a freeware of texture bin packing.

## Algorithm

I used a bin packer algorithm refer to [Jake Gordon's project](https://github.com/jakesgordon/bin-packing).

## Code with framepacker

**framepacker** is implemented with C++ template, it requires a C++11 compatible compiler. `framepacker::packer` is where the pack algorithm is.

## Usage

 * framepacker FILE_LIST [-o output_file] [OPTIONS] - Packs some images
 * FILE_LIST
  * := file*
 * OPTIONS
  * := -p n    : Padding, 1 as default
  * := -s mxn  : Expected texture size, eg. 256x512, may enlarge result
  * := -t      : Disable rotation
  * := -w      : Disable forcing texture to POT (Power of 2)
  * := -m      : Disable alpha trim

## Performance

**framepacker** is fast, it packs 200 textures in less than 0.15s on common desktop machines.
