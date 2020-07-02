/*
** This source file is part of framepacker
**
** For the latest info, see https://github.com/paladin-t/framepacker/
**
** Copyright (C) 2020 Taylor Holberton
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

#include <framepacker.hpp>

#include <list>
#include <string>

#include <cstdlib>
#include <cstring>

// From 'third/' directory.
#include <stb_image.h>
#include <stb_image_write.h>

namespace fp = framepacker;

namespace {

using size_type = std::size_t;

using color_type = unsigned char;

struct rgba final {

	color_type data[4];

	inline static constexpr rgba from_buffer(const color_type* rgba_ptr) noexcept {
		return rgba {
			rgba_ptr[0],
			rgba_ptr[1],
			rgba_ptr[2],
			rgba_ptr[3]
		};
	}

	inline color_type& operator [] (size_type i) noexcept {
		return data[i];
	}

	inline const color_type& operator [] (size_type i) const noexcept {
		return data[i];
	}

	inline constexpr bool is_transparent() const noexcept {
		return data[3] == 0;
	}
};

class image final {
public:
	~image() {
		std::free(color_buffer);
		color_buffer = nullptr;
		w = 0;
		h = 0;
	}

	inline const char* get_path() const noexcept {
		return path.c_str();
	}

	size_type width() const noexcept {
		return w;
	}
	size_type height() const noexcept {
		return h;
	}

	inline constexpr const color_type* data(size_type x = 0, size_type y = 0) const noexcept {
		return &color_buffer[((y * w) + x) * 4];
	}

	inline color_type* data(size_type x = 0, size_type y = 0) noexcept {
		return &color_buffer[((y * w) + x) * 4];
	}

	inline constexpr rgba pixel(size_type x, size_type y) const noexcept {
		return rgba::from_buffer(&color_buffer[((y * w) + x) * 4]);
	}

	inline void pixel(size_type x, size_type y, const rgba &c) noexcept {
		auto* dst = data(x, y);
		dst[0] = c[0];
		dst[1] = c[1];
		dst[2] = c[2];
		dst[3] = c[3];
	}

	void copy_from(const image &src,
	               size_type src_x,
	               size_type src_y,
	               size_type width,
	               size_type height,
	               size_type dst_x,
	               size_type dst_y) {

		// TODO : Worth checking if 'src' is ever equal to 'this'.
		// If not, then we don't need the temporary buffer.

		color_type* tmp = (color_type*) std::malloc(width * height * 4);
		if (!tmp) {
			return;
		}

		// Copy from source onto temporary buffer.

		for (size_type y = 0; y < height; y++) {

			const color_type* src_ptr = src.data(src_x, src_y + y);

			color_type* dst_ptr = &tmp[y * width * 4];

			std::memcpy(dst_ptr, src_ptr, width * 4);
		}

		// Copy from temporary buffer to destination.

		for (size_type y = 0; y < height; y++) {

			const color_type* src_ptr = &tmp[y * width * 4];

			color_type* dst_ptr = data(dst_x, dst_y + y);

			std::memcpy(dst_ptr, src_ptr, width * 4);
		}

		std::free(tmp);
	}

	inline constexpr bool is_transparent(size_type x, size_type y) const noexcept {
		return pixel(x, y).is_transparent();
	}

	bool resize(size_type _w, size_type _h) noexcept {

		auto* tmp = (color_type*) std::realloc(color_buffer, _w * _h * 4);
		if (!tmp) {
			return false;
		}

		color_buffer = tmp;
		w = _w;
		h = _h;

		return true;
	}

	bool load_file(const char* p) noexcept {

		int tmp_w = 0;
		int tmp_h = 0;
		int tmp_n = 0;
		color_type* tmp = stbi_load(p, &tmp_w, &tmp_h, &tmp_n, 4);
		if (!tmp) {
			return false;
		}

		std::free(color_buffer);

		color_buffer = tmp;
		w = size_type(tmp_w);
		h = size_type(tmp_h);

		path = p;

		return true;
	}

	bool save_png(const char* p) noexcept {
		return stbi_write_png(p, w, h, 4, color_buffer, w * 4) == 0;
	}

private:
	std::string path;
	color_type* color_buffer = nullptr;
	size_type w = 0;
	size_type h = 0;
};

using packer_type = framepacker::packer<image>;

#define _BIN_FILE_NAME "framepacker"

#define _CHECK_ARG(__c, __i, __e) \
	do { \
		if(__c <= __i + 1) { \
			printf(__e); \
			return; \
		} \
	} while(0)

void show_tip(void) {
	printf("[Usage]\n");
	printf("  %s FILE_LIST [-o output_file] [OPTIONS] - Packs some images\n", _BIN_FILE_NAME);
	printf("\n");
	printf("  FILE_LIST := file*\n");
	printf("  OPTIONS   := -p n    : Padding, 1 as default\n");
	printf("            := -t      : Disable rotation\n");
	printf("            := -w      : Disable forcing texture to POT (Power of 2)\n");
	printf("            := -m      : Disable alpha trim\n");
}

void process_parameters(int argc, char* argv[]) {
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
		if(!std::memcmp(argv[i], "-", 1)) {
			if(!std::memcmp(argv[i] + 1, "o", 1)) {
				m = 'o';
				_CHECK_ARG(argc, i, "-o: File name expected.\n");
				options.push_back(std::make_pair(m, argv[++i]));
				++i;
			} else if(!memcmp(argv[i] + 1, "p", 1)) {
				_CHECK_ARG(argc, i, "-p: Padding size expected.\n");
				padding = std::max(std::atoi(argv[++i]), 0);
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

	packer_type packer;
	packer.padding = padding;
	packer.output_texture_size = fp::vec2(w, h);
	packer.allow_rotate = allow_rotate;
	packer.power_of_2 = power_of_2;
	packer.alpha_trim = alpha_trim;
	packer.comparer = packer_type::compare_area;

	for(auto it = inputs.begin(); it != inputs.end(); ++it) {

		image* img = new image;

		if (!img->load_file(it->c_str())) {
			// TODO : handle this
		}

		packer_type::texture_type texture(img);

		packer.add(it->c_str(), texture);
	}

	// Packs it.
	packer_type::texture_type result(new image);
	packer_type::texture_coll_type packed;
	packer_type::texture_coll_type failed;
	packer.pack(result, packed, failed);

	// Prompts.
	{
		for(auto it = packed.begin(); it != packed.end(); ++it) {
			packer_type::block_type &blk = it->second;
			printf("  %s [%d, %d] - [%d, %d]\n", it->first.c_str(), blk.fit->min_x(), blk.fit->min_y(), blk.fit->min_x() + blk.fit->width(), blk.fit->min_y() + blk.fit->height());
		}
	}
	if(failed.size()) {
		for(auto it = failed.begin(); it != failed.end(); ++it) {
			printf("  %s\n", it->first.c_str());
		}
	}
	{
	}

	// Serializes.
	{
		std::string out_tex = outpath + ".png";
		result->save_png(out_tex.c_str());
		printf("Texture written: %s.\n", out_tex.c_str());
	}
	{
		std::string out_meta = outpath + ".json";
		std::ofstream fs(out_meta);
		packer.write_meta(fs, packed, result, outpath.c_str());
		fs.close();
		printf("Meta data written: %s.\n", out_meta.c_str());
	}
}

} // namespace

int main(int argc, char* argv[]) {

	if(argc == 1)
		show_tip();
	else if(argc >= 2)
		process_parameters(argc, argv);

	return 0;
}
