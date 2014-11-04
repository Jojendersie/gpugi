/**************************************************************************//**
 * \file	jofilelib.hpp
 * \brief	A file wrapper for json and structured-raw files.
 * \details This engine provides an easy interface to work with files.
 *			Additional to some basic file methods it provides the following
 *			formats:
 *				* json, sraw via MetaFileWrapper
 *				* png, tga, pfm via ImageFileWrapper
 *
 *			Include this header only and add JoFile[D/R].lib to the linker.
 * \author	Johannes Jendersie
 * \date	2013/09
 *****************************************************************************/
#pragma once

namespace Jo {
namespace Files {
	enum struct Format {
		AUTO_DETECT,
		JSON,
		SRAW,
		PNG,
		PFM,		///< Portable float map
		TGA
	};
} // namespace Files
} // namespace Jo

#include "file.hpp"
#include "memfile.hpp"
#include "hddfile.hpp"
#include "filewrapper.hpp"
#include "imagewrapper.hpp"
#include "fileutils.hpp"
#include "streamreader.hpp"