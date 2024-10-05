#include <QtConcurrent/QtConcurrent>
#include "Processor.h"
#include "FileUtils.h"
#include "ImageProcessorBayer.h"
#include "ImageProcessorMono.h"
#include "ImageProcessorRGB.h"


void Processor::stopProcessing()
{
	stopAfterCurrent = true;
}

void Processor::process(const ProcessingParcel& parcel)
{
	stopAfterCurrent = false;

	QFuture<void> future = QtConcurrent::run(&Processor::processWorker, this, parcel);
}

void Processor::processWorker(const ProcessingParcel& parcel)
{
	emit signalProcessingStarted(parcel.globalProcessingOptions.calculateCommonScaleForBatch ? parcel.items.size() * 2 : parcel.items.size());

	TwoPassProcessingState twoPassProcessingState;

	int step = 1;
	if (parcel.globalProcessingOptions.scaleChannelsToAvoidClipping && parcel.globalProcessingOptions.calculateCommonScaleForBatch)
	{
		for (int i = 0; i < parcel.items.size(); i++)
		{
			if (stopAfterCurrent)
			{
				emit signalProcessingFinished();
				return;
			}

			ProcessingItem item = parcel.items[i];
			ImageProcessor* imageProcessor = getImageProcessor(item.sourceFile->metadata->rawType);
			QList<uint16_t> imageBuffer(imageProcessor->getImageDataSize(item.sourceFile->metadata));
			QList<uint16_t> referenceBuffer(imageProcessor->getImageDataSize(item.sourceFile->metadata));

			if (!read(item, imageBuffer, referenceBuffer))
			{
				continue;
			}

			imageProcessor->process(imageBuffer, referenceBuffer, parcel, i, twoPassProcessingState);

			delete imageProcessor;

			emit signalProcessingProgressChanged(step++);
		}

		twoPassProcessingState.performBatchScale = true;
	}

	for (int i = 0; i < parcel.items.size(); i++)
	{
		if (stopAfterCurrent)
		{
			emit signalProcessingFinished();
			return;
		}

		ProcessingItem item = parcel.items[i];
		ImageProcessor* imageProcessor = getImageProcessor(item.sourceFile->metadata->rawType);
		QList<uint16_t> imageBuffer(imageProcessor->getImageDataSize(item.sourceFile->metadata));
		QList<uint16_t> referenceBuffer(imageProcessor->getImageDataSize(item.sourceFile->metadata));

		if (!read(item, imageBuffer, referenceBuffer))
		{
			continue;
		}

		imageProcessor->process(imageBuffer, referenceBuffer, parcel, i, twoPassProcessingState);

		delete imageProcessor;

		save(item, parcel.savingOptions, parcel.sourceFileRoot, imageBuffer);

		emit signalProcessingProgressChanged(step++);
	}

	emit signalProcessingFinished();
}

bool Processor::read(const ProcessingItem& item, QList<uint16_t>& imageBuffer, QList<uint16_t>& referenceBuffer)
{
	QFile imageFile(item.sourceFile->filePath);
	QFile referenceFile(item.referenceFile->filePath);

	if (!imageFile.open(QIODevice::ReadOnly) || !referenceFile.open(QIODevice::ReadOnly) || !imageFile.seek(item.sourceFile->metadata->dataOffset) || !referenceFile.seek(item.referenceFile->metadata->dataOffset))
	{
		return false;
	}

	imageFile.read((char*)imageBuffer.data(), item.sourceFile->metadata->dataSize);
	referenceFile.read((char*)referenceBuffer.data(), item.referenceFile->metadata->dataSize);
	return true;
}

void Processor::save(const ProcessingItem& item, const SavingOptions& savingOptions, const QString& sourceFilesRoot, const QList<uint16_t>& imageBuffer)
{
	QString destinationFilePath;

	if (savingOptions.saveTo == SavingOptions::SaveToEnum::Folder)
	{
		destinationFilePath = FileUtils::createDestinationFileInDestinationFolder(item.sourceFile->filePath, sourceFilesRoot, savingOptions);

	}
	else if (savingOptions.saveTo == SavingOptions::Subfolder)
	{
		destinationFilePath = FileUtils::createDestinationFileInSubfolder(item.sourceFile->filePath, savingOptions);
	}

	QFile destinationFile(destinationFilePath);

	if (destinationFile.open(QIODevice::ReadWrite) && destinationFile.seek(item.sourceFile->metadata->dataOffset))
	{
		destinationFile.write((char*)imageBuffer.data(), item.sourceFile->metadata->dataSize);
	}
}

ImageProcessor* Processor::getImageProcessor(Metadata::RawTypeEnum rawType)
{
	if (rawType == Metadata::RawTypeEnum::Mono)
	{
		return new ImageProcessorMono();
	}
	else if (rawType == Metadata::RawTypeEnum::RGB)
	{
		return new ImageProcessorRGB();
	}
	else
	{
		return new ImageProcessorBayer();
	}
}
