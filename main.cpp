/*
** This source file is part of framepacker
**
** For the latest info, see https://github.com/paladin-t/framepacker/
**
** Copyright (C) 2015 Wang Renxin
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of
** this software and associated documentation files (the "Software"), to deal in
** the Software without restriction, including without limitation the rights to
** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
** the Software, and to permit persons to whom the Software is furnished to do so,
** subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
** FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
** COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
** IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framepacker.hpp"
#include <list>
#include <string>
#include <Windows.h>
#include "freeimage.h"

using namespace framepacker;

namespace framepacker {

struct color {
	color() : r(0), g(0), b(0), a(0) {
	}
	color(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) : r(_r), g(_g), b(_b), a(_a) {
	}

	bool is_transparent(void) const {
		return a == 0;
	}

	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

class freeimage {

public:
	freeimage() : handle(NULL), w(0), h(0) {
	}
	freeimage(const char* p) {
		load_file(p);
	}
	~freeimage() {
		unload();
	}

	const char* get_path(void) const { return path.c_str(); }

	int width(void) const { return w; }
	int height(void) const { return h; }

	BYTE* bytes(void) const { return FreeImage_GetBits(handle); }

	color pixel(unsigned x, unsigned y) const {
		RGBQUAD col;
		FreeImage_GetPixelColor(handle, x, y, &col);
		color ret;
		ret.r = col.rgbRed;
		ret.g = col.rgbGreen;
		ret.b = col.rgbBlue;
		ret.a = col.rgbReserved;

		return ret;
	}
	void pixel(unsigned x, unsigned y, color &c) {
		RGBQUAD col;
		col.rgbRed = c.r;
		col.rgbGreen = c.g;
		col.rgbBlue = c.b;
		col.rgbReserved = c.a;
		FreeImage_SetPixelColor(handle, x, y, &col);
	}

	void copy_from(freeimage &src, unsigned left, unsigned top, unsigned width, unsigned height, unsigned x, unsigned y) {
		FIBITMAP* fib = FreeImage_Copy(src.handle, left, top, left + width, top + height);
		FreeImage_Paste(handle, fib, x, y, 255);
		FreeImage_Unload(fib);
	}

	bool is_transparent(unsigned x, unsigned y) const {
		color c = pixel(x, y);

		return c.is_transparent();
	}

	bool resize(int _w, int _h) {
		unload();
		handle = FreeImage_Allocate(_w, _h, 32);
		w = _w;
		h = _h;

		return true;
	}

	bool load_file(const char* p) {
		path = p;

		handle = NULL;
		w = 0;
		h = 0;

		FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(path.c_str());
		if(fif != FIF_UNKNOWN) {
			handle = FreeImage_Load(fif, path.c_str());
			if (FreeImage_GetBPP(handle) != 32)  {
				FIBITMAP* tmp = handle;
				handle = FreeImage_ConvertTo32Bits(tmp);
			}

			w = FreeImage_GetWidth(handle);
			h = FreeImage_GetHeight(handle);

			return true;
		}

		return false;
	}

	void unload(void) {
		if(!handle)
			return;

		FreeImage_Unload(handle);
		handle = NULL;
	}

	void save(const char* p) {
		FreeImage_Save(FIF_PNG, handle, p);
	}

private:
	std::string path;
	FIBITMAP* handle;
	unsigned w;
	unsigned h;

};

}

typedef packer<freeimage> packer_type;

#define _BIN_FILE_NAME "framepacker"

#define _CHECK_ARG(__c, __i, __e) \
	do { \
		if(__c <= __i + 1) { \
			printf(__e); \
			return; \
		} \
	} while(0)

static unsigned short _set_console_color(unsigned short col) {
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	if(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo)) {
		printf("GetConsoleScreenBufferInfo error!\n");

		return 0;
	}
	if(!SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), col)) {
		printf("SetConsoleTextAttribute error!\n");

		return 0;
	}

	return csbiInfo.wAttributes;
}

struct ConsoleColor {
	ConsoleColor(unsigned short col) {
		old = _set_console_color(col);
	}
	~ConsoleColor() {
		_set_console_color(old);
	}

	unsigned short old;
};

#define CONSOLE_COLOR(C) ConsoleColor __##__FUNCTION__##__LINE__(C);

static void _show_tip(void) {
	printf("[Usage]\n");
	printf("  %s FILE_LIST [-o output_file] [OPTIONS] - Packs some images\n", _BIN_FILE_NAME);
	printf("\n");
	printf("  FILE_LIST := file*\n");
	printf("  OPTIONS   := -p n    : Padding, 1 as default\n");
	printf("            := -s mxn  : Expected texture size, eg. 256x512, may enlarge result\n");
	printf("            := -t      : Disable rotation\n");
	printf("            := -w      : Disable forcing texture to POT (Power of 2)\n");
	printf("            := -m      : Disable alpha trim\n");
}

static void _process_parameters(int argc, char* argv[]) {
	int i = 1;
	std::string m = "\0";
	int padding = 1;
	int w = 0, h = 0;
	bool allow_rotate = true;
	bool power_of_2 = true;
	bool alpha_trim = true;

	std::list<std::string> inputs;
	std::list<std::pair<std::string, std::string> > options;

	// Parses arguments.
	while(i < argc) {
		if(!memcmp(argv[i], "-", 1)) {
			if(!memcmp(argv[i] + 1, "o", 1)) {
				m = 'o';
				_CHECK_ARG(argc, i, "-o: File name expected.\n");
				options.push_back(std::make_pair(m, argv[++i]));
				++i;
			} else if(!memcmp(argv[i] + 1, "p", 1)) {
				_CHECK_ARG(argc, i, "-p: Padding size expected.\n");
				padding = max(std::atoi(argv[++i]), 0);
			} else if(!memcmp(argv[i] + 1, "s", 1)) {
				_CHECK_ARG(argc, i, "-s: Size expected.\n");
				char buf[256];
				_snprintf_s(buf, sizeof(buf), argv[++i]);
				char* ctx = NULL;
				char* tmp = strtok_s(buf, "x", &ctx);
				w = std::atoi(tmp);
				h = std::atoi(ctx);
				++i;
			} else if(!memcmp(argv[i] + 1, "t", 1)) {
				allow_rotate = false;
				++i;
			} else if(!memcmp(argv[i] + 1, "w", 1)) {
				power_of_2 = false;
				++i;
			} else if(!memcmp(argv[i] + 1, "m", 1)) {
				alpha_trim = false;
				++i;
			} else {
				printf("Unknown argument: %s.\n", argv[i++]);
			}
		} else {
			inputs.push_back(argv[i++]);
		}
	}

	// Tells output path.
	std::string outpath = "output.png";
	for(auto it = options.begin(); it != options.end(); ++it) {
		if(it->first == "o")
			outpath = it->second;
	}

	// Prepares packing.
	DWORD time = GetTickCount();

	packer_type packer;
	packer.padding = padding;
	packer.output_texture_size = vec2(w, h);
	packer.allow_rotate = allow_rotate;
	packer.power_of_2 = power_of_2;
	packer.alpha_trim = alpha_trim;
	packer.comparer = packer_type::compare_area;

	for(auto it = inputs.begin(); it != inputs.end(); ++it) {
		packer_type::texture_type img(new freeimage(it->c_str()));

		packer.add(it->c_str(), img);
	}

	DWORD load_time = GetTickCount() - time;
	time = GetTickCount();

	// Packs it.
	packer_type::texture_type result(new freeimage);
	packer_type::texture_coll_type packed;
	packer_type::texture_coll_type failed;
	packer.pack(result, packed, failed);

	DWORD pack_time = GetTickCount() - time;

	// Prompts.
	{
		CONSOLE_COLOR(FOREGROUND_INTENSITY);
		printf("Packed %d:\n", packed.size());
		for(auto it = packed.begin(); it != packed.end(); ++it) {
			packer_type::block_type &blk = it->second;
			printf("  %s [%d, %d] - [%d, %d]\n", it->first.c_str(), blk.fit->min_x(), blk.fit->min_y(), blk.fit->min_x() + blk.fit->width(), blk.fit->min_y() + blk.fit->height());
		}
	}
	if(failed.size()) {
		CONSOLE_COLOR(FOREGROUND_RED | FOREGROUND_GREEN);
		printf("Failed %d:\n", failed.size());
		for(auto it = failed.begin(); it != failed.end(); ++it) {
			packer_type::block_type &blk = it->second;
			printf("  %s\n", it->first.c_str());
		}
	}
	{
		CONSOLE_COLOR(FOREGROUND_INTENSITY);
		printf("Cost time: %gs loading; %gs packing.\n", (float)load_time / 1000.0f, (float)pack_time / 1000.0f);
	}

	// Serializes.
	{
		CONSOLE_COLOR(FOREGROUND_INTENSITY);
		std::string out_tex = outpath + ".png";
		result->save(out_tex.c_str());
		printf("Texture written: %s.\n", out_tex.c_str());
	}
	{
		CONSOLE_COLOR(FOREGROUND_INTENSITY);
		std::string out_meta = outpath + ".json";
		std::ofstream fs(out_meta);
		packer.write_meta(fs, packed, result, outpath.c_str());
		fs.close();
		printf("Meta data written: %s.\n", out_meta.c_str());
	}
}

static void _on_startup(void) {
	FreeImage_Initialise();
}

static void _on_exit(void) {
	FreeImage_DeInitialise();
}

int main(int argc, char* argv[]) {
	atexit(_on_exit);

	_on_startup();

	if(argc == 1)
		_show_tip();
	else if(argc >= 2)
		_process_parameters(argc, argv);

	return 0;
}
