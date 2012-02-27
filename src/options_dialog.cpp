#include "pch.h"
#include "options_dialog.h"
#include "options.h"

OptionsDialog::OptionsDialog(QWidget *parent)
: QDialog(parent)
{
	setupUi(this);

	// получаем настройки сетки и направляющих
	Options &options = Options::getSingleton();
	mShowGridCheckBox->setChecked(options.isShowGrid());
	mShowDotsCheckBox->setChecked(options.isShowDots());
	mSnapToGridCheckBox->setChecked(options.isSnapToGrid());
	mSnapToVisibleLinesCheckBox->setChecked(options.isSnapToVisibleLines());
	mGridSpacingSpinBox->setValue(options.getGridSpacing());
	mMajorLinesIntervalSpinBox->setValue(options.getMajorLinesInterval());

	mShowGuidesCheckBox->setChecked(options.isShowGuides());
	mSnapToGuidesCheckBox->setChecked(options.isSnapToGuides());
	mEnableSmartGuidesCheckBox->setChecked(options.isEnableSmartGuides());

	// устанавливаем фиксированный размер для диалогового окна
	setVisible(true);
	setFixedSize(size());
}

void OptionsDialog::accept()
{
	applySettings();
	QDialog::accept();
}

void OptionsDialog::on_mButtonBox_clicked(QAbstractButton *button)
{
	if (mButtonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
		applySettings();
}

void OptionsDialog::applySettings()
{
	// устанавливаем настройки сетки и направляющих
	Options &options = Options::getSingleton();
	options.setShowGrid(mShowGridCheckBox->isChecked());
	options.setShowDots(mShowDotsCheckBox->isChecked());
	options.setSnapToGrid(mSnapToGridCheckBox->isChecked());
	options.setSnapToVisibleLines(mSnapToVisibleLinesCheckBox->isChecked());
	options.setGridSpacing(mGridSpacingSpinBox->value());
	options.setMajorLinesInterval(mMajorLinesIntervalSpinBox->value());

	options.setShowGuides(mShowGuidesCheckBox->isChecked());
	options.setSnapToGuides(mSnapToGuidesCheckBox->isChecked());
	options.setEnableSmartGuides(mEnableSmartGuidesCheckBox->isChecked());

	// сохраняем настройки в конфигурационный файл
	QSettings settings;
	options.save(settings);
}
