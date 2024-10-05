#include "QFile"
#include "QIODevice"
#include <QJsonObject>
#include "QJsonDocument"
#include "Settings.h"

float Settings::getDefaultIfNotInRange(const float value, const float minValue, const float maxValue, const float defaultValue)
{
	return value >= minValue && value <= maxValue ? value : defaultValue;
}

bool Settings::isReferenceFileMatcherMaxAllowedFocalLengthDifferencePercentsValid(const float difference)
{
	return difference >= 0 && difference <= maxFocalLengthDifferencePercent;
}

bool Settings::isReferenceFileMatcherMaxAllowedFNumberDifferenceStopsValid(const float difference)
{
	return difference >= 0 && difference <= maxFNumberDifferenceStops;
}

bool Settings::isProcessingIntensityValid(const float intensity)
{
	return intensity >= 0 && intensity <= maxCorrectionIntensity;
}

bool Settings::isGaussianBlurSigmaValid(const float sigma)
{
	return sigma >= 0 && sigma <= maxGaussianBlurRadius;
}

Settings::Settings()
{
	if (!QFile::exists(fileName))
	{
        referenceMatcherOptions.allowedFocalLengthDifferencePercents = defaultFocalLengthDifferencePercent;
        referenceMatcherOptions.allowedFNumberDifferenceStops = defaultFNumberDifferenceStops;
        defaultFileProcessingOptions.luminanceCorrectionIntensity = defaultCorrectionIntensity;
        defaultFileProcessingOptions.colorCorrectionIntensity = defaultCorrectionIntensity;
        defaultFileProcessingOptions.gaussianBlurSigma = defaultGaussianBlurRadius;

        return;
	}

	try
	{
		QFile settingsFile(fileName);
		settingsFile.open(QIODevice::ReadOnly);

		const QJsonDocument jsonDocument = QJsonDocument::fromJson(settingsFile.readAll());

		settingsFile.close();

		referenceMatcherOptions.referenceFilesRoot = jsonDocument["referenceFilesRoot"].toString();
		referenceMatcherOptions.allowedFocalLengthDifferencePercents = getDefaultIfNotInRange(jsonDocument["referenceFileMatcherMaxAllowedFocalLengthDifferencePercents"].toDouble(), 0, maxFocalLengthDifferencePercent, defaultFocalLengthDifferencePercent);
		referenceMatcherOptions.allowedFNumberDifferenceStops = getDefaultIfNotInRange(jsonDocument["referenceFileMatcherMaxAllowedFNumberDifferenceStops"].toDouble(), 0, maxFNumberDifferenceStops, defaultFNumberDifferenceStops);
		referenceMatcherOptions.ignoreFocalLength = jsonDocument["referenceFileMatcherIgnoreFocalLength"].toBool();
		referenceMatcherOptions.ignoreFNumber = jsonDocument["referenceFileMatcherIgnoreFNumber"].toBool();
		referenceMatcherOptions.ignoreLensTag = jsonDocument["referenceFileMatcherIgnoreLensTag"].toBool();

		sourceFilesRoot = jsonDocument["sourceFilesRoot"].toString();
		sourceFilesRecurseSubfolders = jsonDocument["sourceFilesRecurseSubfolders"].toBool();

		defaultFileProcessingOptions.luminanceCorrectionIntensity = getDefaultIfNotInRange(jsonDocument["processingDefaultLuminanceCorrectionIntensity"].toDouble(), 0, maxCorrectionIntensity, defaultCorrectionIntensity);
		defaultFileProcessingOptions.colorCorrectionIntensity = getDefaultIfNotInRange(jsonDocument["processingDefaultColorCorrectionIntensity"].toDouble(), 0, maxCorrectionIntensity, defaultCorrectionIntensity);
		defaultFileProcessingOptions.gaussianBlurSigma = getDefaultIfNotInRange(jsonDocument["processingDefaultGaussianBlurSigma"].toDouble(), 0, maxGaussianBlurRadius, defaultGaussianBlurRadius);

		globalProcessingOptions.limitToWhiteLevel = jsonDocument["processingLimitToWhiteLevel"].toBool();
		globalProcessingOptions.scaleChannelsToAvoidClipping = jsonDocument["processingScaleChannelsToAvoidClipping"].toBool();
		globalProcessingOptions.calculateCommonScaleForBatch = jsonDocument["processingCalculateCommonScaleForBatch"].toBool();

		savingOptions.saveTo = (SavingOptions::SaveToEnum)jsonDocument["saveTo"].toInt();
		savingOptions.saveToFolderPath = jsonDocument["saveProcessedFilesToFolderPath"].toString();
		savingOptions.saveToSubfolderFolderName = jsonDocument["saveProcessedFilesToSubfolderFolderName"].toString();
		lastUsedFolderForOneReferenceFileMode = jsonDocument["lastUsedFolderForOneReferenceFileMode"].toString();

		windowIsMaximized = jsonDocument["windowIsMaximized"].toBool();
		windowHeight = jsonDocument["windowHeight"].toInt();
		windowWidth = jsonDocument["windowWidth"].toInt();
		windowPositionX = jsonDocument["windowPositionX"].toInt();
		windowPositionY = jsonDocument["windowPositionY"].toInt();
	}
	catch (...)
	{
	}
}

Settings::~Settings()
{
	QJsonObject jsonObject;
	jsonObject.insert("referenceFilesRoot", "1");
	jsonObject["referenceFilesRoot"] = "2";
	jsonObject["referenceFilesRoot"] = referenceMatcherOptions.referenceFilesRoot;
	jsonObject["referenceFileMatcherMaxAllowedFocalLengthDifferencePercents"] = referenceMatcherOptions.allowedFocalLengthDifferencePercents;
	jsonObject["referenceFileMatcherMaxAllowedFNumberDifferenceStops"] = referenceMatcherOptions.allowedFNumberDifferenceStops;
	jsonObject["referenceFileMatcherIgnoreFocalLength"] = referenceMatcherOptions.ignoreFocalLength;
	jsonObject["referenceFileMatcherIgnoreFNumber"] = referenceMatcherOptions.ignoreFNumber;
	jsonObject["referenceFileMatcherIgnoreLensTag"] = referenceMatcherOptions.ignoreLensTag;

	jsonObject["sourceFilesRoot"] = sourceFilesRoot;
	jsonObject["sourceFilesRecurseSubfolders"] = sourceFilesRecurseSubfolders;

	jsonObject["processingDefaultLuminanceCorrectionIntensity"] = defaultFileProcessingOptions.luminanceCorrectionIntensity;
	jsonObject["processingDefaultColorCorrectionIntensity"] = defaultFileProcessingOptions.colorCorrectionIntensity;
	jsonObject["processingDefaultGaussianBlurSigma"] = defaultFileProcessingOptions.gaussianBlurSigma;

	jsonObject["processingLimitToWhiteLevel"] = globalProcessingOptions.limitToWhiteLevel;
	jsonObject["processingScaleChannelsToAvoidClipping"] = globalProcessingOptions.scaleChannelsToAvoidClipping;
	jsonObject["processingCalculateCommonScaleForBatch"] = globalProcessingOptions.calculateCommonScaleForBatch;

	jsonObject["saveTo"] = static_cast<int>(savingOptions.saveTo);
	jsonObject["saveProcessedFilesToFolderPath"] = savingOptions.saveToFolderPath;
	jsonObject["saveProcessedFilesToSubfolderFolderName"] = savingOptions.saveToSubfolderFolderName;
	jsonObject["lastUsedFolderForOneReferenceFileMode"] = lastUsedFolderForOneReferenceFileMode;

	jsonObject["windowIsMaximized"] = windowIsMaximized;
	jsonObject["windowHeight"] = windowHeight;
	jsonObject["windowWidth"] = windowWidth;
	jsonObject["windowPositionX"] = windowPositionX;
	jsonObject["windowPositionY"] = windowPositionY;

	try
	{
		QFile settingsFile(fileName);
		settingsFile.open(QIODevice::WriteOnly);
		settingsFile.write(QJsonDocument(jsonObject).toJson());
		settingsFile.close();
	}
	catch (...)
	{
	}
}