#include "ImageProcessor.h"

#include <QtConcurrent/QtConcurrentMap>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>



void ImageProcessor::blurChannel(OpenCVParcel parcel)
{
	cv::Mat mat = cv::Mat(parcel.height, parcel.width, CV_32F, parcel.channel);
	cv::GaussianBlur(mat, mat, cv::Size(0, 0), parcel.gaussianBlurSigma, parcel.gaussianBlurSigma);
}

void ImageProcessor::blurChannels(QList<QList<float>>& channels, const QSharedPointer<Metadata>& metadata, float gaussianBlurSigma)
{
	QList<OpenCVParcel> openCVParcels;
	for (int i = 0; i < channels.size(); i++)
	{
		openCVParcels.append(OpenCVParcel(channels[i].data(), getChannelHeight(metadata), getChannelWidth(metadata), gaussianBlurSigma));
	}

	QFuture<void> future = QtConcurrent::map(openCVParcels, &ImageProcessor::blurChannel);
	future.waitForFinished();
}

void ImageProcessor::normalizeChannel(QList<float>& channel)
{
	float maximumValue = 0;
	for (int i = 0; i < channel.size(); i++)
	{
		if (channel[i] > maximumValue)
		{
			maximumValue = channel[i];
		}
	}

	for (int i = 0; i < channel.size(); i++)
	{
		channel[i] = channel[i] / maximumValue;
	}
}

void ImageProcessor::scaleChannel(QList<float>& channel, float scale)
{
	for (int i = 0; i < channel.size(); i++)
	{
		channel[i] = channel[i] * scale;
	}
}

void ImageProcessor::clipChannel(QList<float>& channel, uint16_t maxValue)
{
	for (int i = 0; i < channel.size(); i++)
	{
		if (channel[i] > maxValue)
		{
			channel[i] = maxValue;
		}
	}
}

float ImageProcessor::calculateMax(QList<float>& channel)
{
	float maximumValue = 0;
	for (int i = 0; i < channel.size(); i++)
	{
		if (channel[i] > maximumValue)
		{
			maximumValue = channel[i];
		}
	}
	return maximumValue;
}

float ImageProcessor::calculateScale(QList<float>& channel, uint16_t whiteLevel)
{
	const float maxValue = calculateMax(channel);
	if (maxValue > (float)whiteLevel)
	{
		return (float)whiteLevel / maxValue;
	}

	return 1;
}

int ImageProcessor::getActiveAreaHeight(const QSharedPointer<Metadata>& metadata)
{
	return metadata->activeArea[2] - metadata->activeArea[0];
}

int ImageProcessor::getActiveAreaWidth(const QSharedPointer<Metadata>& metadata)
{
	return metadata->activeArea[3] - metadata->activeArea[1];
}

void ImageProcessor::process(QList<uint16_t>& imageBuffer, QList<uint16_t>& referenceBuffer, const ProcessingParcel& parcel, int index, TwoPassProcessingState& twoPassProcessingState)
{
	const ProcessingItem item = parcel.items[index];

	QList<QList<float>> imageChannels(item.sourceFile->metadata->getChannelsCount());
	QList<QList<float>> referenceChannels(item.referenceFile->metadata->getChannelsCount());

	int channelSize = getChannelSize(item.sourceFile->metadata);
	for (int channel = 0; channel < item.sourceFile->metadata->getChannelsCount(); channel++)
	{
		imageChannels[channel] = QList<float>(channelSize);
		referenceChannels[channel] = QList<float>(channelSize);
	}

	splitImage(imageBuffer, imageChannels, item.sourceFile->metadata);
	splitImage(referenceBuffer, referenceChannels, item.referenceFile->metadata);

	blurChannels(referenceChannels, item.referenceFile->metadata, item.processingOptions.gaussianBlurSigma);

	correct(imageChannels, referenceChannels, item);

	scale(imageChannels, parcel, index, twoPassProcessingState);

	assembleImage(imageChannels, imageBuffer, item.sourceFile->metadata);
}

void ImageProcessor::scale(QList<QList<float>>& channels, const ProcessingParcel& parcel, int index, TwoPassProcessingState& twoPassProcessingState)
{
	if (parcel.globalProcessingOptions.scaleChannelsToAvoidClipping)
	{
		float imageScale = 1;

		if (twoPassProcessingState.performBatchScale)
		{
			imageScale = twoPassProcessingState.commonScaleForBatch;
		}
		else
		{
			for (int i = 0; i < channels.size(); i++)
			{
				const float currentChannelScale = calculateScale(channels[i], parcel.globalProcessingOptions.limitToWhiteLevel ? parcel.items[index].sourceFile->metadata->whiteLevels[i] : 0xffff);
				if (currentChannelScale < imageScale)
				{
					imageScale = currentChannelScale;
				}

				if (parcel.globalProcessingOptions.calculateCommonScaleForBatch && imageScale < twoPassProcessingState.commonScaleForBatch)
				{
					twoPassProcessingState.commonScaleForBatch = imageScale;
				}
			}
		}

		if (parcel.globalProcessingOptions.calculateCommonScaleForBatch && !twoPassProcessingState.performBatchScale)
		{
			return;
		}

		for (int i = 0; i < channels.size(); i++)
		{
			scaleChannel(channels[i], imageScale);
		}
	}
	else
	{
		for (int i = 0; i < channels.size(); i++)
		{
			clipChannel(channels[i], parcel.globalProcessingOptions.limitToWhiteLevel ? parcel.items[index].sourceFile->metadata->whiteLevels[i] : 0xffff);
		}
	}
}

int ImageProcessor::getChannelSize(const QSharedPointer<Metadata>& metadata)
{
	return getChannelHeight(metadata) * getChannelWidth(metadata);
}