//========== Copyright (c) 2012, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		image wrapper
//
// $NoKeywords: $img
//===============================================================================//

#include "Image.h"
#include "Engine.h"
#include "Environment.h"
#include "File.h"

#include <png.h>
#include <turbojpeg.h>
#include <zlib.h>

#include <csetjmp>
#include <cstddef>
#include <cstring>
#include <mutex>

// this is complete bullshit and a bug in zlib-ng (probably, less likely libpng)
// need to prevent zlib from lazy-initializing the crc tables, otherwise data race galore
// literally causes insane lags/issues in completely unrelated places for async loading
static std::mutex zlib_init_mutex;
static std::atomic<bool> zlib_initialized{false};

static void garbage_zlib()
{
	if (zlib_initialized.load(std::memory_order_acquire))
		return;
	std::lock_guard<std::mutex> lock(zlib_init_mutex);
	if (zlib_initialized.load(std::memory_order_relaxed))
		return;
	uLong dummy_crc = crc32(0L, Z_NULL, 0);
	const char test_data[] = "shit";
	dummy_crc = crc32(dummy_crc, reinterpret_cast<const Bytef *>(test_data), 4);
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	if (inflateInit(&strm) == Z_OK)
		inflateEnd(&strm);
	(void)dummy_crc;
	zlib_initialized.store(true, std::memory_order_release);
}

struct pngErrorManager
{
	jmp_buf setjmp_buffer;
};

void pngErrorExit(png_structp png_ptr, png_const_charp error_msg)
{
	debugLog("PNG Error: {:s}\n", error_msg);
	auto *err = static_cast<pngErrorManager *>(png_get_error_ptr(png_ptr));
	longjmp(err->setjmp_buffer, 1);
}

void pngWarning(png_structp, png_const_charp warning_msg)
{
	debugLog("PNG Warning: {:s}\n", warning_msg);
}

struct pngMemoryReader
{
	const unsigned char *data;
	size_t size;
	size_t offset;
};

void pngReadFromMemory(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
{
	auto *reader = static_cast<pngMemoryReader *>(png_get_io_ptr(png_ptr));

	if (reader->offset + byteCountToRead > reader->size)
	{
		png_error(png_ptr, "Read past end of data");
		return;
	}

	memcpy(outBytes, reader->data + reader->offset, byteCountToRead);
	reader->offset += byteCountToRead;
}

bool Image::decodePNGFromMemory(const unsigned char *data, size_t size, std::vector<unsigned char> &outData, int &outWidth, int &outHeight, int &outChannels)
{
	garbage_zlib();
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		debugLog("Image Error: png_create_read_struct failed\n");
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		debugLog("Image Error: png_create_info_struct failed\n");
		return false;
	}

	pngErrorManager err;
	png_set_error_fn(png_ptr, &err, pngErrorExit, pngWarning);

	if (setjmp(err.setjmp_buffer))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}

	// Set up memory reading
	pngMemoryReader reader;
	reader.data = data;
	reader.size = size;
	reader.offset = 0;
	png_set_read_fn(png_ptr, &reader, pngReadFromMemory);

	png_read_info(png_ptr, info_ptr);

	outWidth = static_cast<int>(png_get_image_width(png_ptr, info_ptr));
	outHeight = static_cast<int>(png_get_image_height(png_ptr, info_ptr));
	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	// convert to RGBA if needed
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	// these color types don't have alpha channel, so fill it with 0xff
	if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	// after transformations, we should always have RGBA
	outChannels = 4;

	if (outWidth > 8192 || outHeight > 8192)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		debugLog("Image Error: PNG image size is too big ({} x {})\n", outWidth, outHeight);
		return false;
	}

	// allocate memory for the image
	outData.resize(static_cast<long>(outWidth * outHeight) * outChannels);

	// read it
	auto *row_pointers = new png_bytep[outHeight];
	for (int y = 0; y < outHeight; y++)
	{
		row_pointers[y] = &outData[static_cast<long>(y * outWidth * outChannels)];
	}

	png_read_image(png_ptr, row_pointers);
	delete[] row_pointers;

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return true;
}

void Image::saveToImage(unsigned char *data, unsigned int width, unsigned int height, UString filepath)
{
	garbage_zlib();
	debugLog("Saving image to {:s} ...\n", filepath);

	FILE *fp = fopen(filepath.toUtf8(), "wb");
	if (!fp)
	{
		debugLog("PNG error: Could not open file {:s} for writing\n", filepath);
		engine->showMessageError("PNG Error", "Could not open file for writing");
		return;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fclose(fp);
		debugLog("PNG error: png_create_write_struct failed\n");
		engine->showMessageError("PNG Error", "png_create_write_struct failed");
		return;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		debugLog("PNG error: png_create_info_struct failed\n");
		engine->showMessageError("PNG Error", "png_create_info_struct failed");
		return;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		debugLog("PNG error during write\n");
		engine->showMessageError("PNG Error", "Error during PNG write");
		return;
	}

	png_init_io(png_ptr, fp);

	// write header (8 bit colour depth, RGB)
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	// write image data
	auto row = new png_byte[3L * width];
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			row[x * 3 + 0] = data[(y * width + x) * 3 + 0];
			row[x * 3 + 1] = data[(y * width + x) * 3 + 1];
			row[x * 3 + 2] = data[(y * width + x) * 3 + 2];
		}
		png_write_row(png_ptr, row);
	}
	delete[] row;

	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
}

Image::Image(UString filepath, bool mipmapped, bool keepInSystemMemory) : Resource(filepath)
{
	m_bMipmapped = mipmapped;
	m_bKeepInSystemMemory = keepInSystemMemory;

	m_type = Image::TYPE::TYPE_PNG;
	m_filterMode = Graphics::FILTER_MODE::FILTER_MODE_LINEAR;
	m_wrapMode = Graphics::WRAP_MODE::WRAP_MODE_CLAMP;
	m_iNumChannels = 4;
	m_iWidth = 1;
	m_iHeight = 1;

	m_bHasAlphaChannel = true;
	m_bCreatedImage = false;
}

Image::Image(int width, int height, bool mipmapped, bool keepInSystemMemory) : Resource()
{
	m_bMipmapped = mipmapped;
	m_bKeepInSystemMemory = keepInSystemMemory;

	m_type = Image::TYPE::TYPE_RGBA;
	m_filterMode = Graphics::FILTER_MODE::FILTER_MODE_LINEAR;
	m_wrapMode = Graphics::WRAP_MODE::WRAP_MODE_CLAMP;
	m_iNumChannels = 4;
	m_iWidth = width;
	m_iHeight = height;

	m_bHasAlphaChannel = true;
	m_bCreatedImage = true;

	// reserve and fill with pink pixels
	m_rawImage.resize(static_cast<long>(m_iWidth * m_iHeight * m_iNumChannels));
	for (int i = 0; i < m_iWidth * m_iHeight; i++)
	{
		m_rawImage.push_back(255);
		m_rawImage.push_back(0);
		m_rawImage.push_back(255);
		m_rawImage.push_back(255);
	}

	// special case: filled rawimage is always already async ready
	m_bAsyncReady = true;
}

bool Image::loadRawImage()
{
	bool alreadyLoaded = m_rawImage.size() > 0;

	// if it isn't a created image (created within the engine), load it from the corresponding file
	if (!m_bCreatedImage)
	{
		if (alreadyLoaded) // has already been loaded (or loading it again after setPixel(s))
			return true;

		if (!env->fileExists(m_sFilePath))
		{
			debugLog("Image Error: Couldn't find file {:s}\n", m_sFilePath);
			return false;
		}

		if (m_bInterrupted) // cancellation point
			return false;

		// load entire file
		McFile file(m_sFilePath);
		if (!file.canRead())
		{
			debugLog("Image Error: Couldn't canRead() file {:s}\n", m_sFilePath);
			return false;
		}
		if (file.getFileSize() < 4)
		{
			debugLog("Image Error: FileSize is < 4 in file {:s}\n", m_sFilePath);
			return false;
		}

		if (m_bInterrupted) // cancellation point
			return false;

		const char *data = file.readFile();
		if (data == NULL)
		{
			debugLog("Image Error: Couldn't readFile() file {:s}\n", m_sFilePath);
			return false;
		}

		if (m_bInterrupted) // cancellation point
			return false;

		// determine file type by magic number (png/jpg)
		bool isJPEG = false;
		bool isPNG = false;
		{
			const int numBytes = 4;

			unsigned char buf[numBytes];

			for (int i = 0; i < numBytes; i++)
			{
				buf[i] = (unsigned char)data[i];
			}

			if (buf[0] == 0xff && buf[1] == 0xD8 && buf[2] == 0xff) // 0xFFD8FF
				isJPEG = true;
			else if (buf[0] == 0x89 && buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47) // 0x89504E47 (%PNG)
				isPNG = true;
		}

		// depending on the type, load either jpeg or png
		if (isJPEG)
		{
			m_type = Image::TYPE::TYPE_JPG;

			// decode jpeg
			tjhandle tjInstance = tj3Init(TJINIT_DECOMPRESS);
			if (!tjInstance)
			{
				debugLog("Image Error: tj3Init failed in file {:s}\n", m_sFilePath);
				return false;
			}

			if (tj3DecompressHeader(tjInstance, (unsigned char *)data, file.getFileSize()) < 0)
			{
				debugLog("Image Error: tj3DecompressHeader failed: {:s} in file {:s}\n", tj3GetErrorStr(tjInstance), m_sFilePath);
				tj3Destroy(tjInstance);
				return false;
			}

			if (m_bInterrupted) // cancellation point
			{
				tj3Destroy(tjInstance);
				return false;
			}

			m_iWidth = tj3Get(tjInstance, TJPARAM_JPEGWIDTH);
			m_iHeight = tj3Get(tjInstance, TJPARAM_JPEGHEIGHT);
			m_iNumChannels = 4; // always convert to RGBA for consistency with PNG
			m_bHasAlphaChannel = true;

			if (m_iWidth > 8192 || m_iHeight > 8192)
			{
				debugLog("Image Error: JPEG image size is too big ({} x {}) in file {:s}\n", m_iWidth, m_iHeight, m_sFilePath);
				tj3Destroy(tjInstance);
				return false;
			}

			if (m_bInterrupted) // cancellation point
			{
				tj3Destroy(tjInstance);
				return false;
			}

			// preallocate
			m_rawImage.resize(static_cast<long>(m_iWidth * m_iHeight * m_iNumChannels));

			// decompress directly to RGBA
			if (tj3Decompress8(tjInstance, (unsigned char *)data, file.getFileSize(), &m_rawImage[0], 0, TJPF_RGBA) < 0)
			{
				debugLog("Image Error: tj3Decompress8 failed: {:s} in file {:s}\n", tj3GetErrorStr(tjInstance), m_sFilePath);
				tj3Destroy(tjInstance);
				return false;
			}

			tj3Destroy(tjInstance);
		}
		else if (isPNG)
		{
			m_type = Image::TYPE::TYPE_PNG;

			// decode png using libpng
			if (!decodePNGFromMemory((const unsigned char *)data, file.getFileSize(), m_rawImage, m_iWidth, m_iHeight, m_iNumChannels))
			{
				debugLog("Image Error: PNG decoding failed in file {:s}\n", m_sFilePath);
				return false;
			}

			m_bHasAlphaChannel = true;
		}
		else
		{
			debugLog("Image Error: Neither PNG nor JPEG in file {:s}\n", m_sFilePath);
			return false;
		}
	}

	if (m_bInterrupted) // cancellation point
		return false;

	// error checking

	// size sanity check
	if (m_rawImage.size() < static_cast<long>(m_iWidth * m_iHeight * m_iNumChannels))
	{
		debugLog("Image Error: Loaded image has only {}/{} bytes in file {:s}\n", (unsigned long)m_rawImage.size(), m_iWidth * m_iHeight * m_iNumChannels,
		         m_sFilePath);
		// engine->showMessageError("Image Error", UString::format("Loaded image has only %i/%i bytes in file %s", m_rawImage.size(),
		// m_iWidth*m_iHeight*m_iNumChannels, m_sFilePath));
		return false;
	}

	// supported channels sanity check
	if (m_iNumChannels != 4 && m_iNumChannels != 3 && m_iNumChannels != 1)
	{
		debugLog("Image Error: Unsupported number of color channels ({}) in file {:s}\n", m_iNumChannels, m_sFilePath);
		// engine->showMessageError("Image Error", UString::format("Unsupported number of color channels (%i) in file %s", m_iNumChannels, m_sFilePath));
		return false;
	}

	// optimization: ignore completely transparent images (don't render) (only PNGs can have them, obviously)
	if (!alreadyLoaded && (m_type == Image::TYPE::TYPE_PNG) && canHaveTransparency(m_rawImage.data(), m_rawImage.size()) && isCompletelyTransparent())
	{
		if (!m_bInterrupted)
			debugLog("Image: Ignoring empty transparent image {:s}\n", m_sFilePath);
		return false;
	}

	return true;
}

void Image::setFilterMode(Graphics::FILTER_MODE filterMode)
{
	m_filterMode = filterMode;
}

void Image::setWrapMode(Graphics::WRAP_MODE wrapMode)
{
	m_wrapMode = wrapMode;
}

Color Image::getPixel(int x, int y) const
{
	const int indexBegin = m_iNumChannels * y * m_iWidth + m_iNumChannels * x;
	const int indexEnd = m_iNumChannels * y * m_iWidth + m_iNumChannels * x + m_iNumChannels;

	if (m_rawImage.size() < 1 || x < 0 || y < 0 || indexEnd < 0 || indexEnd > m_rawImage.size())
		return 0xffffff00;

	unsigned char r = 255;
	unsigned char g = 255;
	unsigned char b = 0;
	unsigned char a = 255;

	r = m_rawImage[indexBegin + 0];
	if (m_iNumChannels > 1)
	{
		g = m_rawImage[indexBegin + 1];
		b = m_rawImage[indexBegin + 2];

		if (m_iNumChannels > 3)
			a = m_rawImage[indexBegin + 3];
		else
			a = 255;
	}
	else
	{
		g = r;
		b = r;
		a = r;
	}

	return argb(a, r, g, b);
}

void Image::setPixel(int x, int y, Color color)
{
	const int indexBegin = m_iNumChannels * y * m_iWidth + m_iNumChannels * x;
	const int indexEnd = m_iNumChannels * y * m_iWidth + m_iNumChannels * x + m_iNumChannels;

	if (m_rawImage.size() < 1 || x < 0 || y < 0 || indexEnd < 0 || indexEnd > m_rawImage.size())
		return;

	m_rawImage[indexBegin + 0] = color.R();
	if (m_iNumChannels > 1)
		m_rawImage[indexBegin + 1] = color.G();
	if (m_iNumChannels > 2)
		m_rawImage[indexBegin + 2] = color.B();
	if (m_iNumChannels > 3)
		m_rawImage[indexBegin + 3] = color.A();
}

void Image::setPixels(const char *data, size_t size, TYPE type)
{
	if (data == NULL)
		return;

	// TODO: implement remaining types
	switch (type)
	{
	case TYPE::TYPE_PNG: {
		if (!decodePNGFromMemory((const unsigned char *)data, size, m_rawImage, m_iWidth, m_iHeight, m_iNumChannels))
		{
			debugLog("Image Error: PNG decoding failed in setPixels\n");
		}
	}
	break;

	default:
		debugLog("Image Error: Format not yet implemented\n");
		break;
	}
}

void Image::setPixels(const std::vector<unsigned char> &pixels)
{
	if (pixels.size() < static_cast<long>(m_iWidth * m_iHeight * m_iNumChannels))
	{
		debugLog("Image Error: setPixels() supplied array is too small!\n");
		return;
	}

	m_rawImage = pixels;
}

// internal
bool Image::canHaveTransparency(const unsigned char *data, size_t size)
{
	if (size < 33) // not enough data for IHDR, so just assume true
		return true;

	// PNG IHDR chunk starts at offset 16 (8 bytes signature + 8 bytes chunk header)
	// color type is at offset 25 (16 + 4 width + 4 height + 1 bit depth)
	if (size > 25)
	{
		unsigned char colorType = data[25];
		return colorType != 2; // RGB without alpha
	}

	return true; // unknown format? just assume true
}

bool Image::isCompletelyTransparent() const
{
	if (m_rawImage.empty() || m_iNumChannels < 4 || !m_bHasAlphaChannel)
		return false;

	const size_t alphaOffset = 3;
	const size_t stride = m_iNumChannels;
	const size_t totalPixels = m_iWidth * m_iHeight;

	for (size_t i = 0; i < totalPixels; ++i)
	{
		if (m_bInterrupted) // cancellation point
			return false;

		// check alpha channel directly
		if (m_rawImage[i * stride + alphaOffset] > 0)
			return false; // non-transparent pixel
	}

	return true; // all pixels are transparent
}
