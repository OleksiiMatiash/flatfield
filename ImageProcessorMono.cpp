#include "ImageProcessorMono.h"

void ImageProcessorMono::splitImage(QList<uint16_t>& imageBuffer, QList<QList<float>>& channels, const QSharedPointer<Metadata>& metadata)
{
	const int imageWidth = metadata->imageWidth;
	const int topOffset = metadata->activeArea[0];
	const int leftOffset = metadata->activeArea[1];
	const int globalOffset = topOffset * imageWidth;
	int dataPointer = globalOffset + leftOffset;
	int channelOffset = 0;

	for (int channelRow = 0; channelRow < getChannelHeight(metadata); channelRow++)
	{
		for (int channelColumn = 0; channelColumn < getChannelWidth(metadata); channelColumn++)
		{
			channels[0][channelOffset] = (float)(imageBuffer[dataPointer + channelColumn] - metadata->blackLevels[0]);

			channelOffset++;
		}
		dataPointer += imageWidth;
	}
}

void ImageProcessorMono::assembleImage(QList<QList<float>>& channels, QList<uint16_t>& imageBuffer, const QSharedPointer<Metadata>& metadata)
{
	const int imageWidth = metadata->imageWidth;
	const int topOffset = metadata->activeArea[0];
	const int leftOffset = metadata->activeArea[1];
	const int globalOffset = topOffset * imageWidth;
	int dataPointer = globalOffset + leftOffset;
	int channelOffset = 0;

	for (int channelRow = 0; channelRow < getChannelHeight(metadata); channelRow++)
	{
		for (int channelColumn = 0; channelColumn < getChannelWidth(metadata); channelColumn++)
		{
			imageBuffer[dataPointer + channelColumn] = (uint16_t)channels[0][channelOffset] + metadata->blackLevels[0];

			channelOffset++;
		}
		dataPointer += imageWidth;
	}
}

void ImageProcessorMono::correct(QList<QList<float>>& imageChannels, QList<QList<float>>& referenceChannels, const ProcessingItem& item)
{
	normalizeChannel(referenceChannels[0]);

	if (item.processingOptions.luminanceCorrectionIntensity > 0.0f)
	{
		for (int i = 0; i < imageChannels[0].size(); i++)
		{
			imageChannels[0][i] = imageChannels[0][i] / (1 - (1 - referenceChannels[0][i] * item.processingOptions.luminanceCorrectionIntensity));
		}
	}
}

int ImageProcessorMono::getChannelHeight(const QSharedPointer<Metadata>& metadata)
{
	return getActiveAreaHeight(metadata);
}

int ImageProcessorMono::getChannelWidth(const QSharedPointer<Metadata>& metadata)
{
	return getActiveAreaWidth(metadata);
}

int ImageProcessorMono::getImageDataSize(const QSharedPointer<Metadata>& metadata)
{
	return metadata->imageHeight * metadata->imageWidth;
}
