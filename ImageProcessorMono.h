#pragma once
#include "ImageProcessor.h"

class ImageProcessorMono : public ImageProcessor
{
protected:
	void splitImage(QList<uint16_t>& imageBuffer, QList<QList<float>>& channels, const QSharedPointer<Metadata>& metadata) override;
	void assembleImage(QList<QList<float>>& channels, QList<uint16_t>& imageBuffer, const QSharedPointer<Metadata>& metadata) override;
	void correct(QList<QList<float>>& imageChannels, QList<QList<float>>& referenceChannels, const ProcessingItem& item) override;
	int getChannelHeight(const QSharedPointer<Metadata>& metadata) override;
	int getChannelWidth(const QSharedPointer<Metadata>& metadata) override;

public:
	int getImageDataSize(const QSharedPointer<Metadata>& metadata) override;
};
