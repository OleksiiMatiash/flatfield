#pragma once
#include <QList>
#include <QSharedPointer>

struct Metadata
{
	enum RawTypeEnum
	{
		Unknown = 0,
		Mono = 1,
		RGB = 3,
		Bayer = 4
	};

	enum CFAPatternEnum
	{
		Red,
		Green,
		Blue
	};

	QString cameraMaker = "";
	QString cameraModel = "";
	QString lens = "";
	float focalLength = 0;
	float fNumber = 0;
	RawTypeEnum rawType = RawTypeEnum::Unknown;
	int imageHeight = 0;
	int imageWidth = 0;
	QList<int> activeArea;
	QList<uint16_t> blackLevels;
	QList<uint16_t> whiteLevels;
	QList<CFAPatternEnum> cfaColorPattern;
	int dataOffset = 0;
	int dataSize = 0;

	int getChannelsCount() const
	{
		return rawType;
	}

	bool isValid() const
	{
		return !cameraMaker.isEmpty() &&
			!cameraModel.isEmpty() &&
			(rawType == RGB || rawType == Bayer && !cfaColorPattern.isEmpty() || rawType == Mono) &&
			imageHeight != 0 &&
			imageWidth != 0 &&
			!activeArea.isEmpty() &&
			dataOffset != 0 &&
			dataSize != 0;
	}

	static bool isCompatible(const QSharedPointer<Metadata>& firstMetadata, const QSharedPointer<Metadata>& secondMetadata)
	{
		return firstMetadata->cameraMaker == secondMetadata->cameraMaker &&
			firstMetadata->cameraModel == secondMetadata->cameraModel &&
			firstMetadata->rawType == secondMetadata->rawType &&
			firstMetadata->imageHeight == secondMetadata->imageHeight &&
			firstMetadata->imageWidth == secondMetadata->imageWidth &&
			firstMetadata->activeArea[2] - firstMetadata->activeArea[0] == secondMetadata->activeArea[2] - secondMetadata->activeArea[0] &&
			firstMetadata->activeArea[3] - firstMetadata->activeArea[1] == secondMetadata->activeArea[3] - secondMetadata->activeArea[1];
	}


	QString toString()
	{
		return QString("%1 %2, %3, %4 mm @ F%5").arg(
			cameraMaker.isEmpty() ? QString("<no camera maker info>") : cameraMaker,
			cameraModel.isEmpty() ? QString("<no camera model info>") : cameraModel,
			lens.isEmpty() ? QString("<no lens info>") : lens,
			QString::number(focalLength),
			QString::number(fNumber));
	}
};

struct FileInfo
{
	QString filePath;
	QSharedPointer<Metadata> metadata;

	FileInfo()
	{
		filePath = "";
	}

	FileInfo(const QString& filePath, const QSharedPointer<Metadata>& metadata)
	{
		this->filePath = filePath;
		this->metadata = metadata;
	}
};

struct GlobalProcessingOptions
{
	bool limitToWhiteLevel = false;
	bool scaleChannelsToAvoidClipping = false;
	bool calculateCommonScaleForBatch = false;
};

struct ProcessingOptions
{
	float luminanceCorrectionIntensity = 0;
	float colorCorrectionIntensity = 0;
	float gaussianBlurSigma = 0;

	static bool areFloatValuesEqual(float value1, float value2)
	{
		return static_cast<int>(value1 * 1000) == static_cast<int>(value2 * 1000);
	}
};

struct SavingOptions
{
	enum SaveToEnum
	{
		Folder,
		Subfolder
	};

	SaveToEnum saveTo = SaveToEnum::Subfolder;
	QString saveToFolderPath;
	QString saveToSubfolderFolderName = "out";
};

struct ProcessingItem
{
	QSharedPointer<FileInfo> sourceFile;
	QSharedPointer<FileInfo> referenceFile;
	ProcessingOptions processingOptions;

	ProcessingItem(QSharedPointer<FileInfo> sourceFile, QSharedPointer<FileInfo> referenceFile, ProcessingOptions processingOptions)
	{
		this->sourceFile = sourceFile;
		this->referenceFile = referenceFile;
		this->processingOptions = processingOptions;
	}
};


struct ProcessingParcel
{
	QList<ProcessingItem> items;
	QString sourceFileRoot;
	GlobalProcessingOptions globalProcessingOptions;
	SavingOptions savingOptions;

	ProcessingParcel(const QList<ProcessingItem>& items, const QString& sourceFileRoot, const GlobalProcessingOptions& globalProcessingOptions, const SavingOptions& savingOptions)
	{
		this->items = items;
		this->sourceFileRoot = sourceFileRoot;
		this->globalProcessingOptions = globalProcessingOptions;
		this->savingOptions = savingOptions;
	}
};

struct ReferenceMatcherOptions
{
	QString referenceFilesRoot;
	float allowedFocalLengthDifferencePercents;
	float allowedFNumberDifferenceStops;
	bool ignoreFocalLength = false;
	bool ignoreFNumber = false;
	bool ignoreLensTag = false;
};

struct TwoPassProcessingState
{
	float commonScaleForBatch = 1;
	bool performBatchScale = false;
};

class SourceFileInfo
{
public:
	QSharedPointer<FileInfo> sourceFile;
	QSharedPointer<FileInfo> activeReferenceFile;
	QList<QSharedPointer<FileInfo>> referenceFiles;
	ProcessingOptions processingOptions;


	SourceFileInfo(QString filePath, QSharedPointer<Metadata> metadata, const ProcessingOptions& defaultProcessingOptions)
	{
		this->sourceFile = QSharedPointer<FileInfo>(new FileInfo(filePath, metadata));
		this->processingOptions.luminanceCorrectionIntensity = defaultProcessingOptions.luminanceCorrectionIntensity;
		this->processingOptions.colorCorrectionIntensity = defaultProcessingOptions.colorCorrectionIntensity;
		this->processingOptions.gaussianBlurSigma = defaultProcessingOptions.gaussianBlurSigma;
	}
};