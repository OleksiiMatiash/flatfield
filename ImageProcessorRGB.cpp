#include "ImageProcessorRGB.h"

void ImageProcessorRGB::splitImage(QList<uint16_t>& imageBuffer, QList<QList<float>>& channels, const QSharedPointer<Metadata>& metadata)
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
			channels[0][channelOffset] = (float)(imageBuffer[dataPointer + channelColumn * 3] - metadata->blackLevels[0]);
			channels[1][channelOffset] = (float)(imageBuffer[dataPointer + channelColumn * 3 + 1] - metadata->blackLevels[1]);
			channels[2][channelOffset] = (float)(imageBuffer[dataPointer + channelColumn * 3 + 2] - metadata->blackLevels[2]);

			channelOffset++;
		}
		dataPointer += imageWidth * 3;
	}
}

void ImageProcessorRGB::assembleImage(QList<QList<float>>& channels, QList<uint16_t>& imageBuffer, const QSharedPointer<Metadata>& metadata)
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
			imageBuffer[dataPointer + channelColumn * 3] = (uint16_t)channels[0][channelOffset] + metadata->blackLevels[0];
			imageBuffer[dataPointer + channelColumn * 3 + 1] = (uint16_t)channels[1][channelOffset] + metadata->blackLevels[1];
			imageBuffer[dataPointer + channelColumn * 3 + 2] = (uint16_t)channels[2][channelOffset] + metadata->blackLevels[2];

			channelOffset++;
		}
		dataPointer += imageWidth * 3;
	}
}

void ImageProcessorRGB::correct(QList<QList<float>>& imageChannels, QList<QList<float>>& referenceChannels, const ProcessingItem& item)
{
	for (int i = 0; i < referenceChannels.size(); i++)
	{
		normalizeChannel(referenceChannels[i]);
	}

	for (int channel = 0; channel < imageChannels.size(); channel++)
	{
		//	Correcting luminance.
		if (item.processingOptions.luminanceCorrectionIntensity > 0.0f)
		{
			for (int i = 0; i < imageChannels[channel].size(); i++)
			{
				imageChannels[channel][i] = imageChannels[channel][i] / (1 - (1 - referenceChannels[1][i] * item.processingOptions.luminanceCorrectionIntensity));
			}
		}

		//	Removing luminance correction from R\B reference channels and correcting color in R\B image channels
		if (channel != 1 && item.processingOptions.colorCorrectionIntensity > 0.0f)
		{
			for (int i = 0; i < imageChannels[channel].size(); i++)
			{
				imageChannels[channel][i] = imageChannels[channel][i] / (1 - (1 - referenceChannels[channel][i] / referenceChannels[1][i] * item.processingOptions.colorCorrectionIntensity));
			}
		}
	}
}

int ImageProcessorRGB::getChannelHeight(const QSharedPointer<Metadata>& metadata)
{
	return getActiveAreaHeight(metadata);
}

int ImageProcessorRGB::getChannelWidth(const QSharedPointer<Metadata>& metadata)
{
	return getActiveAreaWidth(metadata);
}

int ImageProcessorRGB::getImageDataSize(const QSharedPointer<Metadata>& metadata)
{
	return metadata->imageHeight * metadata->imageWidth * 3;
}
