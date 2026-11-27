#include "document.h"

namespace
{

// Значение PhotometricInterpretation для LinearRaw
constexpr uint16_t linearRawPhotometric = 34892;

QImage loadFloatTiff(const QString& filePath)
{
    // В Windows используем Unicode-путь, в Linux - системную кодировку
#ifdef Q_OS_WIN
    TIFF* handle = TIFFOpenW(
        reinterpret_cast<const wchar_t*>(filePath.utf16()), "r");
#else
    const QByteArray encodedPath = QFile::encodeName(filePath);
    TIFF* handle = TIFFOpen(encodedPath.constData(), "r");
#endif

    // Закрываем TIFF автоматически при любом выходе из функции
    std::unique_ptr<TIFF, decltype(&TIFFClose)> tiff(handle, &TIFFClose);
    if (!tiff)
        return QImage();

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t bitsPerSample = 0;
    uint16_t samplesPerPixel = 1;
    uint16_t sampleFormat = SAMPLEFORMAT_UINT;
    uint16_t photometric = PHOTOMETRIC_MINISBLACK;
    uint16_t orientation = ORIENTATION_TOPLEFT;

    if (!TIFFGetField(tiff.get(), TIFFTAG_IMAGEWIDTH, &width)
        || !TIFFGetField(tiff.get(), TIFFTAG_IMAGELENGTH, &height)
        || !TIFFGetField(tiff.get(), TIFFTAG_BITSPERSAMPLE, &bitsPerSample)
        || !TIFFGetField(tiff.get(), TIFFTAG_PHOTOMETRIC, &photometric))
    {
        return QImage();
    }

    TIFFGetFieldDefaulted(
        tiff.get(), TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    TIFFGetFieldDefaulted(
        tiff.get(), TIFFTAG_SAMPLEFORMAT, &sampleFormat);
    TIFFGetFieldDefaulted(
        tiff.get(), TIFFTAG_ORIENTATION, &orientation);

    // Этот загрузчик предназначен только для одноканального float32
    if (bitsPerSample != 32 || samplesPerPixel != 1
        || sampleFormat != SAMPLEFORMAT_IEEEFP)
    {
        return QImage();
    }

    // На первом этапе поддерживаем построчные TIFF без поворота
    if (TIFFIsTiled(tiff.get()) || orientation != ORIENTATION_TOPLEFT)
        return QImage();

    if (photometric != PHOTOMETRIC_MINISBLACK
        && photometric != PHOTOMETRIC_MINISWHITE
        && photometric != linearRawPhotometric)
    {
        return QImage();
    }

    const uint32_t maxDimension =
        static_cast<uint32_t>(std::numeric_limits<int>::max());

    if (width == 0 || height == 0
        || width > maxDimension || height > maxDimension)
    {
        return QImage();
    }

    const tmsize_t scanlineSize = TIFFScanlineSize(tiff.get());

    if (scanlineSize <= 0
        || static_cast<uint64_t>(scanlineSize)
            != static_cast<uint64_t>(width) * sizeof(float))
    {
        return QImage();
    }

    // Буфер занимает только одну строку исходного изображения
    std::unique_ptr<float, decltype(&_TIFFfree)> row(
        static_cast<float*>(_TIFFmalloc(scanlineSize)), &_TIFFfree);

    if (!row)
        return QImage();

    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();

    // Первый проход: определяем диапазон конечных значений
    for (uint32_t y = 0; y < height; ++y)
    {
        if (TIFFReadScanline(tiff.get(), row.get(), y, 0) != 1)
            return QImage();

        for (uint32_t x = 0; x < width; ++x)
        {
            const double value = row.get()[x];

            if (!std::isfinite(value))
                continue;

            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
    }

    if (!std::isfinite(minimum))
        return QImage();

    QImage image(
        static_cast<int>(width),
        static_cast<int>(height),
        QImage::Format_Grayscale8);

    if (image.isNull())
        return QImage();

    image.fill(0);

    // Перезапускаем чтение, в том числе для TIFF со сжатыми полосами
    if (!TIFFSetDirectory(tiff.get(), 0))
        return QImage();

    const double range = maximum - minimum;

    // Второй проход: переводим исходную яркость в диапазон 0–255
    for (uint32_t y = 0; y < height; ++y)
    {
        if (TIFFReadScanline(tiff.get(), row.get(), y, 0) != 1)
            return QImage();

        uchar* destination = image.scanLine(static_cast<int>(y));

        for (uint32_t x = 0; x < width; ++x)
        {
            const double value = row.get()[x];

            // NaN и бесконечности отображаем чёрным
            if (!std::isfinite(value))
                continue;

            double brightness = 0.0;

            if (range > 0.0)
                brightness = (value - minimum) / range;

            if (photometric == PHOTOMETRIC_MINISWHITE)
                brightness = 1.0 - brightness;

            brightness = std::max(0.0, std::min(1.0, brightness));

            destination[x] =
                static_cast<uchar>(brightness * 255.0 + 0.5);
        }
    }

    return image;
}

}

Document::Document()
{
    scene = simple_ptr<QGraphicsScene>::create();
}

Document::~Document()
{
    _undoStack.reset();
}

Document::Ptr Document::create(const QString& path)
{
    Ptr doc = Ptr::create();
    doc->filePath = path;
    doc->polygonList.model.setColumnCount(1);

    return doc;
}

bool Document::loadImage()
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    QImage image;

    // Для TIFF сначала пробуем загрузчик одноканального float32
    if (suffix == "tif" || suffix == "tiff")
        image = loadFloatTiff(filePath);

    if (!image.isNull())
        pixmap = QPixmap::fromImage(image);
    else
        pixmap = QPixmap(filePath);

    if (pixmap.isNull())
        return false;

    if (!videoRect)
    {
        videoRect = new qgraph::VideoRect(scene);
    }
    videoRect->setPixmap(pixmap);

    videoRect->setPos(0, 0);
    scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(pixmap.size())));
    return true;
}

void Document::markModified()
{
    isModified = true;
    autoSaveUpToDate = false;
}
