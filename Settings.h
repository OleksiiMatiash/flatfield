#pragma once
#include <QObject>

#include "DataStructs.h"


class Settings : public QObject
{
	Q_OBJECT

	static constexpr float defaultFocalLengthDifferencePercent = 5;
	static constexpr float defaultFNumberDifferenceStops = 0.5;
	static constexpr float defaultCorrectionIntensity = 1.0;
	static constexpr float defaultGaussianBlurRadius = 50;

	QString fileName = "settings.json";

	static float getDefaultIfNotInRange(float value, float minValue, float maxValue, float defaultValue);

public:
	QString sourceFilesRoot;
	static constexpr float maxFocalLengthDifferencePercent = 100;
	static constexpr float maxFNumberDifferenceStops = 100;
	static constexpr float maxCorrectionIntensity = 1;
	static constexpr float maxGaussianBlurRadius = 1000;
	static constexpr int minWindowHeight = 600;
	static constexpr int minWindowWidth = 1000;

	ReferenceMatcherOptions referenceMatcherOptions;
	ProcessingOptions defaultFileProcessingOptions;
	GlobalProcessingOptions globalProcessingOptions;
	SavingOptions savingOptions;

	bool sourceFilesRecurseSubfolders = true;
	QString lastUsedFolderForOneReferenceFileMode = "";

	bool windowIsMaximized = false;
	int windowPositionX = -1;
	int windowPositionY = -1;
	int windowHeight = minWindowHeight;
	int windowWidth = minWindowWidth;

	Settings();
	~Settings() override;

	static bool isReferenceFileMatcherMaxAllowedFocalLengthDifferencePercentsValid(float difference);
	static bool isReferenceFileMatcherMaxAllowedFNumberDifferenceStopsValid(float difference);
	static bool isProcessingIntensityValid(float intensity);
	static bool isGaussianBlurSigmaValid(float sigma);
};
