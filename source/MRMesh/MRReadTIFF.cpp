#if !defined( __EMSCRIPTEN__ ) && !defined( MRMESH_NO_TIFF )
#include "MRReadTIFF.h"
#include "MRStringConvert.h"
#include "MRBuffer.h"

#include <tiffio.h>

namespace MR
{

template<typename SampleType>
void setDataValue( float* data, const SampleType* input, TiffParameters::ValueType type,
    float* min, float* max )
{
    float res = 0;

    switch ( type )
    {
    case MR::TiffParameters::ValueType::Scalar:
        res = float( *input );
        break;
    case MR::TiffParameters::ValueType::RGB:
    case MR::TiffParameters::ValueType::RGBA:
        res =
            ( 0.299f * float( input[0] ) +
                0.587f * float( input[1] ) +
                0.114f * float( input[2] ) );
        break;
    case MR::TiffParameters::ValueType::Unknown:
    default:
        break;
    }

    if ( min && res < *min )
        *min = res;
    if ( max && res > *max )
        *max = res;

    *data = res;
}

template<typename SampleType>
void readRawTiff( TIFF* tiff, float* data, size_t size, const TiffParameters& tp, float* min = nullptr, float* max = nullptr )
{
    assert( sizeof( SampleType ) == tp.bytesPerSample );
    int samplePerPixel = 0;
    if ( tp.valueType == TiffParameters::ValueType::Scalar )
        samplePerPixel = 1;
    else if ( tp.valueType == TiffParameters::ValueType::RGB )
        samplePerPixel = 3;
    else if ( tp.valueType == TiffParameters::ValueType::RGBA )
        samplePerPixel = 4;
    assert( samplePerPixel != 0 );

    Buffer<SampleType> buffer( tp.tiled ? 
        ( size_t( tp.tileSize.x ) * tp.tileSize.y * samplePerPixel ) :
        ( size_t( tp.imageSize.x ) * samplePerPixel ) );

    if ( tp.tiled )
    {
        for ( int y = 0; y < tp.imageSize.y; y += tp.tileSize.y )
        {
            for ( int x = 0; x < tp.imageSize.x; x += tp.tileSize.x )
            {
                TIFFReadTile( tiff, ( void* )( buffer.data() ), x, y, 0, 0 );
                for ( int y0 = y; y0 < std::min( y + tp.tileSize.y, tp.imageSize.y ); ++y0 )
                {
                    for ( int x0 = x; x0 < std::min( x + tp.tileSize.x, tp.imageSize.x ); ++x0 )
                    {
                        size_t dataShift = tp.imageSize.x * y0 + x0;
                        if ( dataShift >= size )
                            continue;
                        setDataValue( data + dataShift,
                            buffer.data() + samplePerPixel * ( tp.tileSize.x * ( y0 - y ) + ( x0 - x ) ), tp.valueType,
                            min, max );
                    }
                }
            }
        }
    }
    else
    {
        for ( uint32_t i = 0; i < uint32_t( tp.imageSize.y ); ++i )
        {
            TIFFReadScanline( tiff, ( void* )( buffer.data() ), i );
            auto shift = i * tp.imageSize.x;
            for ( int j = 0; j < tp.imageSize.x; ++j )
            {
                size_t dataShift = shift + j;
                if ( dataShift >= size )
                    continue;
                setDataValue( data + dataShift, buffer.data() + samplePerPixel * j, tp.valueType,
                    min, max );
            }
        }
    }
}

class TiffHolder 
{
public:
    TiffHolder( const std::filesystem::path& path, const char* mode )
    {
#ifdef __WIN32__
        tiffPtr_ = TIFFOpenW( path.wstring().c_str(), mode );
#else
        tiffPtr_ = TIFFOpen( utf8string( path ).c_str(), mode );
#endif
    }
    ~TiffHolder()
    {
        if ( !tiffPtr_ )
            return;
        TIFFClose( tiffPtr_ );
        tiffPtr_ = nullptr;
    }
    operator TIFF* ( ) { return tiffPtr_; }
    operator const TIFF* ( ) const { return tiffPtr_; }
    operator bool() const { return bool( tiffPtr_ ); }
private:
    TIFF* tiffPtr_{ nullptr };
};

bool isTIFFFile( const std::filesystem::path& path )
{
    TiffHolder tiff( path, "rh" );// only header
    return bool( tiff );
}

Expected<TiffParameters, std::string> readTifParameters( TIFF* tiff )
{
    TiffParameters params;

    int bitsPerSample = 0;
    TIFFGetField( tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample );
    params.bytesPerSample = bitsPerSample >> 3; // convert to bytes

    int samplePerPixel = 0;
    TIFFGetField( tiff, TIFFTAG_SAMPLESPERPIXEL, &samplePerPixel );
    if ( samplePerPixel == 1 )
        params.valueType = TiffParameters::ValueType::Scalar;
    else if ( samplePerPixel == 3 )
        params.valueType = TiffParameters::ValueType::RGB;
    else if ( samplePerPixel == 4 )
        params.valueType = TiffParameters::ValueType::RGBA;

    int sampleFormat = 0;
    TIFFGetField( tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat );
    if ( sampleFormat == SAMPLEFORMAT_UINT || sampleFormat == 0 )
        params.sampleType = TiffParameters::SampleType::Uint;
    else if ( sampleFormat == SAMPLEFORMAT_INT )
        params.sampleType = TiffParameters::SampleType::Int;
    else if ( sampleFormat == SAMPLEFORMAT_IEEEFP )
        params.sampleType = TiffParameters::SampleType::Float;

    TIFFGetField( tiff, TIFFTAG_IMAGEWIDTH, &params.imageSize.x );
    TIFFGetField( tiff, TIFFTAG_IMAGELENGTH, &params.imageSize.y );

    params.tiled = bool( TIFFIsTiled( tiff ) );
    if ( params.tiled )
    {
        TIFFGetField( tiff, TIFFTAG_TILEWIDTH, &params.tileSize.x );
        TIFFGetField( tiff, TIFFTAG_TILELENGTH, &params.tileSize.y );

        TIFFGetField( tiff, TIFFTAG_TILEDEPTH, &params.depth );
        if ( params.depth != 0 )
            params.layers = int( TIFFNumberOfTiles( tiff ) );
    }

    if ( params.valueType == TiffParameters::ValueType::Unknown ||
        params.sampleType == TiffParameters::SampleType::Unknown )
        return unexpected( "Unsupported pixel format" );

    if ( params.depth != 0 )
        return unexpected( "Unsupported tiles format" );

    return params;
}

Expected<TiffParameters, std::string> readTiffParameters( const std::filesystem::path& path )
{
    TiffHolder tif( path, "r" );
    if ( !tif )
        return unexpected( "Cannot read file: " + utf8string( path ) );

    return addFileNameInError( readTifParameters( tif ), path );
}

VoidOrErrStr readRawTiff( const std::filesystem::path& path, RawTiffOutput& output )
{
    assert( output.size != 0 );
    if ( output.size == 0 )
        return unexpected( "Cannot read file to empty buffer" );
    TiffHolder tiff( path, "r" );
    if ( !tiff )
        return unexpected( "Cannot read file: " + utf8string( path ) );
    auto localParams = readTifParameters( tiff );
    if ( !localParams.has_value() )
        return unexpected( localParams.error() + ": " + utf8string( path ) );
    if ( output.params )
        *output.params = *localParams;

    if ( localParams->sampleType == TiffParameters::SampleType::Uint )
    {
        if ( localParams->bytesPerSample == sizeof( uint8_t ) )
            readRawTiff<uint8_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
        else if ( localParams->bytesPerSample == sizeof( uint16_t ) )
            readRawTiff<uint16_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
        else if ( localParams->bytesPerSample == sizeof( uint32_t ) )
            readRawTiff<uint32_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
        else if ( localParams->bytesPerSample == sizeof( uint64_t ) )
            readRawTiff<uint64_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
    }
    else if ( localParams->sampleType == TiffParameters::SampleType::Int )
    {
        if ( localParams->bytesPerSample == sizeof( int8_t ) )
            readRawTiff<int8_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
        else if ( localParams->bytesPerSample == sizeof( int16_t ) )
            readRawTiff<int16_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
        else if ( localParams->bytesPerSample == sizeof( int32_t ) )
            readRawTiff<int32_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
        else if ( localParams->bytesPerSample == sizeof( int64_t ) )
            readRawTiff<int64_t>( tiff, output.data, output.size, *localParams, output.min, output.max );
    }
    else if ( localParams->sampleType == TiffParameters::SampleType::Float )
    {
        if ( localParams->bytesPerSample == sizeof( float ) )
            readRawTiff<float>( tiff, output.data, output.size, *localParams, output.min, output.max );
        else if ( localParams->bytesPerSample == sizeof( double ) )
            readRawTiff<double>( tiff, output.data, output.size, *localParams, output.min, output.max );
    }
    return {};
}

}

#endif