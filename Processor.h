#pragma once
#include <QObject>

#include "DataStructs.h"
#include "ImageProcessor.h"

class Processor : public QObject
{
	Q_OBJECT

		bool stopAfterCurrent = false;

	void processWorker(const ProcessingParcel& parcel);
	static bool read(const ProcessingItem& item, QList<uint16_t>& imageBuffer, QList<uint16_t>& referenceBuffer);
	static void save(const ProcessingItem& item, const SavingOptions& savingOptions, const QString& sourceFilesRoot, const QList<uint16_t>& imageBuffer);
	static ImageProcessor* getImageProcessor(Metadata::RawTypeEnum rawType);

signals:
	void signalProcessingStarted(int total);
	void signalProcessingFinished();
	void signalProcessingProgressChanged(int progress);

public:
	void process(const ProcessingParcel& parcel);
	void stopProcessing();
};