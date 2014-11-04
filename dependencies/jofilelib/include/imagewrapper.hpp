#pragma once

#include <cstdint>

namespace Jo {
namespace Files {

	enum struct Format;
	class IFile;

	/**************************************************************************//**
	 * \class	Jo::Files::ImageWrapper
	 * \brief	Loads images of various types and provides an unified access.
	 *
	 *			The pixel coordinates are mathematically: (0,0) is the lower left
	 *			corner???? TODO: really (true for tga, the others ?)
	 * \details	In general color palettes are not supported!
	 *****************************************************************************/
	class ImageWrapper
	{
	public:
		enum struct ChannelType {
			UINT,	///< Any unsigned int with 1,2,4,8,16 or 32 bit (depends on bit depth)
			INT,	///< Any signed int with 8, 16 or 32 bit (depends on bit depth)
			FLOAT	///< IEEE754 32bit float
		};

		/// \brief Use a wrapped file to read from.
		/// \details This is only to read images call write to save an image.
		/// \param _format [in] How should the input be interpreted.
		ImageWrapper( const IFile& _file, Format _format = Format(0) );

		/// \brief Create an empty image. The pixels are uninitialized and have
		///		an unknown value.
		/// \param [in] _width Number of pixels in x direction.
		/// \param [in] _height Number of pixels in y direction.
		/// \param [in] _numChannels Number of color channels in [1,4].
		///		(E.g. use 3 for RGB).
		/// \param [in] _type Type of a color channel. The type restricts the
		///		_bitDepth. See \ref{ChannelType}. TODO: Doxygentest
		/// \param [in] _bitDepth Number of bits per channel common is 8 or 16.
		///		1,2,4 and 32 can also be used but make sure your target
		///		file-format supports that bit depth
		ImageWrapper( uint32_t _width, uint32_t _height, uint32_t _numChannels, ChannelType _type = ChannelType::UINT, int _bitDepth=8 );

		~ImageWrapper();

		/// \brief Return the width of the image in memory in pixels.
		uint32_t Width() const		{ return m_width; }

		/// \brief Return the height of the image in memory in pixels.
		uint32_t Height() const		{ return m_height; }

		/// \brief Return the number of different channels (e.g. RGB -> 3).
		uint32_t NumChannels() const	{ return m_numChannels; }

		/// \brief Return the type of all channels (they all have the same)
		ChannelType GetChannelType() const	{ return m_channelType; }

		/// \brief Return number of bits per channel.
		int BitDepth() const 				{ return m_bitDepth; }

		/// \brief Direct access to the image memory.
		/// \details This should be used for uploading images to the GPU.
		///
		///		The pixels are stored row wise.
		const void* GetBuffer() const		{ return m_buffer; }
		
		/// \brief Set one channel of a pixel.
		/// \details If the format is not float the value will be converted.
		///		Therefore the value is clamped to [0,1] or [-1,1]. It is not
		///		clamped if the channel contains floats!
		///		
		///		If the out of bound check fails for x, y or c nothing is done.
		void Set( int _x, int _y, int _c, float _value );

		/// \brief Get one channel of a pixel.
		/// \details If the format is not float the value will be converted.
		/// 
		///		If x, y, or c is out of bounds the return value is -10000.0f.
		/// \return Value in [0,1] if unsigned channels, [-1,1] for signed int
		///		or unconverted float value.
		float Get( int _x, int _y, int _c ) const;

		/// \brief Write image to a file of arbitrary type.
		/// \details This method fails if the target format does not support
		///		the current channels + bit depth
		void Write( IFile& _file, Format _format ) const;

	private:
		uint8_t* m_buffer;		///< RAW buffer of all pixels (row major)
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_numChannels;
		int m_bitDepth;
		ChannelType m_channelType;
		//int m_pixelSize;		///< Size of a pixel in bytes (= m_numChannels * m_bitDepth / 8)

		bool IsPNG( const IFile& _file );
		void ReadPNG( const IFile& _file );
		void WritePNG( IFile& _file ) const;

		bool IsPFM( const IFile& _file );
		/// \brief Read in a portable float map
		void ReadPFM( const IFile& _file );
		/// \brief Write a portable float map
		void WritePFM( IFile& _file ) const;

		bool IsTGA( const IFile& _file );
		/// \brief Read in a targa
		void ReadTGA( const IFile& _file );
		/// \brief Write a targa
		void WriteTGA( IFile& _file ) const;
	};

} // namespace Files
} // namespace Jo