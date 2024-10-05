#include "ImageProcessorBayer.h"

void ImageProcessorBayer::splitImage(QList<uint16_t>& imageBuffer, QList<QList<float>>& channels, const QSharedPointer<Metadata>& metadata)
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
			channels[0][channelOffset] = (float)(imageBuffer[dataPointer + channelColumn * 2] - metadata->blackLevels[0]);
			channels[1][channelOffset] = (float)(imageBuffer[dataPointer + channelColumn * 2 + 1] - metadata->blackLevels[1]);
			channels[2][channelOffset] = (float)(imageBuffer[dataPointer + imageWidth + channelColumn * 2] - metadata->blackLevels[2]);
			channels[3][channelOffset] = (float)(imageBuffer[dataPointer + imageWidth + channelColumn * 2 + 1] - metadata->blackLevels[3]);

			channelOffset++;
		}
		dataPointer += imageWidth * 2;
	}
}

void ImageProcessorBayer::assembleImage(QList<QList<float>>& channels, QList<uint16_t>& imageBuffer, const QSharedPointer<Metadata>& metadata)
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
			imageBuffer[dataPointer + channelColumn * 2] = (uint16_t)channels[0][channelOffset] + metadata->blackLevels[0];
			imageBuffer[dataPointer + channelColumn * 2 + 1] = (uint16_t)channels[1][channelOffset] + metadata->blackLevels[1];
			imageBuffer[dataPointer + imageWidth + channelColumn * 2] = (uint16_t)channels[2][channelOffset] + metadata->blackLevels[2];
			imageBuffer[dataPointer + imageWidth + channelColumn * 2 + 1] = (uint16_t)channels[3][channelOffset] + metadata->blackLevels[3];
			channelOffset++;
		}
		dataPointer += imageWidth * 2;
	}
}

void ImageProcessorBayer::correct(QList<QList<float>>& imageChannels, QList<QList<float>>& referenceChannels, const ProcessingItem& item)
{
	// Averaging reference green channels, scaling R/B channels to 0..1
	QList<float> averagedGreenReferenceChannels(getChannelSize(item.sourceFile->metadata));
	int greenChannels = 0;
	for (int channel = 0; channel < imageChannels.size(); channel++)
	{
		if (item.sourceFile->metadata->cfaColorPattern[channel] == Metadata::CFAPatternEnum::Green)
		{
			for (int i = 0; i < referenceChannels[channel].size(); i++)
			{
				if (greenChannels == 0)
				{
					averagedGreenReferenceChannels[i] = referenceChannels[channel][i];
				}
				else
				{
					averagedGreenReferenceChannels[i] += referenceChannels[channel][i];
				}
			}
			greenChannels++;
		}
		else
		{
			normalizeChannel(referenceChannels[channel]);
		}
	}

	normalizeChannel(averagedGreenReferenceChannels);

	for (int channel = 0; channel < imageChannels.size(); channel++)
	{
		//	Correcting luminance.
		if (item.processingOptions.luminanceCorrectionIntensity > 0.0f)
		{
			for (int i = 0; i < imageChannels[channel].size(); i++)
			{
				imageChannels[channel][i] = imageChannels[channel][i] / (1 - item.processingOptions.luminanceCorrectionIntensity + averagedGreenReferenceChannels[i] * item.processingOptions.luminanceCorrectionIntensity);
			}
		}

		//	Removing luminance correction from R\B reference channels and correcting color in R\B image channels
		if (item.sourceFile->metadata->cfaColorPattern[channel] != Metadata::CFAPatternEnum::Green && item.processingOptions.colorCorrectionIntensity > 0.0f)
		{
			for (int i = 0; i < imageChannels[channel].size(); i++)
			{
				imageChannels[channel][i] = imageChannels[channel][i] / (1 - item.processingOptions.colorCorrectionIntensity + referenceChannels[channel][i] / averagedGreenReferenceChannels[i] * item.processingOptions.colorCorrectionIntensity);
			}
		}
	}
}

int ImageProcessorBayer::getChannelHeight(const QSharedPointer<Metadata>& metadata)
{
	return getActiveAreaHeight(metadata) / 2;
}

int ImageProcessorBayer::getChannelWidth(const QSharedPointer<Metadata>& metadata)
{
	return getActiveAreaWidth(metadata) / 2;
}

int ImageProcessorBayer::getImageDataSize(const QSharedPointer<Metadata>& metadata)
{
	return metadata->imageHeight * metadata->imageWidth;
}
