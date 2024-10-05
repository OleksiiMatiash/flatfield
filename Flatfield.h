#pragma once
#include <QStandardItemModel>
#include "Processor.h"
#include "ReferenceFiles.h"
#include "Settings.h"
#include "ui_flatfield.h"

class Flatfield : public QMainWindow
{
	Q_OBJECT

		Ui::FlatfieldClass ui;

	QList<QSharedPointer<SourceFileInfo>> sourceFiles;
	QMap<QString, QSharedPointer<SourceFileInfo>> pathToSourceFileMap;

	Settings settings;
	Processor* processor;
	ReferenceFiles referenceFiles;
	QStandardItemModel* sourceFilesModel = new QStandardItemModel(0, 2, this);
	QStandardItemModel* referenceFilesModel = new QStandardItemModel(0, 3, this);

	static QColor getSourceFileItemColor(const QSharedPointer<SourceFileInfo>& sourceFileInfo);

	void setupUI();
	void connectSignalsToSlots();
	void setSaveToViewsState() const;

	QList<QSharedPointer<SourceFileInfo>> getSelectedSourceFiles() const;
	QList<QSharedPointer<SourceFileInfo>> getAllReadySourceFiles() const;
	QList<QSharedPointer<SourceFileInfo>> getSelectedReadySourceFiles() const;
	QSharedPointer<FileInfo> getSelectedReferenceFile() const;
	bool areSelectedFilesCompatible() const;
	void setProcessButtonsVisibility(bool isVisible) const;
	void updateProcessButtonsState() const;
	static QString getReferenceFilesCountText(int count);
	void rematch();
	void start() const;
	void fillSourceList() const;
	void colorSourceList() const;
	void setReferenceFile(const QSharedPointer<FileInfo>& selectedReferenceFile) const;
	void fillReferenceList(const QList<QSharedPointer<SourceFileInfo>>& selectedSourceFiles) const;
	void markReferenceList(const QList<QSharedPointer<SourceFileInfo>>& selectedSourceFiles, const QList<QSharedPointer<FileInfo>>& commonReferenceFiles) const;
	static QSharedPointer<FileInfo> getCommonActiveReferenceFile(const QList<QSharedPointer<SourceFileInfo>>& selectedSourceFiles);
	void setUIState(bool isEnabled) const;
	void prepareProcessingParcel(const QList<QSharedPointer<SourceFileInfo>>& files) const;
	void colorCalculateCommonBatchScaleCheckbox() const;

signals:
	void signalProcessingStarted(int total);
	void signalProcessingFinished();
	void signalProcessingProgressChanged(int progress);

private slots:
	void slotSourceFilesSelectRootClicked();
	void slotSourceFilesRecurseSubfoldersChanged(bool recurse);
	void slotSourceFileSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) const;

	void slotReferenceFilesSelectRootClicked();
	void slotReferenceFilesRebuildDBClicked();
	void slotReferenceFileSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) const;
	void slotReferenceFilesClickOnTableEmptySpace() const;

	void slotReferenceFileMatcherMaxAllowedFocalLengthDifferenceEditingFinished() const;
	void slotReferenceFileMatcherMaxAllowedFNumberDifferenceEditingFinished() const;
	void slotReferenceFileMatcherMaxAllowedFocalLengthDifferencePercentsChanged(const QString& differenceString);
	void slotReferenceFileMatcherIgnoreFocalLengthChanged(bool ignore);
	void slotReferenceFileMatcherMaxAllowedFNumberDifferenceStopsChanged(const QString& differenceString);
	void slotReferenceFileMatcherIgnoreFNumberChanged(bool ignore);
	void slotReferenceFileMatcherIgnoreLensTagChanged(bool ignore);
	void slotReferenceFileMatcherRematchClicked();

	void slotProcessingLuminanceCorrectionIntensityChanged(const QString& intensityString) const;
	void slotProcessingLuminanceCorrectionIntensityEditingFinished() const;
	void slotProcessingColorCorrectionIntensityChanged(const QString& intensityString) const;
	void slotProcessingColorCorrectionIntensityEditingFinished() const;
	void slotProcessingGaussianBlurSigmaChanged(const QString& sigmaString) const;
	void slotProcessingGaussianBlurSigmaEditingFinished() const;
	void slotProcessingLimitToWhiteLevelChanged(bool limit);
	void slotProcessingScaleChannelsToAvoidClipping(bool scale);
	void slotProcessingCalculateCommonScaleForBatch(bool calculate);

	void slotSaveToFolderRadioButtonClicked();
	void slotSaveToSubfolderRadioButtonClicked();
	void slotSaveToOutputFolderClicked();
	void slotSaveProcessedFilesToFolderPathChanged(const QString& path);
	void slotSaveProcessedFilesToSubfolderFolderNameChanged(const QString& name);
	void slotProcessAllReadyClicked() const;
	void slotProcessAllReadySelectedClicked() const;
	void slotProcessSelectedWithOneReferenceClicked() const;
	void slotCancelClicked() const;

	void slotProcessingStarted(int total) const;
	void slotProcessingFinished() const;
	void slotProcessingGlobalProgressChanged(int progress) const;
	void slotReferenceDBRebuildingStarted(int total) const;
	void slotReferenceDBRebuildingProgressChanged(int progress) const;
	void slotReferenceDBRebuildingFinished() const;
	void slotReferenceFilesDBSizeChanged(int count);

public:
	Flatfield(QWidget* parent = nullptr);
	~Flatfield() override;
};