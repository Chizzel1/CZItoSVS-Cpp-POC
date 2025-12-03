#include <tiffio.h>
#include <cstdint>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include "stb_image.h"
#include <vector>
#include <tiff.h>
#include <algorithm>
#include <ios>
#include <libCZI.h>
#include <iostream>
#include <fstream>
#include <stdexcept>


static void write_tiff_tiles_helper(
    TIFF* tif,
    const std::vector<unsigned char>& pixels,
    std::uint32_t width,
    std::uint32_t height
)
{
    const int sample_per_pixels = 3;
    std::uint32_t tileW = 0, tileH = 0;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileW);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileH);

    std::vector<std::uint8_t> tileBuf(tileW * tileH * sample_per_pixels);

    for (std::uint32_t ty = 0; ty < height; ty += tileH) {
        for (std::uint32_t tx = 0; tx < width; tx += tileW) {

            std::uint32_t xMax = std::min(tx + tileW, width);
            std::uint32_t yMax = std::min(ty + tileH, height);

            std::fill(tileBuf.begin(), tileBuf.end(), 0);

            for (std::uint32_t y = ty; y < yMax; ++y) {
                const std::uint8_t* src = &pixels[(std::size_t(y) * width + tx) * sample_per_pixels];
                std::uint8_t* dst = &tileBuf[(std::size_t(y - ty) * tileW) * sample_per_pixels];
                std::memcpy(dst, src, (xMax - tx) * sample_per_pixels);
            }

 //           tsize_t nBytes = tsize_t(yMax - ty) * tileW * sample_per_pixels;
            tsize_t nBytes = tsize_t(tileW) * tileH * sample_per_pixels;
            ttile_t tile = TIFFComputeTile(tif, tx, ty, 0, 0);
            TIFFWriteEncodedTile(tif, tile, tileBuf.data(), nBytes);
        }
    }
}


static void write_tiff_strips_helper(
    TIFF* tif,
    const std::vector<unsigned char>& pixels,
    std::uint32_t width,
    std::uint32_t height
)
{
    const int SPP = 3;
    std::uint32_t rowsPerStrip = 0;
    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);

    for (std::uint32_t y = 0; y < height; y += rowsPerStrip) {
        std::uint32_t nRows = std::min(rowsPerStrip, height - y);
        tsize_t nBytes = tsize_t(nRows) * width * SPP;
        const std::uint8_t* src = &pixels[(std::size_t)y * width * SPP];

        tstrip_t strip = TIFFComputeStrip(tif, y, 0);
        TIFFWriteEncodedStrip(tif, strip, (void*)src, nBytes);
    }
}

std::vector<uint8_t> CziBitmapToBuffer(
    const std::shared_ptr<libCZI::IBitmapData>& bmp,
    int* outW = nullptr,
    int* outH = nullptr,
    int* outBpp = nullptr

)
{
    libCZI::ScopedBitmapLockerSP lock(bmp);
    const int w = bmp->GetWidth();
    const int h = bmp->GetHeight();
    const int stride = lock.stride;
    int bpp = 3;
    std::vector<uint8_t> buffer(size_t(w) * h * bpp);
    for (int y = 0; y < h; y++) {
        const uint8_t* src =
            static_cast<const uint8_t*>(lock.ptrDataRoi) + size_t(y) * stride;

        uint8_t* dst =
            buffer.data() + size_t(y) * w * bpp;

        std::memcpy(dst, src, size_t(w) * bpp);
    }
    if (outW)  *outW = w;
    if (outH)  *outH = h;
    if (outBpp)*outBpp = bpp;

    return buffer;
}

static void load_from_bitmap(const std::string& path,
    std::vector<unsigned char>& pixels,
    int& width,
    int& height)
{
    int channels_in_file = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height,
        &channels_in_file,
        3);
    pixels.assign(data, data + width * height * 3);
    stbi_image_free(data);
    
}

void write_base_ifd (TIFF* tif, std::vector<unsigned char>& pixels, int width, int height, std::string desc)
{
	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(tif, TIFFTAG_IMAGEDEPTH, 1);

	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
	TIFFSetField(tif, TIFFTAG_JPEGQUALITY, 75);
	TIFFSetField(tif, TIFFTAG_TILEWIDTH, 512);
	TIFFSetField(tif, TIFFTAG_TILELENGTH, 512);
	TIFFSetField(tif, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, desc.c_str());
    write_tiff_tiles_helper(tif, pixels, width, height);
    TIFFWriteDirectory(tif);

}

void write_thumbnail_ifd(
    TIFF* tif, std::vector<unsigned char>& pixels, int width, int height,
    const std::string& desc
) {
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_IMAGEDEPTH, 1);

    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
    TIFFSetField(tif, TIFFTAG_JPEGQUALITY, 75);

   
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 16);

    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, desc.c_str());

    write_tiff_strips_helper(tif, pixels, width, height);

    TIFFWriteDirectory(tif); 
}

void write_pyramid_ifd(
    TIFF* tif, std::vector<unsigned char>& pixels,
    int width, int height,
    const std::string& desc
) {
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_IMAGEDEPTH, 1);

    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
    TIFFSetField(tif, TIFFTAG_JPEGQUALITY, 75);
    TIFFSetField(tif, TIFFTAG_TILEWIDTH, 512);
    TIFFSetField(tif, TIFFTAG_TILELENGTH, 512);

    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, desc.c_str());

    write_tiff_tiles_helper(tif, pixels, width, height);

    TIFFWriteDirectory(tif);
}


void write_label_ifd(
    TIFF* tif, std::vector<unsigned char>& pixels,
    uint32 width, uint32 height,
    const std::string& desc
) {
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_IMAGEDEPTH, 1);

    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(tif, TIFFTAG_PREDICTOR, 2);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 16);
    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, 1);
    TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, desc.c_str());

    write_tiff_strips_helper(tif, pixels, width, height);

    TIFFWriteDirectory(tif);

}
void write_macro_ifd(
        TIFF * tif, std::vector<unsigned char>& pixels,
        uint32 width, uint32 height,
        const std::string & desc) {
        
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_IMAGEDEPTH, 1);

        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, 2);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 16);

        TIFFSetField(tif, TIFFTAG_SUBFILETYPE, 9);   
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, desc.c_str());

        write_tiff_strips_helper(tif, pixels, width, height);

        TIFFWriteDirectory(tif);
    }



namespace description_generators
    {
        std::uint32_t encode_bg(double r, double g, double b)
        {
            int R = static_cast<int>(std::lround(r * 255.0));
            int G = static_cast<int>(std::lround(g * 255.0));
            int B = static_cast<int>(std::lround(b * 255.0));
            return (static_cast<std::uint32_t>(R) << 16) |
                (static_cast<std::uint32_t>(G) << 8) |
                static_cast<std::uint32_t>(B);
        }

        static void pad_equals(std::string& s, std::size_t pad_length)
        {
            if (s.size() < pad_length)
                s.append(pad_length - s.size(), '=');
        }

        static std::string make_aperio_description_IFD0(

            int full_w,
            int full_h,
            int tile_w,
            int tile_h,
            int quality,
            int appmag,
            double mpp,
            double bg_r, double bg_g, double bg_b,
            const std::string& barcode,
            std::size_t pad_length = 2048
        )
        {
            std::uint32_t bg = encode_bg(bg_r, bg_g, bg_b);

            std::ostringstream oss;
            oss << "Aperio Image Library v12.0.0.1" << "\n\n"
                << full_w << 'x' << full_h << " "
                << "[0,0 " << full_w << 'x' << full_h << "] "
                << "[" << tile_w << 'x' << tile_h << "] "
                << "JPEG/RGB "
                << "Q = " << quality << '|'
                << "AppMag = " << appmag << '|'
                << "MPP = " << mpp << '|'
                << "BackgroundColor = " << bg << '|'
                << "Barcode = " << barcode << '|';

            std::string desc = oss.str();
            pad_equals(desc, pad_length);
            return desc;
        }

        std::string make_aperio_description_thumbnail(

            int full_w,
            int full_h,
            int thumb_w,
            int thumb_h,
            int quality,
            int appmag,
            double mpp,
            double bg_r, double bg_g, double bg_b,
            const std::string& barcode,
            std::size_t pad_length = 2048
        )
        {
            std::uint32_t bg = encode_bg(bg_r, bg_g, bg_b);

            std::ostringstream oss;
            oss << "Aperio Image Library v12.0.0.1" << "\n\n"
                << full_w << 'x' << full_h << " -> "
                << thumb_w << 'x' << thumb_h << " "
                << "Q = " << quality << '|'
                << "AppMag = " << appmag << '|'
                << "MPP = " << std::fixed << std::setprecision(6) << mpp << '|'
                << "BackgroundColor = " << bg << '|'
                << "Barcode = " << barcode << '|';

            std::string desc = oss.str();
            pad_equals(desc, pad_length);
            return desc;
        }

        std::string make_aperio_description_overview(
            int full_w,
            int full_h,
            int tile_w,
            int tile_h,
            int ov_w,
            int ov_h,
            int quality,
            int appmag,
            double mpp,
            double bg_r, double bg_g, double bg_b,
            const std::string& barcode,
            std::size_t pad_length = 2048
        )
        {
            std::uint32_t bg = encode_bg(bg_r, bg_g, bg_b);

            std::ostringstream oss;
            oss << "Aperio Image Library v12.0.0.1" << "\n\n"
                << full_w << 'x' << full_h << " "
                << "[0,0 " << full_w << 'x' << full_h << "] "
                << "[" << tile_w << 'x' << tile_h << "] -> "
                << ov_w << 'x' << ov_h << " "
                << "JPEG/RGB "
                << "Q = " << quality << '|'
                << "AppMag = " << appmag << '|'
                << "MPP = " << std::fixed << std::setprecision(6) << mpp << '|'
                << "BackgroundColor = " << bg << '|'
                << "Barcode = " << barcode << '|';

            std::string desc = oss.str();
            pad_equals(desc, pad_length);
            return desc;
        }

        std::string make_aperio_description_label(
            int label_w,
            int label_h,
            std::size_t pad_length = 2048
        )
        {
            std::ostringstream oss;
            oss << "Aperio Image Library v12.0.0.1" << "\n\n\n"
                << "label " << label_w << 'x' << label_h << '|';

            std::string desc = oss.str();
            pad_equals(desc, pad_length);
            return desc;
        }

        std::string make_aperio_description_macro(

            int macro_w,
            int macro_h,
            std::size_t pad_length = 2048
        )
        {
            std::ostringstream oss;
            oss << "Aperio Image Library v12.0.0.1" << "\n\n"
                << "macro " << macro_w << 'x' << macro_h << '|';

            std::string desc = oss.str();
            pad_equals(desc, pad_length);
            return desc;
        }
    }


int attachment_index(std::shared_ptr<libCZI::ICZIReader> reader, const char* name)
{
    int idx = -1;

    reader->EnumerateSubset(
        nullptr,
        name,
        [&](int i, const libCZI::AttachmentInfo&) {
            idx = i;
            return false;
        }
    );
    return idx;
}


int main() 
{
    // Proof of concept showing SVS files can be written with libtiff - compression is done internally by libtiff however i have tested with GPU compressed tiles at some point and that also works.
	// Shows:
    // Reading CZI Files
    // reading regions within the base image
	// Reading scaled versions of the base image (direct access to CZI pyramids is also possible with the pyramid accessor)
    // extracting usable XML metadata,
	// finding and reading label and macro attachments,
	// Shows it is possible to match Aperio SVS structure using libtiff (c++ Proof of concept is not as complete as the python version although i am sure it is possible)

    TIFF* tif = TIFFOpen("C:\\Projects\\Test files\\output.svs", "w8");
    wchar_t* czifile = LR"(C:\Users\lewpi\Downloads\591797_H383248_25-2647_1.czi)";
    
    int ROI_limit_w = 25000, ROI_limit_h = 25000;
    int thumbnail_w = 0, thumbnail_h = 0;
    int ov_w = 0, ov_h;
	int base_w = 0, base_h = 0;
    
	int ov1_w = 0, ov1_h = 0;
	int ov2_w = 0, ov2_h = 0;
	int ov3_w = 0, ov3_h = 0;
    int label_w = 0, label_h = 0;
    int macro_w = 0, macro_h = 0;
    int tile_size = 512;
    int appmag = 40;
    int quality = 75;
    int br = 1, bb = 1, bg = 1;
    float zoom = 1.0f;

    double mpp = 0.174;

    std::string barcode = "BarcodeExample";
    std::string ov_desc;
    std::vector<unsigned char> pixels;

    libCZI::CDimCoordinate planeCoord{ { libCZI::DimensionIndex::C,0 } };

    // Set up main reader stream 
    std::shared_ptr<libCZI::IStream> stream =
        libCZI::CreateStreamFromFile(czifile);
    std::shared_ptr<libCZI::ICZIReader> mainreader =
        libCZI::CreateCZIReader();
    mainreader->Open(stream);
    auto mainstats = mainreader->GetStatistics();
    auto mainbbox = mainstats.boundingBox;
    std::cout << "Main image dims:" << " X: " << mainbbox.x << " Y: " << mainbbox.y << " W: " << mainbbox.w << " H: " << mainbbox.h << "\n";
    mainbbox.w = std::min(mainbbox.w, ROI_limit_w);
    mainbbox.h = std::min(mainbbox.h, ROI_limit_h);
    std::cout << "Main image dims (Cropped):" << " X: " << mainbbox.x << " Y: " << mainbbox.y << " W: " << mainbbox.w << " H: " << mainbbox.h << "\n";
	
    // metadata is stored structured like scaling-mPP/name,Channel indexes via a number of diffrent interfaces,   
    // however you can also produce a XML metadata dump, this is how the python program find MPP, appmag, barcode etc.
    auto metadatasegment = mainreader->ReadMetadataSegment();
	auto metadataobj = metadatasegment->CreateMetaFromMetadataSegment();
    auto metastructured = metadataobj->GetDocumentInfo();
    
    //Structured metadata examples:
    auto scaling = metastructured->GetScalingInfo();
    std::cout << "scaling microns per pixel X: " << scaling.scaleX * 1.0e6 << "\n";
    auto channelmeta = metastructured->GetDimensionChannelsInfo();
    std::cout << "Number of channels:" << channelmeta->GetChannelCount();
    
    //XML dump: 
    std::string xml = metadataobj->GetXml();
    std::cout << xml;
	
    //  This finds the index within the attachment portion for the label attachment and sets up a reader, Theese are nested CZI within the main CZI file.
    int labelIndex = attachment_index(mainreader, "Label");
    std::shared_ptr labelAttachment = mainreader->ReadAttachment(labelIndex);
	std::shared_ptr labelstream = libCZI::CreateStreamFromMemory(labelAttachment.get());
	std::shared_ptr labelReader = libCZI::CreateCZIReader();
	labelReader->Open(labelstream);
	auto labelstats = labelReader->GetStatistics();
    auto labelbbox = labelstats.boundingBox;
    std::cout << "Label image dims:" << " X: " << labelbbox.x << " Y: " << labelbbox.y << " W: " << labelbbox.w << " H: " << labelbbox.h << "\n";


	// Same but for the macro image (CZI calls it "SlidePreview")
    int macroIndex = attachment_index(mainreader, "SlidePreview");
    std::shared_ptr macroAttachment = mainreader->ReadAttachment(macroIndex);
    std::shared_ptr macrostream = libCZI::CreateStreamFromMemory(macroAttachment.get());
    std::shared_ptr macroReader = libCZI::CreateCZIReader();
    macroReader->Open(macrostream);
    auto macrostats = macroReader->GetStatistics();
    auto macrobbox = macrostats.boundingBox;
    std::cout << "macro image dims:" << " X: " << macrobbox.x << " Y: " << macrobbox.y << " W: " << macrobbox.w << " H: " << macrobbox.h << "\n";

	auto mainimageAccessor = mainreader->CreateSingleChannelScalingTileAccessor();
    auto mainbitmap = mainimageAccessor->Get(libCZI::IntRect{ mainbbox.x, mainbbox.y, mainbbox.w, mainbbox.h }, &planeCoord, zoom, nullptr);
    pixels = CziBitmapToBuffer(mainbitmap, &base_w, &base_h);
    std::string base_desc = description_generators::make_aperio_description_IFD0(base_w, base_h, tile_size, tile_size, quality, appmag, mpp, br, bb, bg, barcode);
    write_base_ifd(tif, pixels, base_w, base_h, base_desc);

	zoom = 0.04f;
    auto thumbnailbitmap = mainimageAccessor->Get(libCZI::IntRect{ mainbbox.x, mainbbox.y, mainbbox.w, mainbbox.h }, &planeCoord, zoom, nullptr);
    pixels = CziBitmapToBuffer(thumbnailbitmap, &thumbnail_w, &thumbnail_h);
    std::string thumbnail_desc = description_generators::make_aperio_description_thumbnail(base_w, base_h, thumbnail_w, thumbnail_h, quality, appmag, mpp, br, bb, bg, barcode);
    write_thumbnail_ifd(tif, pixels, thumbnail_w, thumbnail_h, thumbnail_desc);
	std::cout << "Thumbnail dims created to fit:" << " W: " << thumbnail_w << " H: " << thumbnail_h << "\n";

	zoom = 0.5f;
    auto ov1bitmap = mainimageAccessor->Get(libCZI::IntRect{ mainbbox.x, mainbbox.y, mainbbox.w, mainbbox.h }, &planeCoord, zoom, nullptr);
    pixels = CziBitmapToBuffer(ov1bitmap, &ov1_w, &ov1_h);
    ov_desc = description_generators::make_aperio_description_overview(base_w, base_h, tile_size, tile_size, ov1_w, ov1_h, quality, appmag, mpp, br, bb, bg, barcode);
    write_pyramid_ifd(tif, pixels, ov1_w, ov1_h, ov_desc);

	zoom = 0.25f;
    auto ov2bitmap = mainimageAccessor->Get(libCZI::IntRect{ mainbbox.x, mainbbox.y, mainbbox.w, mainbbox.h }, &planeCoord, zoom, nullptr);
    pixels = CziBitmapToBuffer(ov2bitmap, &ov2_w, &ov2_h);
    ov_desc = description_generators::make_aperio_description_overview(base_w, base_h, tile_size, tile_size, ov2_w, ov2_h, quality, appmag, mpp, br, bb, bg, barcode);
    write_pyramid_ifd(tif, pixels, ov2_w, ov2_h, ov_desc);

    zoom = 0.125f;
    auto ov3bitmap = mainimageAccessor->Get(libCZI::IntRect{ mainbbox.x, mainbbox.y, mainbbox.w, mainbbox.h }, &planeCoord, zoom, nullptr);
    pixels = CziBitmapToBuffer(ov3bitmap, &ov3_w, &ov3_h);
    ov_desc = description_generators::make_aperio_description_overview(base_w, base_h, tile_size, tile_size, ov3_w, ov3_h, quality, appmag, mpp, br, bb, bg, barcode);
    write_pyramid_ifd(tif, pixels, ov3_w, ov3_h, ov_desc);
    
    auto labelaccessor = labelReader->CreateSingleChannelTileAccessor();
	auto labelbitmap = labelaccessor->Get(libCZI::IntRect{ labelbbox.x, labelbbox.y, labelbbox.w, labelbbox.h }, &planeCoord, nullptr);
	pixels = CziBitmapToBuffer(labelbitmap, &label_w, &label_h);
	std::cout << "Found label image dims:" << " W: " << label_w << " H: " << label_h << "\n";
    std::string label_desc = description_generators::make_aperio_description_label(label_w, label_h);
    write_label_ifd(tif, pixels, label_w, label_h, label_desc);

    auto macroAccessor = macroReader->CreateSingleChannelTileAccessor();
    auto macrobitmap = macroAccessor->Get(libCZI::IntRect{ macrobbox.x, macrobbox.y, macrobbox.w, macrobbox.h }, &planeCoord, nullptr);
    pixels = CziBitmapToBuffer(macrobitmap, &macro_w, &macro_h);
    std::cout << "Found macro image dims:" << " W: " << macro_w << " H: " << macro_h << "\n";
    std::string macro_desc = description_generators::make_aperio_description_macro(macro_w, macro_h);
    write_macro_ifd(tif, pixels, macro_w, macro_h, macro_desc);

    TIFFClose(tif);
    return 0;
}
