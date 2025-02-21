#pragma once

#include "video/transport/frames/udp.h"
#include "video/transport/compression.h"
#include <QImage>

namespace qgraph {

/**
  Функции преобразуют изображение из формата cv::Mat в QImage

  data   - указатель на буфер данных в формате cv::Mat;
  size   - размер буфера данных;
  width  - ширина изображения;
  height - высота изображения;
  cvStep - размер одной записи (one-row) изображения в представлении cv::Mat;
  cvType - тип матрицы изображения в представлении OpenCV
*/
QImage cvMatToQImage(
    const char* data, int size, int width, int height, int cvStep, int cvType,
    pproto::VideoFrameCompression compression = pproto::VideoFrameCompression::None);

QImage cvMatToQImage(
    const QByteArray& data, int width, int height, int cvStep, int cvType,
    pproto::VideoFrameCompression compression = pproto::VideoFrameCompression::None);

QImage cvMatToQImage(const video::transport::FrameHeader::Ptr& header,
                     const video::transport::FrameBuff::Ptr& data);

} // namespace qgraph {
