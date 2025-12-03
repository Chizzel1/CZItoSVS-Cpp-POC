#include <iostream>
#include <string>
#include <tiffio.h>
#include "stb_image.h"
#include <libCZI.h>
 
#include <iostream>
#include <memory>
#include <libCZI_Compositor.h>
#include <exception>
#include <cstdint>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>
#include <libCZI_DimCoordinate.h>
#include <libCZI_Pixels.h>
#include <string.h>
#include <codecvt>
#include <locale>
#include <cmath>
#include <limits>
#include <libCZI_Metadata.h>


static void writeBMP(const std::string& filename,
    const std::shared_ptr<libCZI::IBitmapData>& bitmap)
{
    using namespace libCZI;

    if (bitmap->GetPixelType() != PixelType::Bgr24) {
        std::cerr << "Bitmap is not Bgr24, got pixel type = "
            << static_cast<int>(bitmap->GetPixelType()) << "\n";
        return;
    }

    const uint32_t width = bitmap->GetWidth();
    const uint32_t height = bitmap->GetHeight();

    BitmapLockInfo lockInfo = bitmap->Lock();
    auto* srcBase = static_cast<const uint8_t*>(lockInfo.ptrDataRoi);
    const uint32_t srcStride = lockInfo.stride;   // bytes per row from libCZI

    // BMP rows are padded to a 4-byte boundary
    const uint32_t bytesPerPixel = 3;
    const uint32_t rowSize = ((width * bytesPerPixel + 3) / 4) * 4;
    const uint32_t imageSize = rowSize * height;
    const uint32_t fileSize = 14 + 40 + imageSize; // file + info headers

    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        bitmap->Unlock();
        throw std::runtime_error("Failed to open output file " + filename);
    }

    auto writeLE16 = [](uint8_t* dst, uint16_t v) {
        dst[0] = static_cast<uint8_t>(v & 0xFF);
        dst[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        };

    auto writeLE32 = [](uint8_t* dst, uint32_t v) {
        dst[0] = static_cast<uint8_t>(v & 0xFF);
        dst[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
        dst[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
        dst[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
        };

    // ---- BITMAPFILEHEADER (14 bytes) ----
    uint8_t fileHeader[14] = {};
    fileHeader[0] = 'B';
    fileHeader[1] = 'M';
    writeLE32(&fileHeader[2], fileSize);
    // bytes 6–9 are reserved = 0
    writeLE32(&fileHeader[10], 14 + 40); // offset to pixel data (54)

    // ---- BITMAPINFOHEADER (40 bytes) ----
    uint8_t infoHeader[40] = {};
    writeLE32(&infoHeader[0], 40);          // header size
    writeLE32(&infoHeader[4], width);      // width
    writeLE32(&infoHeader[8], height);     // height (positive = bottom-up)
    writeLE16(&infoHeader[12], 1);          // planes
    writeLE16(&infoHeader[14], 24);         // bitCount
    writeLE32(&infoHeader[16], 0);          // compression = BI_RGB (none)
    writeLE32(&infoHeader[20], imageSize);  // image size
    writeLE32(&infoHeader[24], 2835);       // xPelsPerMeter (~72 DPI)
    writeLE32(&infoHeader[28], 2835);       // yPelsPerMeter
    writeLE32(&infoHeader[32], 0);          // clrUsed
    writeLE32(&infoHeader[36], 0);          // clrImportant

    ofs.write(reinterpret_cast<char*>(fileHeader), sizeof(fileHeader));
    ofs.write(reinterpret_cast<char*>(infoHeader), sizeof(infoHeader));

    // ---- Pixel data (bottom-up) ----
    std::vector<uint8_t> rowBuf(rowSize);

    for (uint32_t y = 0; y < height; ++y) {
        // BMP stores rows bottom->top
        uint32_t srcY = height - 1 - y;
        const uint8_t* srcRow = srcBase + srcY * srcStride;

        // Copy the visible part
        std::memcpy(rowBuf.data(), srcRow, width * bytesPerPixel);

        // Zero the padding (if any)
        if (rowSize > width * bytesPerPixel) {
            std::memset(rowBuf.data() + width * bytesPerPixel, 0,
                rowSize - width * bytesPerPixel);
        }

        ofs.write(reinterpret_cast<const char*>(rowBuf.data()), rowSize);
    }

    bitmap->Unlock();
}

inline std::string ws2utf8(const std::wstring& ws)
{
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(ws);
}

struct CziMetadata {
    std::string barcode;        // e.g. "562298--25-290-2"
    double mppX_um;             // microns per pixel in X
    double mppY_um;             // microns per pixel in Y
    double apparentMag;         // e.g. 20.0
    double minificationFactor;  // e.g. 2.0
    int    pyramidLayers;       // e.g. 5
};

struct CziMetadataDefaults {
    std::string barcode = "";   // fallback if no barcode in XML
    double mppX_um = std::numeric_limits<double>::quiet_NaN();
    double mppY_um = std::numeric_limits<double>::quiet_NaN();
    double apparentMag = 21;
    double minificationFactor = 2.0;  // sensible Axioscan default
    int    pyramidLayers = 1;         // if no pyramid info
};

static inline std::string trim(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Find first <Tag>value</Tag> and return value as string
static bool findFirstTagValue(const std::string& xml,
    const std::string& tagName,
    std::string& out)
{
    const std::string open = "<" + tagName + ">";
    const std::string close = "</" + tagName + ">";

    auto pos = xml.find(open);
    if (pos == std::string::npos) return false;

    auto start = pos + open.size();
    auto end = xml.find(close, start);
    if (end == std::string::npos) return false;

    out = trim(xml.substr(start, end - start));
    return !out.empty();
}

// Find *all* <Tag>v</Tag>, return max double value (or NaN if none)
static double findMaxTagDouble(const std::string& xml,
    const std::string& tagName)
{
    const std::string open = "<" + tagName + ">";
    const std::string close = "</" + tagName + ">";

    double best = std::numeric_limits<double>::quiet_NaN();
    auto pos = xml.find(open);
    while (pos != std::string::npos) {
        auto start = pos + open.size();
        auto end = xml.find(close, start);
        if (end == std::string::npos) break;

        std::string text = trim(xml.substr(start, end - start));
        try {
            double v = std::stod(text);
            if (std::isnan(best) || v > best) {
                best = v;
            }
        }
        catch (...) {
            // ignore parse failures
        }

        pos = xml.find(open, end + close.size());
    }
    return best;
}

static CziMetadata extractCziMetadataFromXml(const std::string& xml,
    const CziMetadataDefaults& defaults = {})
{
    CziMetadata md{};
    md.barcode = defaults.barcode;
    md.mppX_um = defaults.mppX_um;
    md.mppY_um = defaults.mppY_um;
    md.apparentMag = defaults.apparentMag;
    md.minificationFactor = defaults.minificationFactor;
    md.pyramidLayers = defaults.pyramidLayers;

    // ---------- Barcode ----------
    // Restrict search to the <Barcodes> section so we don't hit other <Content>
    {
        std::string barcodeSection;
        {
            const std::string startTag = "<Barcodes>";
            const std::string endTag = "</Barcodes>";
            auto start = xml.find(startTag);
            auto end = xml.find(endTag, start == std::string::npos ? 0 : start);
            if (start != std::string::npos && end != std::string::npos) {
                end += endTag.size();
                barcodeSection = xml.substr(start, end - start);
            }
        }

        std::string content;
        if (!barcodeSection.empty() &&
            findFirstTagValue(barcodeSection, "Content", content))
        {
            md.barcode = content;
        }
    }

    // ---------- Microns per pixel ----------
    // From <Scaling><Items><Distance Id="X"><Value>1.725E-07</Value> ... </Distance>
    // We'll just take the first <Value> in the <Scaling> block for X and Y.
    {
        const std::string scalingTag = "<Scaling>";
        const std::string scalingEnd = "</Scaling>";
        auto s = xml.find(scalingTag);
        auto e = xml.find(scalingEnd, s == std::string::npos ? 0 : s);
        if (s != std::string::npos && e != std::string::npos) {
            e += scalingEnd.size();
            std::string scaling = xml.substr(s, e - s);

            // Simple: find first two <Value> entries and assume X/Y
            std::string v1, v2;
            if (findFirstTagValue(scaling, "Value", v1)) {
                try {
                    double m_per_px = std::stod(v1);
                    md.mppX_um = m_per_px * 1e6;
                }
                catch (...) {}
            }
            // look for second <Value> after the first close
            auto secondPos = scaling.find("</Value>");
            if (secondPos != std::string::npos) {
                secondPos = scaling.find("<Value>", secondPos);
                if (secondPos != std::string::npos) {
                    std::string tmp = scaling.substr(secondPos);
                    if (findFirstTagValue(tmp, "Value", v2)) {
                        try {
                            double m_per_px = std::stod(v2);
                            md.mppY_um = m_per_px * 1e6;
                        }
                        catch (...) {}
                    }
                }
            }
        }
    }

    // ---------- Apparent magnification (final system mag) ----------
    // Use the maximum of all <TotalMagnification> and <TheoreticalTotalMagnification>.
    {
        double bestTot = findMaxTagDouble(xml, "TotalMagnification");
        double bestTheo = findMaxTagDouble(xml, "TheoreticalTotalMagnification");

        double best = bestTot;
        if (std::isnan(best) || (!std::isnan(bestTheo) && bestTheo > best)) {
            best = bestTheo;
        }
        if (!std::isnan(best)) {
            md.apparentMag = best;
        }
    }

    // ---------- Minification factor ----------
    {
        double v = findMaxTagDouble(xml, "MinificationFactor");
        if (!std::isnan(v)) {
            md.minificationFactor = v;
        }
    }

    // ---------- Pyramid layers ----------
    {
        double v = findMaxTagDouble(xml, "PyramidLayersCount");
        if (!std::isnan(v)) {
            md.pyramidLayers = static_cast<int>(v);
        }
    }

    return md;
}


static void dumpStructuredDocInfo(const std::shared_ptr<libCZI::ICziMetadata>& meta)
{
    using namespace libCZI;

    std::shared_ptr<ICziMultiDimensionDocumentInfo> docInfo = meta->GetDocumentInfo();
    if (!docInfo) {
        std::cout << "No ICziMultiDimensionDocumentInfo present.\n";
        return;
    }

    // --- General document info ---
    GeneralDocumentInfo g = docInfo->GetGeneralDocumentInfo();

    std::wcout << L"---- GeneralDocumentInfo ----\n";
    if (g.name_valid)         std::wcout << L"Name: " << g.name << L"\n";
    if (g.title_valid)        std::wcout << L"Title: " << g.title << L"\n";
    if (g.description_valid)  std::wcout << L"Description: " << g.description << L"\n";
    if (g.userName_valid)     std::wcout << L"UserName: " << g.userName << L"\n";
    if (g.comment_valid)      std::wcout << L"Comment: " << g.comment << L"\n";
    if (g.keywords_valid)     std::wcout << L"Keywords: " << g.keywords << L"\n";
    if (g.creationDateTime_valid) {
        std::wcout << L"CreationDateTime present in metadata\n";
        // You can inspect g.creationDateTime.year, month, day, etc if you like
    }
    std::wcout << L"\n";

    // --- Scaling info (pixel size etc.) ---
    ScalingInfo s = docInfo->GetScalingInfo();

    std::cout << "---- ScalingInfo ----\n";
    if (s.IsScaleXValid()) {
        std::cout << "scaleX (m/pixel): " << s.scaleX
            << "  ~ " << s.scaleX * 1e6 << " um/pixel\n";
    }
    if (s.IsScaleYValid()) {
        std::cout << "scaleY (m/pixel): " << s.scaleY
            << "  ~ " << s.scaleY * 1e6 << " um/pixel\n";
    }
    if (s.IsScaleZValid()) {
        std::cout << "scaleZ (m/step):  " << s.scaleZ
            << "  ~ " << s.scaleZ * 1e6 << " um/step\n";
    }
    std::cout << "\n";

    // If you want to go further (dimension bounds, channels, etc.),
    // this is the object to explore in IntelliSense.
}

static void writeMetadataXmlToFile(const std::shared_ptr<libCZI::ICziMetadata>& meta,
    const std::string& filename)
{
    std::string xml = meta->GetXml();  // UTF-8 XML string
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "Could not open " << filename << " for writing\n";
        return;
    }
    ofs.write(xml.data(), static_cast<std::streamsize>(xml.size()));
    std::cout << "Wrote metadata XML to " << filename << "\n";
}

int main() {
    try {
        // Create file-backed stream
        std::shared_ptr<libCZI::IStream> stream = libCZI::CreateStreamFromFile(LR"(C:\Users\lewpi\Downloads\580152_H303164_25-602_1-17_03_2025.czi)");

        // Create the CZI reader
        std::shared_ptr<libCZI::ICZIReader> reader = libCZI::CreateCZIReader();

        // Open the CZI file
        reader->Open(stream);


        std::cout << "CZI file opened successfully." << std::endl;


        libCZI::SubBlockStatistics stats = reader->GetStatistics();


        const libCZI::IntRect& bbox = stats.boundingBox;

        int x = bbox.x;
        int y = bbox.y;
        int w = bbox.w;
        int h = bbox.h;
        w = 5024;
        h = 5024;
        x = x + 20000;
        y = y + 10000;
        libCZI::IntRect roi{ x, y, w, h };
        std::cout << "Bounding box layer 0: "
            << "x=" << x
            << " y=" << y
            << " w=" << w
            << " h=" << h
            << std::endl;

        std::cout << "Debug" << std::endl;
        libCZI::CDimCoordinate plane;

        plane.Set(libCZI::DimensionIndex::C, 0);
        std::shared_ptr accessor = reader->CreateSingleChannelScalingTileAccessor();
        libCZI::ISingleChannelScalingTileAccessor::Options options;
        options.Clear();
        options.backGroundColor = libCZI::RgbFloatColor{ 0.5f, 1.0f, 0.2f };

        float zoom = 1.0f;
        std::shared_ptr<libCZI::IBitmapData> bitmap = accessor->Get(roi, &plane, zoom, &options);
        writeBMP("output.bmp", bitmap);

        auto metaSeg = reader->ReadMetadataSegment();
        if (!metaSeg) {
            std::cout << "No metadata segment in this file.\n";
        }
        else {
            auto meta = metaSeg->CreateMetaFromMetadataSegment();
            if (!meta) {
                std::cout << "Failed to create ICziMetadata.\n";
            }
            else if (!meta->IsXmlValid()) {
                std::cout << "Metadata XML is invalid.\n";
            }
            else {
                // Optional: still dump the XML for inspection
                writeMetadataXmlToFile(meta, "metadata.xml");

                // NEW: parse XML string without libCZI helpers
                std::string xml = meta->GetXml();
                CziMetadataDefaults def;        // set any fallbacks you want
                CziMetadata md = extractCziMetadataFromXml(xml, def);

                std::cout << "Barcode:           " << md.barcode << "\n";
                std::cout << "MPP X:             " << md.mppX_um << " um/px\n";
                std::cout << "MPP Y:             " << md.mppY_um << " um/px\n";
                std::cout << "Apparent mag:      " << md.apparentMag << "x\n";
                std::cout << "Minification:      " << md.minificationFactor << "\n";
                std::cout << "Pyramid layers:    " << md.pyramidLayers << "\n";
            }
        }
    
    }
	catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return 0;

}




