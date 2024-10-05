#include "QScreen"
#include "QFileDialog"
#include <QtCore/QtCore>
#include <QtConcurrent/QtConcurrentRun>

#include "Flatfield.h"
#include "FileUtils.h"
#include "LimitingDoubleValidator.h"
#include "MetadataReader.h"


Flatfield::Flatfield(QWidget* parent) : QMainWindow(parent)
{
	processor = new Processor();

	ui.setupUi(this);

	setupUI();
	connectSignalsToSlots();

	referenceFiles.load(settings.referenceMatcherOptions.referenceFilesRoot);
}

Flatfield::~Flatfield()
{
	settings.windowHeight = height();
	settings.windowWidth = width();
	settings.windowPositionX = x();
	settings.windowPositionY = y();
	settings.windowIsMaximized = ui.centralwidget->window()->isMaximized();

	delete processor;
}

void Flatfield::setupUI()
{
	if (settings.windowIsMaximized)
	{
		ui.centralwidget->window()->showMaximized();
	}
	else
	{
		resize(settings.windowWidth, settings.windowHeight);

		if (settings.windowPositionX < 0 || settings.windowPositionY < 0)
		{
			const QPoint center = QApplication::primaryScreen()->availableGeometry().center();
			move(center.x() - width() / 2, center.y() - height() / 2);
		}
		else
		{
			move(settings.windowPositionX, settings.windowPositionY);
		}
	}

	ui.lineEditReferenceFilesRoot->setStyleSheet(":disabled { color: black;}");
	ui.lineEditReferenceFilesFound->setStyleSheet(":disabled { color: black;}");
	ui.lineEditSourceFilesRoot->setStyleSheet(":disabled { color: black;}");
	ui.checkBoxSourceFilesRecurseSubfolders->setChecked(settings.sourceFilesRecurseSubfolders);
	ui.lineEditSourceFilesFound->setStyleSheet(":disabled { color: black;}");
	ui.tableViewSourceFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.tableViewReferenceFiles->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.lineEditMatcherFocalLengthMaxDifference->setValidator(new LimitingDoubleValidator(0, Settings::maxFocalLengthDifferencePercent, 1, this));
	ui.lineEditMatcherFNumberMaxDifference->setValidator(new LimitingDoubleValidator(0, Settings::maxFNumberDifferenceStops, 2, this));
	ui.lineEditProcessingLuminanceCorrectionIntensity->setEnabled(false);
	ui.lineEditProcessingLuminanceCorrectionIntensity->setValidator(new LimitingDoubleValidator(0, Settings::maxCorrectionIntensity, 2, this));
	ui.lineEditProcessingColorCorrectionIntensity->setEnabled(false);
	ui.lineEditProcessingColorCorrectionIntensity->setValidator(new LimitingDoubleValidator(0, Settings::maxCorrectionIntensity, 2, this));
	ui.lineEditProcessingGaussianBlurSigma->setEnabled(false);
	ui.lineEditProcessingGaussianBlurSigma->setValidator(new LimitingDoubleValidator(0, Settings::maxGaussianBlurRadius, 1, this));
	ui.lineEditSaveToFolderRoot->setStyleSheet(":disabled { color: black;}");
	ui.pushButtonStop->setVisible(false);

	ui.lineEditReferenceFilesRoot->setText(settings.referenceMatcherOptions.referenceFilesRoot);
	ui.lineEditMatcherFocalLengthMaxDifference->setText(QString::number(settings.referenceMatcherOptions.allowedFocalLengthDifferencePercents, 'g', 3));
	ui.lineEditMatcherFocalLengthMaxDifference->setEnabled(!settings.referenceMatcherOptions.ignoreFocalLength);
	ui.lineEditMatcherFNumberMaxDifference->setText(QString::number(settings.referenceMatcherOptions.allowedFNumberDifferenceStops, 'g', 3));
	ui.lineEditMatcherFNumberMaxDifference->setEnabled(!settings.referenceMatcherOptions.ignoreFNumber);
	ui.checkBoxMatcherFocalLengthIgnore->setChecked(settings.referenceMatcherOptions.ignoreFocalLength);
	ui.checkBoxMatcherFNumberIgnore->setChecked(settings.referenceMatcherOptions.ignoreFNumber);
	ui.checkBoxMatcherIgnoreExifTagLens->setChecked(settings.referenceMatcherOptions.ignoreLensTag);
	ui.checkBoxProcessingLimitToWhiteLevel->setChecked(settings.globalProcessingOptions.limitToWhiteLevel);
	ui.checkBoxProcessingScaleChannels->setChecked(settings.globalProcessingOptions.scaleChannelsToAvoidClipping);
	ui.checkBoxProcessingCalculateCommonScale->setChecked(settings.globalProcessingOptions.calculateCommonScaleForBatch);
	ui.lineEditSaveToFolderRoot->setText(settings.savingOptions.saveToFolderPath);
	ui.lineEditSaveToSubfolderName->setText(settings.savingOptions.saveToSubfolderFolderName);
	ui.tableViewSourceFiles->setModel(sourceFilesModel);
	ui.tableViewReferenceFiles->setModel(referenceFilesModel);

	colorCalculateCommonBatchScaleCheckbox();
	setSaveToViewsState();
}

void Flatfield::connectSignalsToSlots()
{
	connect(ui.pushButtonReferenceFilesSelectRoot, &QPushButton::clicked, this, &Flatfield::slotReferenceFilesSelectRootClicked);
	connect(ui.pushButtonReferenceFilesRebuildDB, &QPushButton::clicked, this, &Flatfield::slotReferenceFilesRebuildDBClicked);
	connect(ui.pushButtonSourceFilesSelectRoot, &QPushButton::clicked, this, &Flatfield::slotSourceFilesSelectRootClicked);
	connect(ui.tableViewSourceFiles->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Flatfield::slotSourceFileSelectionChanged);
	connect(ui.tableViewReferenceFiles->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Flatfield::slotReferenceFileSelectionChanged);
	connect(ui.tableViewReferenceFiles, &ReferenceTableView::clickOnEmptySpace, this, &Flatfield::slotReferenceFilesClickOnTableEmptySpace);

	connect(ui.lineEditMatcherFocalLengthMaxDifference, &QLineEdit::textChanged, this, &Flatfield::slotReferenceFileMatcherMaxAllowedFocalLengthDifferencePercentsChanged);
	connect(ui.lineEditMatcherFocalLengthMaxDifference, &QLineEdit::editingFinished, this, &Flatfield::slotReferenceFileMatcherMaxAllowedFocalLengthDifferenceEditingFinished);
	connect(ui.lineEditMatcherFNumberMaxDifference, &QLineEdit::textChanged, this, &Flatfield::slotReferenceFileMatcherMaxAllowedFNumberDifferenceStopsChanged);
	connect(ui.lineEditMatcherFNumberMaxDifference, &QLineEdit::editingFinished, this, &Flatfield::slotReferenceFileMatcherMaxAllowedFNumberDifferenceEditingFinished);
	connect(ui.pushButtonRematch, &QPushButton::clicked, this, &Flatfield::slotReferenceFileMatcherRematchClicked);

	connect(ui.lineEditProcessingLuminanceCorrectionIntensity, &QLineEdit::textChanged, this, &Flatfield::slotProcessingLuminanceCorrectionIntensityChanged);
	connect(ui.lineEditProcessingLuminanceCorrectionIntensity, &QLineEdit::editingFinished, this, &Flatfield::slotProcessingLuminanceCorrectionIntensityEditingFinished);
	connect(ui.lineEditProcessingColorCorrectionIntensity, &QLineEdit::textChanged, this, &Flatfield::slotProcessingColorCorrectionIntensityChanged);
	connect(ui.lineEditProcessingColorCorrectionIntensity, &QLineEdit::editingFinished, this, &Flatfield::slotProcessingColorCorrectionIntensityEditingFinished);
	connect(ui.lineEditProcessingGaussianBlurSigma, &QLineEdit::textChanged, this, &Flatfield::slotProcessingGaussianBlurSigmaChanged);
	connect(ui.lineEditProcessingGaussianBlurSigma, &QLineEdit::editingFinished, this, &Flatfield::slotProcessingGaussianBlurSigmaEditingFinished);
	connect(ui.checkBoxProcessingLimitToWhiteLevel, &QCheckBox::clicked, this, &Flatfield::slotProcessingLimitToWhiteLevelChanged);
	connect(ui.checkBoxProcessingScaleChannels, &QCheckBox::clicked, this, &Flatfield::slotProcessingScaleChannelsToAvoidClipping);
	connect(ui.checkBoxProcessingCalculateCommonScale, &QCheckBox::clicked, this, &Flatfield::slotProcessingCalculateCommonScaleForBatch);

	connect(ui.radioButtonSaveToFolder, &QRadioButton::clicked, this, &Flatfield::slotSaveToFolderRadioButtonClicked);
	connect(ui.radioButtonSaveToSubfolder, &QRadioButton::clicked, this, &Flatfield::slotSaveToSubfolderRadioButtonClicked);
	connect(ui.pushButtonSaveToFolderSelectFolder, &QPushButton::clicked, this, &Flatfield::slotSaveToOutputFolderClicked);
	connect(ui.pushButtonProcessReady, &QPushButton::clicked, this, &Flatfield::slotProcessAllReadyClicked);
	connect(ui.pushButtonProcessReadySelected, &QPushButton::clicked, this, &Flatfield::slotProcessAllReadySelectedClicked);
	connect(ui.pushButtonProcessWithOneReference, &QPushButton::clicked, this, &Flatfield::slotProcessSelectedWithOneReferenceClicked);
	connect(ui.pushButtonStop, &QPushButton::clicked, this, &Flatfield::slotCancelClicked);

	connect(ui.checkBoxSourceFilesRecurseSubfolders, &QPushButton::clicked, this, &Flatfield::slotSourceFilesRecurseSubfoldersChanged);
	connect(ui.checkBoxMatcherFocalLengthIgnore, &QCheckBox::stateChanged, this, &Flatfield::slotReferenceFileMatcherIgnoreFocalLengthChanged);
	connect(ui.checkBoxMatcherFNumberIgnore, &QCheckBox::clicked, this, &Flatfield::slotReferenceFileMatcherIgnoreFNumberChanged);
	connect(ui.checkBoxMatcherIgnoreExifTagLens, &QPushButton::clicked, this, &Flatfield::slotReferenceFileMatcherIgnoreLensTagChanged);
	connect(ui.lineEditSaveToSubfolderName, &QLineEdit::textChanged, this, &Flatfield::slotSaveProcessedFilesToSubfolderFolderNameChanged);
	connect(ui.lineEditSaveToSubfolderName, &QLineEdit::textChanged, this, &Flatfield::slotSaveProcessedFilesToSubfolderFolderNameChanged);

	connect(&referenceFiles, &ReferenceFiles::signalRebuildingDBStarted, this, &Flatfield::slotFileScanStarted);
	connect(&referenceFiles, &ReferenceFiles::signalRebuildingDBProgressChanged, this, &Flatfield::slotFileScanProgressChanged);
	connect(&referenceFiles, &ReferenceFiles::signalRebuildingDBFinished, this, &Flatfield::slotFileScanFinished);
	connect(&referenceFiles, &ReferenceFiles::signalDBSizeChanged, this, &Flatfield::slotReferenceFilesDBSizeChanged);

	connect(processor, &Processor::signalProcessingStarted, this, &Flatfield::slotProcessingStarted);
	connect(processor, &Processor::signalProcessingFinished, this, &Flatfield::slotProcessingFinished);
	connect(processor, &Processor::signalProcessingProgressChanged, this, &Flatfield::slotProcessingGlobalProgressChanged);

	connect(this, &Flatfield::signalProcessingStarted, this, &Flatfield::slotFileScanStarted);
	connect(this, &Flatfield::signalProcessingFinished, this, &Flatfield::slotFileScanFinished);
	connect(this, &Flatfield::signalProcessingProgressChanged, this, &Flatfield::slotProcessingGlobalProgressChanged);
}

void Flatfield::setSaveToViewsState() const
{
	const SavingOptions::SaveToEnum saveTo = settings.savingOptions.saveTo;
	ui.radioButtonSaveToFolder->setChecked(saveTo == SavingOptions::SaveToEnum::Folder);
	ui.radioButtonSaveToSubfolder->setChecked(saveTo == SavingOptions::SaveToEnum::Subfolder);
	ui.pushButtonSaveToFolderSelectFolder->setEnabled(saveTo == SavingOptions::SaveToEnum::Folder);
	ui.lineEditSaveToSubfolderName->setEnabled(saveTo == SavingOptions::SaveToEnum::Subfolder);
}

void Flatfield::slotReferenceFilesSelectRootClicked()
{
	const QString folder = QFileDialog::getExistingDirectory(this, "Select reference files root folder", settings.referenceMatcherOptions.referenceFilesRoot, QFileDialog::Option::ShowDirsOnly);
	if (folder.isEmpty())
	{
		return;
	}

	ui.lineEditReferenceFilesRoot->setText(folder);
	settings.referenceMatcherOptions.referenceFilesRoot = folder;
	referenceFiles.load(folder);
}

void Flatfield::slotReferenceFilesRebuildDBClicked()
{
	QFuture<void> future = QtConcurrent::run(&ReferenceFiles::createDB, &referenceFiles, settings.referenceMatcherOptions.referenceFilesRoot);
}

void Flatfield::slotSourceFilesSelectRootClicked()
{
	const QString folder = QFileDialog::getExistingDirectory(this, "Select source files root folder", settings.sourceFilesRoot, QFileDialog::Option::ShowDirsOnly);

	if (folder.isEmpty())
	{
		return;
	}

	emit signalProcessingStarted(0);

	ui.lineEditSourceFilesRoot->setText(folder);
	settings.sourceFilesRoot = folder;
	sourceFiles.clear();
	pathToSourceFileMap.clear();

	QList<QString> files;

	QDirIterator iterator(settings.sourceFilesRoot, { "*.dng" }, QDir::Files, settings.sourceFilesRecurseSubfolders ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
	while (iterator.hasNext())
	{
		files.append(iterator.next());
	}

	emit signalProcessingStarted(files.size());

	for (int i = 0; i < files.size(); i++)
	{
		const QString filePath = files[i];
		QSharedPointer<Metadata> metadata = MetadataReader::readMetadata(filePath);
		if (metadata != nullptr)
		{
			QSharedPointer<SourceFileInfo> sourceFileInfo = QSharedPointer<SourceFileInfo>(new SourceFileInfo(filePath, metadata, settings.defaultFileProcessingOptions));
			sourceFiles.append(sourceFileInfo);
			pathToSourceFileMap[filePath] = sourceFileInfo;
		}

		emit signalProcessingProgressChanged(i + 1);
	}

	fillSourceList();
	rematch();

	emit signalProcessingFinished();
}

void Flatfield::slotSaveToOutputFolderClicked()
{
	const QString folder = QFileDialog::getExistingDirectory(this, "Select output folder", settings.savingOptions.saveToFolderPath, QFileDialog::Option::ShowDirsOnly);

	if (folder.isEmpty())
	{
		return;
	}

	settings.savingOptions.saveToFolderPath = folder;
	ui.lineEditSaveToFolderRoot->setText(settings.savingOptions.saveToFolderPath);
}

QColor Flatfield::getSourceFileItemColor(const QSharedPointer<SourceFileInfo>& sourceFileInfo)
{
	if (sourceFileInfo->activeReferenceFile)
	{
		return QColor(0, 127, 0, 255);
	}

	if (sourceFileInfo->referenceFiles.empty())
	{
		return QColor(255, 0, 0, 255);
	}

	if (sourceFileInfo->referenceFiles.size() > 1)
	{
		return QColor(255, 127, 0, 255);
	}

	return QColor(0, 0, 0, 255);
}


QList<QSharedPointer<SourceFileInfo>> Flatfield::getSelectedSourceFiles() const
{
	const QModelIndexList modelIndices = ui.tableViewSourceFiles->selectionModel()->selectedRows();
	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles;

	for (int i = 0; i < modelIndices.count(); i++)
	{
		QSharedPointer<SourceFileInfo> sourceFileInfo = pathToSourceFileMap[FileUtils::getAbsolutePath(settings.sourceFilesRoot, sourceFilesModel->item(modelIndices.at(i).row(), 0)->text().trimmed())];
		if (sourceFileInfo)
		{
			selectedSourceFiles.append(sourceFileInfo);
		}

	}
	return selectedSourceFiles;
}

QList<QSharedPointer<SourceFileInfo>> Flatfield::getAllReadySourceFiles() const
{
	QList<QSharedPointer<SourceFileInfo>> result;
	for (int i = 0; i < sourceFiles.size(); i++)
	{
		if (sourceFiles[i]->activeReferenceFile)
		{
			result.append(sourceFiles[i]);
		}
	}
	return result;
}

QList<QSharedPointer<SourceFileInfo>> Flatfield::getSelectedReadySourceFiles() const
{
	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();
	QList<QSharedPointer<SourceFileInfo>> result;

	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		if (selectedSourceFiles[i]->activeReferenceFile)
		{
			result.append(selectedSourceFiles[i]);
		}
	}
	return result;
}

QSharedPointer<FileInfo> Flatfield::getSelectedReferenceFile() const
{
	const QModelIndexList modelIndices = ui.tableViewReferenceFiles->selectionModel()->selectedRows();
	if (modelIndices.length() > 0)
	{
		return referenceFiles.getFileMetadata(FileUtils::getAbsolutePath(settings.referenceMatcherOptions.referenceFilesRoot, referenceFilesModel->item(modelIndices.at(0).row(), 1)->text().trimmed()));
	}

	return nullptr;
}

bool Flatfield::areSelectedFilesCompatible() const
{
	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();
	if (selectedSourceFiles.isEmpty())
	{
		return false;
	}

	bool areCompatible = true;
	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		for (int k = 0; k < selectedSourceFiles.size(); k++)
		{
			areCompatible &= Metadata::isCompatible(selectedSourceFiles[i]->sourceFile->metadata, selectedSourceFiles[k]->sourceFile->metadata);
		}
	}
	return areCompatible;
}

void Flatfield::setProcessButtonsVisibility(const bool isVisible) const
{
	ui.pushButtonProcessReady->setVisible(isVisible);
	ui.pushButtonProcessReadySelected->setVisible(isVisible);
	ui.pushButtonProcessWithOneReference->setVisible(isVisible);
	ui.pushButtonStop->setVisible(!isVisible);
}

QString Flatfield::getReferenceFilesCountText(int count)
{
	return QString(count == 1 ? "%1 file" : "%1 files").arg(count);
}

void Flatfield::rematch()
{
	for (int i = 0; i < sourceFiles.size(); i++)
	{
		const QSharedPointer<SourceFileInfo> sourceFileInfo = sourceFiles[i];

		sourceFileInfo->activeReferenceFile = nullptr;
		sourceFileInfo->referenceFiles.clear();
		sourceFileInfo->referenceFiles.append(referenceFiles.findMatchingReferenceFiles(sourceFileInfo->sourceFile->metadata, settings.referenceMatcherOptions));

		if (sourceFileInfo->referenceFiles.size() == 1)
		{
			sourceFileInfo->activeReferenceFile = sourceFileInfo->referenceFiles[0];
		}
	}
	colorSourceList();
	fillReferenceList(getSelectedSourceFiles());
}

void Flatfield::start() const
{
	QList<QSharedPointer<SourceFileInfo>> sourceFilesList = getSelectedSourceFiles();
	QList<ProcessingItem> processingItems;

	for (int i = 0; i < sourceFilesList.size(); i++)
	{
		processingItems.append(ProcessingItem(sourceFilesList[i]->sourceFile, sourceFilesList[i]->activeReferenceFile, sourceFilesList[i]->processingOptions));
	}
	processor->process(ProcessingParcel(processingItems, settings.sourceFilesRoot, settings.globalProcessingOptions, settings.savingOptions));
}

void Flatfield::fillSourceList() const
{
	sourceFilesModel->clear();
	sourceFilesModel->setRowCount(sourceFiles.count());

	for (int i = 0; i < sourceFiles.size(); i++)
	{
		const QSharedPointer<SourceFileInfo> sourceFileInfo = sourceFiles[i];
		QString relativeFilePath = FileUtils::getRelativePath(settings.sourceFilesRoot, sourceFileInfo->sourceFile->filePath);
		QStandardItem* filePathItem = new QStandardItem(relativeFilePath + "      ");
		QStandardItem* metadataItem = new QStandardItem(sourceFileInfo->sourceFile->metadata->toString());

		sourceFilesModel->setItem(i, 0, filePathItem);
		sourceFilesModel->setItem(i, 1, metadataItem);
	}

	colorSourceList();
	ui.lineEditSourceFilesFound->setText(QString(sourceFiles.count() == 1 ? "%1 file" : "%1 files").arg(sourceFiles.count()));
}

void Flatfield::colorSourceList() const
{
	for (int i = 0; i < sourceFiles.size(); i++)
	{
		QStandardItem* filePathItem = sourceFilesModel->item(i, 0);
		QStandardItem* metadataItem = sourceFilesModel->item(i, 1);

		QSharedPointer<SourceFileInfo> sourceFileInfo = pathToSourceFileMap[sourceFiles[i]->sourceFile->filePath];
		if (sourceFileInfo)
		{
			QColor sourceFileItemColor = getSourceFileItemColor(sourceFileInfo);
			filePathItem->setForeground(QBrush(sourceFileItemColor));
			metadataItem->setForeground(QBrush(sourceFileItemColor));
		}
	}
}

void Flatfield::setReferenceFile(const QSharedPointer<FileInfo>& selectedReferenceFile) const
{
	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();

	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		selectedSourceFiles[i]->activeReferenceFile = selectedReferenceFile;
	}

	colorSourceList();
	markReferenceList(selectedSourceFiles, ReferenceFiles::getCommonReferenceFiles(selectedSourceFiles, settings.referenceMatcherOptions));
	updateProcessButtonsState();
}

void Flatfield::fillReferenceList(const QList<QSharedPointer<SourceFileInfo>>& selectedSourceFiles) const
{
	QList<QSharedPointer<FileInfo>> commonReferenceFiles = ReferenceFiles::getCommonReferenceFiles(selectedSourceFiles, settings.referenceMatcherOptions);

	referenceFilesModel->clear();
	referenceFilesModel->setRowCount(commonReferenceFiles.size());

	for (int i = 0; i < commonReferenceFiles.size(); i++)
	{
		QString relativeFilePath = FileUtils::getRelativePath(settings.referenceMatcherOptions.referenceFilesRoot, commonReferenceFiles[i]->filePath);
		QStandardItem* indicatorItem = new QStandardItem("  ");
		QStandardItem* filePathItem = new QStandardItem(relativeFilePath + "      ");
		QStandardItem* metadataItem = new QStandardItem(commonReferenceFiles[i]->metadata->toString());

		indicatorItem->setTextAlignment(Qt::AlignCenter);
		QFont font = indicatorItem->font();
		font.setBold(true);
		indicatorItem->setFont(font);

		referenceFilesModel->setItem(i, 0, indicatorItem);
		referenceFilesModel->setItem(i, 1, filePathItem);
		referenceFilesModel->setItem(i, 2, metadataItem);
	}

	markReferenceList(selectedSourceFiles, commonReferenceFiles);
}

void Flatfield::markReferenceList(const QList<QSharedPointer<SourceFileInfo>>& selectedSourceFiles, const QList<QSharedPointer<FileInfo>>& commonReferenceFiles) const
{
	const QSharedPointer<FileInfo> commonActiveReferenceFile = getCommonActiveReferenceFile(selectedSourceFiles);

	for (int i = 0; i < commonReferenceFiles.size(); i++)
	{
		QStandardItem* indicatorItem = referenceFilesModel->item(i, 0);

		if (commonReferenceFiles[i] == commonActiveReferenceFile)
		{
			indicatorItem->setText("->");
		}
		else
		{
			indicatorItem->setText("  ");
		}
	}
}

QSharedPointer<FileInfo> Flatfield::getCommonActiveReferenceFile(const QList<QSharedPointer<SourceFileInfo>>& selectedSourceFiles)
{
	QSharedPointer<FileInfo> commonActiveReferenceFile;

	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		if (i == 0)
		{
			commonActiveReferenceFile = selectedSourceFiles[i]->activeReferenceFile;
		}
		else
		{
			if (commonActiveReferenceFile != selectedSourceFiles[i]->activeReferenceFile)
			{
				return nullptr;
			}
		}
	}

	return commonActiveReferenceFile;
}

void Flatfield::setUIState(bool isEnabled) const
{
	ui.pushButtonReferenceFilesSelectRoot->setEnabled(isEnabled);
	ui.pushButtonReferenceFilesRebuildDB->setEnabled(isEnabled);
	ui.pushButtonSourceFilesSelectRoot->setEnabled(isEnabled);
	ui.checkBoxSourceFilesRecurseSubfolders->setEnabled(isEnabled);
	ui.tableViewSourceFiles->setEnabled(isEnabled);
	ui.tableViewReferenceFiles->setEnabled(isEnabled);
	ui.groupBoxReferenceFilesMatcher->setEnabled(isEnabled);
	ui.groupBoxProcessingSettings->setEnabled(isEnabled);
	ui.groupBoxSaveProcessedFiles->setEnabled(isEnabled);
	ui.groupBoxGlobalProcessingOptions->setEnabled(isEnabled);

	updateProcessButtonsState();
}

void Flatfield::prepareProcessingParcel(const QList<QSharedPointer<SourceFileInfo>>& files) const
{
	QList<ProcessingItem> processingItems;
	for (int i = 0; i < files.size(); i++)
	{
		if (settings.savingOptions.saveTo == SavingOptions::Subfolder && FileUtils::isFileFromOutputSubfolder(files[i]->sourceFile->filePath, settings.savingOptions))
		{
			continue;
		}

		const QSharedPointer<SourceFileInfo>& sourceFileInfo = files[i];
		if (sourceFileInfo->activeReferenceFile)
		{
			processingItems.append(ProcessingItem(sourceFileInfo->sourceFile, sourceFileInfo->activeReferenceFile, sourceFileInfo->processingOptions));
		}
	}
	processor->process(ProcessingParcel(processingItems, settings.sourceFilesRoot, settings.globalProcessingOptions, settings.savingOptions));
}

void Flatfield::colorCalculateCommonBatchScaleCheckbox() const
{
	ui.checkBoxProcessingCalculateCommonScale->setChecked(settings.globalProcessingOptions.scaleChannelsToAvoidClipping && settings.globalProcessingOptions.calculateCommonScaleForBatch);
	ui.checkBoxProcessingCalculateCommonScale->setEnabled(settings.globalProcessingOptions.scaleChannelsToAvoidClipping);

	if (settings.globalProcessingOptions.scaleChannelsToAvoidClipping && settings.globalProcessingOptions.calculateCommonScaleForBatch)
	{
		ui.checkBoxProcessingCalculateCommonScale->setStyleSheet("QCheckBox {color: red;}");
	}
	else
	{
		ui.checkBoxProcessingCalculateCommonScale->setStyleSheet("");
	}
}

void Flatfield::slotSaveToFolderRadioButtonClicked()
{
	settings.savingOptions.saveTo = SavingOptions::SaveToEnum::Folder;
	setSaveToViewsState();
}

void Flatfield::slotSaveToSubfolderRadioButtonClicked()
{
	settings.savingOptions.saveTo = SavingOptions::SaveToEnum::Subfolder;
	setSaveToViewsState();
}

void Flatfield::slotReferenceFileMatcherMaxAllowedFocalLengthDifferenceEditingFinished() const
{
	if (ui.lineEditMatcherFocalLengthMaxDifference->text().isEmpty())
	{
		ui.lineEditMatcherFocalLengthMaxDifference->setText(QString::number(settings.referenceMatcherOptions.allowedFocalLengthDifferencePercents, 'g', 3));
	}
}

void Flatfield::slotReferenceFileMatcherMaxAllowedFNumberDifferenceEditingFinished() const
{
	if (ui.lineEditMatcherFNumberMaxDifference->text().isEmpty())
	{
		ui.lineEditMatcherFNumberMaxDifference->setText(QString::number(settings.referenceMatcherOptions.allowedFNumberDifferenceStops, 'g', 2));
	}
}

void Flatfield::slotSourceFileSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) const
{
	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();
	if (selectedSourceFiles.isEmpty())
	{
		ui.lineEditProcessingLuminanceCorrectionIntensity->setEnabled(false);
		ui.lineEditProcessingColorCorrectionIntensity->setEnabled(false);
		ui.lineEditProcessingGaussianBlurSigma->setEnabled(false);
		ui.pushButtonProcessReadySelected->setEnabled(false);
		ui.pushButtonProcessWithOneReference->setEnabled(false);
	}
	else
	{
		ui.lineEditProcessingLuminanceCorrectionIntensity->setEnabled(true);
		ui.lineEditProcessingColorCorrectionIntensity->setEnabled(true);
		ui.lineEditProcessingGaussianBlurSigma->setEnabled(true);

		bool isLuminanceCorrectionIntensityTheSame = true;
		bool isColorCorrectionIntensityTheSame = true;
		bool isGaussianBlurSigmaTheSame = true;

		const QSharedPointer<SourceFileInfo> firstSelectedSourceFile = selectedSourceFiles[0];

		for (int i = 0; i < selectedSourceFiles.size(); i++)
		{
			const QSharedPointer<SourceFileInfo> currentSelectedSourceFile = selectedSourceFiles[i];

			if (!ProcessingOptions::areFloatValuesEqual(firstSelectedSourceFile->processingOptions.luminanceCorrectionIntensity, currentSelectedSourceFile->processingOptions.luminanceCorrectionIntensity))
			{
				isLuminanceCorrectionIntensityTheSame = false;
			}
			if (!ProcessingOptions::areFloatValuesEqual(firstSelectedSourceFile->processingOptions.colorCorrectionIntensity, currentSelectedSourceFile->processingOptions.colorCorrectionIntensity))
			{
				isColorCorrectionIntensityTheSame = false;
			}
			if (!ProcessingOptions::areFloatValuesEqual(firstSelectedSourceFile->processingOptions.gaussianBlurSigma, currentSelectedSourceFile->processingOptions.gaussianBlurSigma))
			{
				isGaussianBlurSigmaTheSame = false;
			}
		}

		if (isLuminanceCorrectionIntensityTheSame)
		{
			ui.lineEditProcessingLuminanceCorrectionIntensity->setText(QString::number(firstSelectedSourceFile->processingOptions.luminanceCorrectionIntensity, 'g', 3));
		}
		else
		{
			ui.lineEditProcessingLuminanceCorrectionIntensity->clear();
		}

		if (isColorCorrectionIntensityTheSame)
		{
			ui.lineEditProcessingColorCorrectionIntensity->setText(QString::number(firstSelectedSourceFile->processingOptions.colorCorrectionIntensity, 'g', 3));
		}
		else
		{
			ui.lineEditProcessingColorCorrectionIntensity->clear();
		}

		if (isGaussianBlurSigmaTheSame)
		{
			ui.lineEditProcessingGaussianBlurSigma->setText(QString::number(firstSelectedSourceFile->processingOptions.gaussianBlurSigma, 'g', 3));
		}
		else
		{
			ui.lineEditProcessingGaussianBlurSigma->clear();
		}
	}

	fillReferenceList(selectedSourceFiles);
	updateProcessButtonsState();
}

void Flatfield::slotReferenceFileSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) const
{
	setReferenceFile(getSelectedReferenceFile());
}

void Flatfield::slotReferenceFilesClickOnTableEmptySpace() const
{
	setReferenceFile(nullptr);
}

void Flatfield::slotProcessAllReadyClicked() const
{
	prepareProcessingParcel(sourceFiles);
}

void Flatfield::slotProcessAllReadySelectedClicked() const
{
	prepareProcessingParcel(getSelectedSourceFiles());
}

void Flatfield::slotProcessSelectedWithOneReferenceClicked() const
{
	const QString referenceFileName = QFileDialog::getOpenFileName(nullptr, "", "", ("DNG file (*.dng)"));
	if (referenceFileName.isEmpty())
	{
		return;
	}

	const QSharedPointer<Metadata> referenceFileMetadata = MetadataReader::readMetadata(referenceFileName);

	if (!referenceFileMetadata)
	{
		return;
	}

	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();

	bool isReferenceCompatibleWithSources = true;
	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		isReferenceCompatibleWithSources &= Metadata::isCompatible(referenceFileMetadata, selectedSourceFiles[i]->sourceFile->metadata);
	}

	if (!isReferenceCompatibleWithSources)
	{
		return;
	}

	QList<ProcessingItem> processingItems;
	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		processingItems.append(ProcessingItem(selectedSourceFiles[i]->sourceFile, QSharedPointer<FileInfo>(new FileInfo(referenceFileName, referenceFileMetadata)), selectedSourceFiles[i]->processingOptions));
	}
	processor->process(ProcessingParcel(processingItems, settings.sourceFilesRoot, settings.globalProcessingOptions, settings.savingOptions));
}

void Flatfield::slotCancelClicked() const
{
	processor->stopProcessing();
}

void Flatfield::slotSourceFilesRecurseSubfoldersChanged(bool recurse)
{
	settings.sourceFilesRecurseSubfolders = recurse;
}

void Flatfield::updateProcessButtonsState() const
{
	ui.pushButtonProcessReady->setEnabled(!getAllReadySourceFiles().isEmpty());
	ui.pushButtonProcessReadySelected->setEnabled(!getSelectedReadySourceFiles().isEmpty());
	ui.pushButtonProcessWithOneReference->setEnabled(areSelectedFilesCompatible());
}

void Flatfield::slotReferenceFileMatcherMaxAllowedFocalLengthDifferencePercentsChanged(const QString& differenceString)
{
	bool isSuccessful;
	const float difference = differenceString.toFloat(&isSuccessful);
	isSuccessful &= Settings::isReferenceFileMatcherMaxAllowedFocalLengthDifferencePercentsValid(difference);
	if (isSuccessful)
	{
		settings.referenceMatcherOptions.allowedFocalLengthDifferencePercents = difference;
	}
}

void Flatfield::slotReferenceFileMatcherMaxAllowedFNumberDifferenceStopsChanged(const QString& differenceString)
{
	bool isSuccessful;
	const float difference = differenceString.toFloat(&isSuccessful);
	isSuccessful &= Settings::isReferenceFileMatcherMaxAllowedFNumberDifferenceStopsValid(difference);
	if (isSuccessful)
	{
		settings.referenceMatcherOptions.allowedFNumberDifferenceStops = difference;
	}
}

void Flatfield::slotReferenceFileMatcherIgnoreFocalLengthChanged(bool ignore)
{
	settings.referenceMatcherOptions.ignoreFocalLength = ignore;
	ui.lineEditMatcherFocalLengthMaxDifference->setEnabled(!ignore);
}

void Flatfield::slotReferenceFileMatcherIgnoreFNumberChanged(bool ignore)
{
	settings.referenceMatcherOptions.ignoreFNumber = ignore;
	ui.lineEditMatcherFNumberMaxDifference->setEnabled(!ignore);
}

void Flatfield::slotReferenceFileMatcherIgnoreLensTagChanged(bool ignore)
{
	settings.referenceMatcherOptions.ignoreLensTag = ignore;
}

void Flatfield::slotReferenceFileMatcherRematchClicked()
{
	rematch();
}

void Flatfield::slotProcessingLuminanceCorrectionIntensityChanged(const QString& intensityString) const
{
	bool isSuccessful;
	const float intensity = intensityString.toFloat(&isSuccessful);
	if (isSuccessful && Settings::isProcessingIntensityValid(intensity))
	{
		QList<QSharedPointer<SourceFileInfo>> files = getSelectedSourceFiles();
		for (int i = 0; i < files.size(); i++)
		{
			files[i]->processingOptions.luminanceCorrectionIntensity = intensity;
		}
	}
}

void Flatfield::slotProcessingColorCorrectionIntensityChanged(const QString& intensityString) const
{
	bool isSuccessful;
	const float intensity = intensityString.toFloat(&isSuccessful);
	if (isSuccessful && Settings::isProcessingIntensityValid(intensity))
	{
		QList<QSharedPointer<SourceFileInfo>> files = getSelectedSourceFiles();
		for (int i = 0; i < files.size(); i++)
		{
			files[i]->processingOptions.colorCorrectionIntensity = intensity;
		}
	}
}

void Flatfield::slotProcessingGaussianBlurSigmaChanged(const QString& sigmaString) const
{
	bool isSuccessful;
	const float sigma = sigmaString.toFloat(&isSuccessful);
	if (isSuccessful && Settings::isGaussianBlurSigmaValid(sigma))
	{
		QList<QSharedPointer<SourceFileInfo>> files = getSelectedSourceFiles();
		for (int i = 0; i < files.size(); i++)
		{
			files[i]->processingOptions.gaussianBlurSigma = sigma;
		}
	}
}

void Flatfield::slotProcessingLuminanceCorrectionIntensityEditingFinished() const
{
	if (ui.lineEditProcessingLuminanceCorrectionIntensity->text().isEmpty())
	{
		ui.lineEditProcessingLuminanceCorrectionIntensity->setText(QString::number(settings.defaultFileProcessingOptions.luminanceCorrectionIntensity, 'g', 2));
	}

	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();
	const float luminanceCorrectionIntensity = ui.lineEditProcessingLuminanceCorrectionIntensity->text().toFloat();
	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		selectedSourceFiles[i]->processingOptions.luminanceCorrectionIntensity = luminanceCorrectionIntensity;
	}
}

void Flatfield::slotProcessingColorCorrectionIntensityEditingFinished() const
{
	if (ui.lineEditProcessingColorCorrectionIntensity->text().isEmpty())
	{
		ui.lineEditProcessingColorCorrectionIntensity->setText(QString::number(settings.defaultFileProcessingOptions.colorCorrectionIntensity, 'g', 2));
	}
	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();
	const float colorCorrectionIntensity = ui.lineEditProcessingColorCorrectionIntensity->text().toFloat();
	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		selectedSourceFiles[i]->processingOptions.colorCorrectionIntensity = colorCorrectionIntensity;
	}
}

void Flatfield::slotProcessingGaussianBlurSigmaEditingFinished() const
{
	if (ui.lineEditProcessingGaussianBlurSigma->text().isEmpty())
	{
		ui.lineEditProcessingGaussianBlurSigma->setText(QString::number(settings.defaultFileProcessingOptions.gaussianBlurSigma, 'g', 2));
	}

	QList<QSharedPointer<SourceFileInfo>> selectedSourceFiles = getSelectedSourceFiles();
	const float gaussianBlurSigma = ui.lineEditProcessingGaussianBlurSigma->text().toFloat();
	for (int i = 0; i < selectedSourceFiles.size(); i++)
	{
		selectedSourceFiles[i]->processingOptions.gaussianBlurSigma = gaussianBlurSigma;
	}
}

void Flatfield::slotProcessingLimitToWhiteLevelChanged(bool limit)
{
	settings.globalProcessingOptions.limitToWhiteLevel = limit;
}

void Flatfield::slotProcessingScaleChannelsToAvoidClipping(bool scale)
{
	settings.globalProcessingOptions.scaleChannelsToAvoidClipping = scale;

	ui.checkBoxProcessingCalculateCommonScale->setEnabled(scale);

	colorCalculateCommonBatchScaleCheckbox();
}

void Flatfield::slotProcessingCalculateCommonScaleForBatch(bool calculate)
{
	settings.globalProcessingOptions.calculateCommonScaleForBatch = calculate;

	if (calculate && settings.globalProcessingOptions.scaleChannelsToAvoidClipping)
	{
		ui.checkBoxProcessingCalculateCommonScale->setStyleSheet("QCheckBox {color: red;}");
	}
	else
	{
		ui.checkBoxProcessingCalculateCommonScale->setStyleSheet("QCheckBox {color: black;}");
	}
}

void Flatfield::slotSaveProcessedFilesToFolderPathChanged(const QString& path)
{
	settings.savingOptions.saveToFolderPath = path;
}

void Flatfield::slotSaveProcessedFilesToSubfolderFolderNameChanged(const QString& name)
{
	settings.savingOptions.saveToSubfolderFolderName = name;
}

void Flatfield::slotProcessingStarted(int total) const
{
	ui.progressBarProcessing->setValue(0);
	ui.progressBarProcessing->setMaximum(total);

	setUIState(false);
	setProcessButtonsVisibility(false);
}

void Flatfield::slotProcessingGlobalProgressChanged(int progress) const
{
	ui.progressBarProcessing->setValue(progress);
}

void Flatfield::slotProcessingFinished() const
{
	ui.progressBarProcessing->setValue(0);

	setUIState(true);
	setProcessButtonsVisibility(true);
}

void Flatfield::slotFileScanStarted(int total) const
{
	ui.progressBarProcessing->setValue(0);
	ui.progressBarProcessing->setMaximum(total);

	setUIState(false);
}

void Flatfield::slotFileScanProgressChanged(int progress) const
{
	ui.progressBarProcessing->setValue(progress);
}

void Flatfield::slotFileScanFinished() const
{
	ui.progressBarProcessing->setValue(0);

	setUIState(true);
}

void Flatfield::slotReferenceFilesDBSizeChanged(int count)
{
	ui.lineEditReferenceFilesFound->setText(getReferenceFilesCountText(count));
	rematch();
}
