#pragma once
#include <QString>
#include "Settings.h"

class FileUtils
{
public:
	static QString getRelativePath(const QString& rootFolder, const QString& absolutePath);
	static QString getAbsolutePath(const QString& rootFolder, const QString& relativePath);
	static QString createDestinationFileInDestinationFolder(const QString& sourceFilePath, const QString& sourceFilesRoot, const SavingOptions& savingOptions);
	static QString createDestinationFileInSubfolder(const QString& sourceFilePath, const SavingOptions& savingOptions);
	static bool isFileFromOutputSubfolder(const QString& sourceFilePath, const SavingOptions& savingOptions);
};
