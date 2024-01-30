#include "functions.h"

#include "shared/logger/logger.h"
#include "shared/logger/format.h"
#include "video/transport/compression.h"

#define log_error_m   alog::logger().error   (alog_line_location, "QGraph")
#define log_warn_m    alog::logger().warn    (alog_line_location, "QGraph")
#define log_info_m    alog::logger().info    (alog_line_location, "QGraph")
#define log_verbose_m alog::logger().verbose (alog_line_location, "QGraph")
#define log_debug_m   alog::logger().debug   (alog_line_location, "QGraph")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "QGraph")

#define CV_CN_MAX     512
#define CV_CN_SHIFT   3
#define CV_DEPTH_MAX  (1 << CV_CN_SHIFT)

#define CV_8U   0
#define CV_8S   1
#define CV_16U  2
#define CV_16S  3
#define CV_32S  4
#define CV_32F  5
#define CV_64F  6
#define CV_USRTYPE1 7

#define CV_MAT_DEPTH_MASK     (CV_DEPTH_MAX - 1)
#define CV_MAT_DEPTH(flags)   ((flags) & CV_MAT_DEPTH_MASK)

#define CV_MAKETYPE(depth,cn) (CV_MAT_DEPTH(depth) + (((cn)-1) << CV_CN_SHIFT))
#define CV_MAKE_TYPE CV_MAKETYPE

#define CV_8UC1 CV_MAKETYPE(CV_8U,1)
#define CV_8UC2 CV_MAKETYPE(CV_8U,2)
#define CV_8UC3 CV_MAKETYPE(CV_8U,3)
#define CV_8UC4 CV_MAKETYPE(CV_8U,4)
#define CV_8UC(n) CV_MAKETYPE(CV_8U,(n))

namespace qgraph {

using namespace pproto;

QImage cvMatToQImage(const char* data, int size, int width, int height,
                     int cvStep, int cvType, VideoFrameCompression compression)
{
    QByteArray dataBuff;
    if (compression != VideoFrameCompression::None)
    {
        if (compression == VideoFrameCompression::Zip)
        {
            dataBuff = qUncompress((uchar*)data, size);
        }
        else if (compression >= VideoFrameCompression::Jpeg90
                 && compression <= VideoFrameCompression::Jpeg50)
        {
            return QImage::fromData((uchar*)data, size, "JPG");
        }
        else
        {
            log_error << "Unsupported image compression format";
            return QImage();
        }
    }
    else
        dataBuff = QByteArray::fromRawData(data, size);

    switch (cvType)
    {
        case CV_8UC4: // 8-bit, 4 channel
        {
            return QImage((uchar*)dataBuff.constData(),
                          width, height, cvStep, QImage::Format_RGB32
                         ).copy(); // Обязательно делать копию QImage
        }
        case CV_8UC3: // 8-bit, 3 channel
        {
            return QImage((uchar*)dataBuff.constData(),
                          width, height, cvStep, QImage::Format_RGB888
                         ).rgbSwapped(); // Здесь уже возвращается копия QImage
        }
        case CV_8UC1: // 8-bit, 1 channel
        {
           static QVector<QRgb>  sColorTable;
           // Only create our color table once
           if (sColorTable.isEmpty())
              for (int i = 0; i < 256; ++i)
                 sColorTable.push_back(qRgb(i, i, i));

           QImage img((uchar*)dataBuff.constData(),
                      width, height, cvStep, QImage::Format_Indexed8);
           img.setColorTable(sColorTable);
           return img.copy(); // Обязательно делать копию QImage
        }
        default:
           log_warn << "cv::Mat image type not handled; type: " << cvType;
    }
    return QImage();
}

QImage cvMatToQImage(const QByteArray& data, int width, int height,
                     int cvStep, int cvType, VideoFrameCompression compression)
{
    return cvMatToQImage(data.constData(), data.size(), width, height,
                         cvStep, cvType, compression);
}

QImage cvMatToQImage(const video::transport::FrameHeader::Ptr& header,
                     const video::transport::FrameBuff::Ptr& data)
{
    return cvMatToQImage(data->ptr(), data->size(),
                         int(header->scaleWidth), int(header->scaleHeight),
                         int(header->scaleStep), header->cvType,
                         static_cast<VideoFrameCompression>(header->compress));
}

} // namespace qgraph

#undef log_error_m
#undef log_warn_m
#undef log_info_m
#undef log_verbose_m
#undef log_debug_m
#undef log_debug2_m
