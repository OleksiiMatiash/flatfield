#include <QDir>
#include "FileUtils.h"

QString FileUtils::getRelativePath(const QString& rootFolder, const QString& absolutePath)
{
	return QDir(rootFolder).relativeFilePath(absolutePath);
}

QString FileUtils::getAbsolutePath(const QString& rootFolder, const QString& relativePath)
{
	return QDir(rootFolder).filePath(relativePath);
}

QString FileUtils::createDestinationFileInDestinationFolder(const QString& sourceFilePath, const QString& sourceFilesRoot, const SavingOptions& savingOptions)
{
	const QDir destinationFolder(savingOptions.saveToFolderPath);
	if (!destinationFolder.exists())
	{
		return "";
	}

	QString destinationFilePath = destinationFolder.absolutePath() + QDir::separator() + getRelativePath(sourceFilesRoot, sourceFilePath);

	if (QFileInfo(destinationFilePath).exists())
	{
		QFile::remove(destinationFilePath);
	}

	if (QFile::copy(sourceFilePath, destinationFilePath))
	{
		return destinationFilePath;
	}

	return "";
}

QString FileUtils::createDestinationFileInSubfolder(const QString& sourceFilePath, const SavingOptions& savingOptions)
{
	const QFileInfo fileInfo(sourceFilePath);
	const QString destinationFolder = fileInfo.dir().path() + QDir::separator() + savingOptions.saveToSubfolderFolderName;
	QString destinationFilePath = destinationFolder + QDir::separator() + fileInfo.fileName();

	QDir().mkdir(destinationFolder);

	if (QFileInfo(destinationFilePath).exists())
	{
		QFile::remove(destinationFilePath);
	}

	if (QFile::copy(sourceFilePath, destinationFilePath))
	{
		return destinationFilePath;
	}

	return "";
}

bool FileUtils::isFileFromOutputSubfolder(const QString& sourceFilePath, const SavingOptions& savingOptions)
{
	QStringList pathSegments = QDir::toNativeSeparators(QFileInfo(sourceFilePath).dir().path()).split(QDir::separator());
	return pathSegments[pathSegments.size() - 1] == savingOptions.saveToSubfolderFolderName;
}
