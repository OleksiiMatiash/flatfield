#pragma once
#include "DataStructs.h"

class ImageProcessor
{
	struct OpenCVParcel
	{
		float* channel;
		int height;
		int width;
		float gaussianBlurSigma;

		OpenCVParcel(float* channel, int height, int width, float gaussianBlurSigma)
		{
			this->channel = channel;
			this->height = height;
			this->width = width;
			this->gaussianBlurSigma = gaussianBlurSigma;
		}
	};

protected:
	virtual void splitImage(QList<uint16_t>& imageBuffer, QList<QList<float>>& channels, const QSharedPointer<Metadata>& metadata) = 0;
	virtual void assembleImage(QList<QList<float>>& channels, QList<uint16_t>& imageBuffer, const QSharedPointer<Metadata>& metadata) = 0;
	virtual void correct(QList<QList<float>>& imageChannels, QList<QList<float>>& referenceChannels, const ProcessingItem& item) = 0;
	virtual int getChannelHeight(const QSharedPointer<Metadata>& metadata) = 0;
	virtual int getChannelWidth(const QSharedPointer<Metadata>& metadata) = 0;

	void blurChannels(QList<QList<float>>& channels, const QSharedPointer<Metadata>& metadata, float gaussianBlurSigma);
	static void blurChannel(OpenCVParcel parcel);
	static void normalizeChannel(QList<float>& channel);
	static void scaleChannel(QList<float>& channel, float scale);
	static void clipChannel(QList<float>& channel, uint16_t maxValue);
	static float calculateMax(QList<float>& channel);
	static float calculateScale(QList<float>& channel, uint16_t whiteLevel);
	static int getActiveAreaHeight(const QSharedPointer<Metadata>& metadata);
	static int getActiveAreaWidth(const QSharedPointer<Metadata>& metadata);

public:
	virtual ~ImageProcessor() = default;
	virtual int getImageDataSize(const QSharedPointer<Metadata>& metadata) = 0;

	void process(QList<uint16_t>& imageBuffer, QList<uint16_t>& referenceBuffer, const ProcessingParcel& parcel, int index, TwoPassProcessingState & twoPassProcessingState);
	static void scale(QList<QList<float>>& channels, const ProcessingParcel& parcel, int index, TwoPassProcessingState&
	                  twoPassProcessingState);
	int getChannelSize(const QSharedPointer<Metadata>& metadata);
};
