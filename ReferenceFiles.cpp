#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDirIterator>
#include "ReferenceFiles.h"
#include <qthread.h>

#include "MetadataReader.h"
#include "FileUtils.h"

bool ReferenceFiles::isReferenceFileCompatibleByFocalLength(float sourceFileFocalLength, float referenceFileFocalLength, const ReferenceMatcherOptions& options)
{
	const float halfDelta = sourceFileFocalLength / 200 * options.allowedFocalLengthDifferencePercents;
	return referenceFileFocalLength >= sourceFileFocalLength - halfDelta && referenceFileFocalLength <= sourceFileFocalLength + halfDelta;
}

bool ReferenceFiles::isReferenceFileCompatibleByFNumber(float sourceFileFNumber, float referenceFileFNumber, const ReferenceMatcherOptions& options)
{
	return abs(log10f(powf(referenceFileFNumber / sourceFileFNumber, 2)) / log10f(2)) <= options.allowedFNumberDifferenceStops;
}

bool ReferenceFiles::isReferenceCompatibleWithSource(const QSharedPointer<Metadata>& sourceMetadata, const QSharedPointer<Metadata>& referenceMetadata, const ReferenceMatcherOptions& options)
{
	return referenceMetadata->cameraMaker == sourceMetadata->cameraMaker &&
		referenceMetadata->cameraModel == sourceMetadata->cameraModel &&
		(areFilesCompatibleByLensTag(referenceMetadata, sourceMetadata, options)) &&
		referenceMetadata->imageWidth == sourceMetadata->imageWidth &&
		referenceMetadata->imageHeight == sourceMetadata->imageHeight &&
		referenceMetadata->rawType == sourceMetadata->rawType &&
		(options.ignoreFocalLength || isReferenceFileCompatibleByFocalLength(referenceMetadata->focalLength, sourceMetadata->focalLength, options)) &&
		(options.ignoreFNumber || isReferenceFileCompatibleByFNumber(referenceMetadata->fNumber, sourceMetadata->fNumber, options));
}

bool ReferenceFiles::areFilesCompatibleByLensTag(const QSharedPointer<Metadata>& referenceMetadata, const QSharedPointer<Metadata>& sourceMetadata, const ReferenceMatcherOptions& options)
{
	if (options.ignoreLensTag)
	{
		return true;
	}

	if (referenceMetadata->lens == nullptr && sourceMetadata->lens == nullptr)
	{
		return true;
	}

	if (referenceMetadata->lens == sourceMetadata->lens)
	{
		return true;
	}
	return false;
}


void ReferenceFiles::createDB(const QString& referenceFilesRoot)
{
    if (referenceFilesRoot.isEmpty() || !QDir(referenceFilesRoot).exists())
	{
		return;
	}

    emit signalRebuildingDBStarted(0);

	db.clear();

	QList<QString> files;

	QDirIterator iterator(referenceFilesRoot, { "*.dng" }, QDir::Files, QDirIterator::Subdirectories);
	while (iterator.hasNext())
	{
		files.append(iterator.next());
	}

	emit signalRebuildingDBStarted(files.size());
	for (int i = 0; i < files.size(); i++)
	{
		const QString filePath = files[i];
		QSharedPointer<Metadata> metadata = MetadataReader::readMetadata(filePath);

		if (metadata != nullptr)
		{
			db.insert(filePath, QSharedPointer<FileInfo>(new FileInfo(filePath, metadata)));
		}

		emit signalRebuildingDBProgressChanged(i + 1);
	}

	save(referenceFilesRoot);

	emit signalRebuildingDBFinished();
	emit signalDBSizeChanged(db.size());
}

void ReferenceFiles::load(const QString& referenceFilesRoot)
{
	if (!QDir(referenceFilesRoot).exists())
	{
		return;
	}

	db.clear();

	const QString dbFilePath = FileUtils::getAbsolutePath(referenceFilesRoot, fileName);
	if (!QFile::exists(dbFilePath))
	{
		return;
	}

	try
	{
		QFile referenceDBFile(dbFilePath);
		referenceDBFile.open(QIODevice::ReadOnly);
		const QJsonDocument jsonDocument = QJsonDocument::fromJson(referenceDBFile.readAll());
		referenceDBFile.close();

		const QJsonArray jsonArray = jsonDocument.array();
		for (int i = 0; i < jsonArray.size(); i++)
		{
			QJsonObject jsonObject = jsonArray[i].toObject();

			QString filePath = FileUtils::getAbsolutePath(referenceFilesRoot, jsonObject["filePath"].toString());

			QSharedPointer<Metadata> metadata = QSharedPointer<Metadata>(new Metadata);
			QJsonObject jsonMetadataObject = jsonObject["metadata"].toObject();

			metadata->cameraMaker = jsonMetadataObject["cameraMaker"].toString();
			metadata->cameraModel = jsonMetadataObject["cameraModel"].toString();
			metadata->lens = jsonMetadataObject["lens"].toString();
			metadata->fNumber = jsonMetadataObject["fNumber"].toDouble();
			metadata->focalLength = jsonMetadataObject["focalLength"].toDouble();
			metadata->rawType = static_cast<Metadata::RawTypeEnum>(jsonMetadataObject["rawType"].toInt());
			metadata->imageHeight = jsonMetadataObject["imageHeight"].toInt();
			metadata->imageWidth = jsonMetadataObject["imageWidth"].toInt();
			metadata->dataOffset = jsonMetadataObject["dataOffset"].toInt();
			metadata->dataSize = jsonMetadataObject["dataSize"].toInt();

			QJsonArray activeArea = jsonMetadataObject["activeArea"].toArray();
			metadata->activeArea.resize(activeArea.size());
			for (int j = 0; j < activeArea.size(); j++)
			{
				metadata->activeArea[j] = activeArea[j].toInt();
			}

			QJsonArray blackLevels = jsonMetadataObject["blackLevels"].toArray();
			metadata->blackLevels.resize(blackLevels.size());
			for (int j = 0; j < blackLevels.size(); j++)
			{
				metadata->blackLevels[j] = blackLevels[j].toInt();
			}

			QJsonArray whiteLevels = jsonMetadataObject["whiteLevels"].toArray();
			metadata->whiteLevels.resize(whiteLevels.size());
			for (int j = 0; j < whiteLevels.size(); j++)
			{
				metadata->whiteLevels[j] = whiteLevels[j].toInt();
			}

			QJsonArray colorPattern = jsonMetadataObject["colorPattern"].toArray();
			metadata->cfaColorPattern.resize(colorPattern.size());
			for (int j = 0; j < colorPattern.size(); j++)
			{
				metadata->cfaColorPattern[j] = static_cast<Metadata::CFAPatternEnum>(colorPattern[j].toInt());
			}

			db.insert(filePath, QSharedPointer<FileInfo>(new FileInfo(filePath, metadata)));
		}
	}
	catch (...)
	{
		return;
	}

	emit signalDBSizeChanged(db.size());
}

void ReferenceFiles::save(const QString& referenceFilesRoot) const
{
	if (!QDir(referenceFilesRoot).exists())
	{
		return;
	}

	QJsonArray jsonArray;

	for (auto [key, fileInfo] : db.asKeyValueRange())
	{
		if (!fileInfo)
		{
			continue;
		}


		QJsonObject jsonObject;
		jsonObject["filePath"] = FileUtils::getRelativePath(referenceFilesRoot, fileInfo->filePath);

		QJsonObject jsonObjectMetadata;
		jsonObjectMetadata["cameraMaker"] = fileInfo->metadata->cameraMaker;
		jsonObjectMetadata["cameraModel"] = fileInfo->metadata->cameraModel;
		jsonObjectMetadata["lens"] = fileInfo->metadata->lens;
		jsonObjectMetadata["fNumber"] = fileInfo->metadata->fNumber;
		jsonObjectMetadata["focalLength"] = fileInfo->metadata->focalLength;
		jsonObjectMetadata["rawType"] = fileInfo->metadata->rawType;
		jsonObjectMetadata["imageHeight"] = fileInfo->metadata->imageHeight;
		jsonObjectMetadata["imageWidth"] = fileInfo->metadata->imageWidth;
		jsonObjectMetadata["dataOffset"] = fileInfo->metadata->dataOffset;
		jsonObjectMetadata["dataSize"] = fileInfo->metadata->dataSize;

		QJsonArray activeArea;
		for (int j = 0; j < fileInfo->metadata->activeArea.length(); j++)
		{
			activeArea.append(fileInfo->metadata->activeArea[j]);
		}
		jsonObjectMetadata["activeArea"] = activeArea;

		if (fileInfo->metadata->blackLevels.length() != 0)
		{
			QJsonArray blackLevels;
			for (int j = 0; j < fileInfo->metadata->blackLevels.length(); j++)
			{
				blackLevels.append(fileInfo->metadata->blackLevels[j]);
			}
			jsonObjectMetadata["blackLevels"] = blackLevels;
		}


		QJsonArray whiteLevels;
		for (int j = 0; j < fileInfo->metadata->whiteLevels.length(); j++)
		{
			whiteLevels.append(fileInfo->metadata->whiteLevels[j]);
		}
		jsonObjectMetadata["whiteLevels"] = whiteLevels;

		QJsonArray colorPattern;
		for (int j = 0; j < fileInfo->metadata->cfaColorPattern.length(); j++)
		{
			colorPattern.append(fileInfo->metadata->cfaColorPattern[j]);
		}
		jsonObjectMetadata["colorPattern"] = colorPattern;

		jsonObject["metadata"] = jsonObjectMetadata;

		jsonArray.append(jsonObject);
	}

	try
	{
		const QString dbFilePath = QDir(referenceFilesRoot).filePath(fileName);
		QFile settingsFile(dbFilePath);
		settingsFile.open(QIODevice::WriteOnly);
		settingsFile.write(QJsonDocument(jsonArray).toJson());
		settingsFile.close();
	}
	catch (...)
	{
	}
}

QList<QSharedPointer<FileInfo>> ReferenceFiles::findMatchingReferenceFiles(const QSharedPointer<Metadata>& sourceMetadata, const ReferenceMatcherOptions& options) const
{
	QList<QSharedPointer<FileInfo>> result;
	for (auto [key, referenceFileInfoOptional] : db.asKeyValueRange())
	{
		if (!referenceFileInfoOptional)
		{
			continue;
		}

		QSharedPointer<FileInfo> referenceFileInfo = referenceFileInfoOptional;


		if (isReferenceCompatibleWithSource(sourceMetadata, referenceFileInfo->metadata, options))
		{
			result.append(referenceFileInfo);
		}
	}
	return result;
}

QList<QSharedPointer<FileInfo>> ReferenceFiles::getCommonReferenceFiles(const QList<QSharedPointer<SourceFileInfo>>& sourceFiles, const ReferenceMatcherOptions& options)
{
	QList<QSharedPointer<FileInfo>> result;
	if (sourceFiles.isEmpty())
	{
		return result;
	}

	if (sourceFiles.size() == 1)
	{
		return sourceFiles[0]->referenceFiles;
	}

	QSet< QSharedPointer<FileInfo>> uniquenessWatch;

	for (int sourceFilesIndex = 0; sourceFilesIndex < sourceFiles.size(); sourceFilesIndex++)
	{
		for (int referenceFilesIndex = 0; referenceFilesIndex < sourceFiles[sourceFilesIndex]->referenceFiles.size(); referenceFilesIndex++)
		{
			if (uniquenessWatch.contains(sourceFiles[sourceFilesIndex]->referenceFiles[referenceFilesIndex]))
			{
				continue;
			}

			bool isCompatible = true;
			for (int i = 0; i < sourceFiles.size(); i++)
			{
				isCompatible &= isReferenceCompatibleWithSource(sourceFiles[i]->sourceFile->metadata, sourceFiles[sourceFilesIndex]->referenceFiles[referenceFilesIndex]->metadata, options);
			}

			if (isCompatible)
			{
				result.append(sourceFiles[sourceFilesIndex]->referenceFiles[referenceFilesIndex]);
				uniquenessWatch.insert(sourceFiles[sourceFilesIndex]->referenceFiles[referenceFilesIndex]);
			}
		}
	}

	return result;
}

QSharedPointer<FileInfo> ReferenceFiles::getFileMetadata(const QString& filePath) const
{
	return db[filePath];
}
