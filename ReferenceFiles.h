#pragma once
#include <QObject>
#include <QString>
#include <QMap>

#include "DataStructs.h"

class ReferenceFiles : public QObject
{
	Q_OBJECT

		inline static const QString fileName = "referencesDB.json";
	QMap<QString, QSharedPointer<FileInfo>> db;

	static bool isReferenceFileCompatibleByFocalLength(float sourceFileFocalLength, float referenceFileFocalLength, const ReferenceMatcherOptions& options);
	static bool isReferenceFileCompatibleByFNumber(float sourceFileFNumber, float referenceFileFNumber, const ReferenceMatcherOptions& options);
	static bool isReferenceCompatibleWithSource(const QSharedPointer<Metadata>& sourceMetadata, const QSharedPointer<Metadata>& referenceMetadata, const ReferenceMatcherOptions& options);
	static bool areFilesCompatibleByLensTag(const QSharedPointer<Metadata>& referenceMetadata, const QSharedPointer<Metadata>& sourceMetadata, const ReferenceMatcherOptions& options);

signals:
	void signalRebuildingDBStarted(int total);
	void signalRebuildingDBProgressChanged(int current);
	void signalRebuildingDBFinished();
	void signalDBSizeChanged(int count);

public:
	void createDB(const QString& referenceFilesRoot);
	void load(const QString& referenceFilesRoot);
	void save(const QString& referenceFilesRoot) const;
	QList<QSharedPointer<FileInfo>> findMatchingReferenceFiles(const QSharedPointer<Metadata>& sourceMetadata, const ReferenceMatcherOptions& options) const;
	static QList<QSharedPointer<FileInfo>> getCommonReferenceFiles(const QList<QSharedPointer<SourceFileInfo>>& sourceFiles, const ReferenceMatcherOptions& options);
	QSharedPointer<FileInfo> getFileMetadata(const QString& filePath) const;
};
