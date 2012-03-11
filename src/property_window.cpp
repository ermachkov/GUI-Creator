#include "property_window.h"
#include "game_object.h"
#include "label.h"
#include "sprite.h"
#include "utils.h"
#include <typeinfo>

PropertyWindow::PropertyWindow(QWidget *parent)
: QDockWidget(parent)
{
	setupUi(this);

	//this->scrollArea->adjustSize();

	//setSizeConstraint(QLayout::SetMinimumSize);
	//dockWidgetContents->setSizeConstraint(QLayout::SetMinimumSize);
	//layout->setSizeConstraint(QLayout::SetMinimumSize);
	//scrollArea->adjustSize();

	//stackedWidget->adjustSize();
/*
	QSize s;

	stackedWidget->setCurrentIndex(0);
	page->adjustSize();
	qDebug() << page->size();
	page->setFixedSize(page->size());

	qDebug() << " area:" << stackedWidget->size();

//	s = page->size();
//	page->setMinimumSize(s);
//	stackedWidget->resize(page->size());


	stackedWidget->setCurrentIndex(1);
	page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	page_2->adjustSize();
	qDebug() << page_2->size();
	page_2->setFixedSize(page_2->size());

//	s = page_2->size();
//	page_2->setMinimumSize(s);
//	stackedWidget->resize(page_2->size());

	qDebug() << " area:" << stackedWidget->size();


	//scrollAreaWidgetContents->resize(QSize(195, 23));
	//page->resize(QSize(95, 23));
	//page_2->resize(QSize(95, 23));

	scrollArea->updateGeometry();
*/
//	stackedWidget->setVisible(false);

//	stackedWidget->setCurrentIndex(0);
//	stackedWidget->setCurrentIndex(1);


	//scrollAreaWidgetContents->adjustSize();
}

void PropertyWindow::onEditorWindowSelectionChanged(const QList<GameObject *> &objects)
{
	//qDebug() << objects;

	// нет выделенных объектов
	if (objects.empty())
	{
		// установка минимального набора общих свойств
		mStackedWidget->setVisible(false);

		// деактивация гуя свойств
		mScrollArea->setEnabled(false);

		return;
	}
	else
	{
		// активация гуя свойств
		mScrollArea->setEnabled(true);
	}

	qDebug() << "--";
	foreach (GameObject *object, objects)
		qDebug() << typeid(*object).name();

	// отображение общих свойств
	updateCommonWidgets(objects);

	// определение идентичности типа выделенных объектов
	QStringList types;
	foreach (GameObject *object, objects)
		types.push_back(typeid(*object).name());
	if (types.count(types.front()) != types.size())
	{
		// выделенные объекты не идентичны

		// установка минимального набора общих свойств
		mStackedWidget->setVisible(false);

		return;
	}
	else
	{
		// установка дополнительного набора специфических свойств
		mStackedWidget->setVisible(true);
	}

	if (dynamic_cast<Sprite *>(objects.front()) != NULL)
	{
		// установка ГУИ для спрайта

		// установка размера виджета для дополнительных свойств
		//mSpritePage->adjustSize();
		//QSize size = mSpritePage->size();
		//mSpritePage->setMinimumSize(size);

		// установка страницы для специфических настроек спрайта
		mStackedWidget->setCurrentIndex(0);

		// отображение свойств спрайтов
		updateSpriteWidgets(objects);
	}
	else if (dynamic_cast<Label *>(objects.front()) != NULL)
	{
		// установка ГУИ для текста

		// установка размера виджета для дополнительных свойств
		//mLabelPage->adjustSize();
		//QSize size = mLabelPage->size();
		//mLabelPage->setMinimumSize(size);

		// установка страницы для специфических настроек текста
		mStackedWidget->setCurrentIndex(1);

		// отображение свойств спрайтов
		updateLabelWidgets(objects);
	}
//	else if (dynamic_cast<"your type here">(objects.front()) != NULL)
//	{
//	}
	else
	{
		qWarning() << "Error can't find type of object";
	}

}

void PropertyWindow::updateCommonWidgets(const QList<GameObject *> &objects)
{
	// флаги идентичности значений свойств объектов
	bool equalName = true;
	bool equalObjectID = true;
	bool equalPosition = true;
	bool equalSize = true;
	bool isPositiveWSize = true;
	bool isNegativeWSize = true;
	bool isPositiveHSize = true;
	bool isNegativeHSize = true;
	bool equalRotationAngle = true;
	//bool equalRotationCenter;

	Q_ASSERT(!objects.empty());

	GameObject *first = objects.front();

	foreach (GameObject *obj, objects)
	{
		GameObject *it = obj;

		if (it->getName() != objects.front()->getName())
			equalName = false;

		if (it->getObjectID() != objects.front()->getObjectID())
			equalObjectID = false;

		if (!Utils::fuzzyCompare(it->getPosition().x(), objects.front()->getPosition().x()) ||
			!Utils::fuzzyCompare(it->getPosition().y(), objects.front()->getPosition().y()))
			equalPosition = false;

		if (!Utils::fuzzyCompare(it->getSize().width(), first->getSize().width()) ||
			!Utils::fuzzyCompare(it->getSize().height(), first->getSize().height()))
			equalSize = false;

		if (it->getSize().width() < 0)
			isPositiveWSize = false;
		if (it->getSize().width() >= 0)
			isNegativeWSize = false;
		if (it->getSize().height() < 0)
			isPositiveHSize = false;
		if (it->getSize().height() >= 0)
			isNegativeHSize = false;

		if (!Utils::fuzzyCompare(it->getRotationAngle(), first->getRotationAngle()))
			equalRotationAngle = false;

		//equalRotationCenter
	}

	const int PRECISION = 8;

	mNameLineEdit->setText(equalName ? first->getName() : "");

	mObjectIDLineEdit->setText(equalObjectID ? QString::number(first->getObjectID()) : "");

	mPositionXLineEdit->setText(equalPosition ? QString::number(first->getPosition().x(), 'g', PRECISION) : "");
	mPositionYLineEdit->setText(equalPosition ? QString::number(first->getPosition().y(), 'g', PRECISION) : "");

	mSizeWLineEdit->setText(equalSize ? QString::number(first->getSize().width(), 'g', PRECISION) : "");
	mSizeHLineEdit->setText(equalSize ? QString::number(first->getSize().height(), 'g', PRECISION) : "");

	if (isPositiveWSize)
		mFlipXCheckBox->setCheckState(Qt::Checked);
	else if (isNegativeWSize)
		mFlipXCheckBox->setCheckState(Qt::Unchecked);
	else
		mFlipXCheckBox->setCheckState(Qt::PartiallyChecked);

	if (isPositiveHSize)
		mFlipYCheckBox->setCheckState(Qt::Checked);
	else if (isNegativeHSize)
		mFlipYCheckBox->setCheckState(Qt::Unchecked);
	else
		mFlipYCheckBox->setCheckState(Qt::PartiallyChecked);

	qDebug() << "++";
	qDebug() << objects.front()->getRotationAngle();
	qDebug() << QString::number(objects.front()->getRotationAngle(), 'g', PRECISION);

	mRotationAngleComboBox->lineEdit()->setText(equalRotationAngle ? QString::number(objects.front()->getRotationAngle(), 'g', PRECISION) : "");

	//objects.front()->getRotationCenter().x()
	//objects.front()->getRotationCenter().y()
}

void PropertyWindow::updateSpriteWidgets(const QList<GameObject *> &objects)
{

}

void PropertyWindow::updateLabelWidgets(const QList<GameObject *> &objects)
{
	// флаги идентичности значений свойств объектов
	bool equalText = true;
	bool equalFileName = true;
	bool equalFontSize = true;
	bool equalAlignment = true;
	bool equalVertAlignment = true;

	bool equalLineSpacing = true;
	bool equalLabelColor = true;

	bool equalLabelOpacity = true;

	Q_ASSERT(!objects.empty());

	Label *first = dynamic_cast<Label *>(objects.front());

	foreach (GameObject *obj, objects)
	{
		Label *it = dynamic_cast<Label *>(obj);

		if (it->getText() != first->getText())
			equalText = false;

		if (it->getFileName() != first->getFileName())
			 equalFileName = false;

		if (it->getFontSize() != first->getFontSize())
			 equalFontSize = false;

		if (it->getAlignment() != first->getAlignment())
			equalAlignment = false;

		if (it->getVertAlignment() != first->getVertAlignment())
			equalVertAlignment = false;

		if (it->getLineSpacing() != first->getLineSpacing())
			equalLineSpacing = false;

		if (it->getColor().rgb() != first->getColor().rgb())
			equalLabelColor = false;

		if (it->getColor().alpha() != first->getColor().alpha())
			equalLabelOpacity = false;
	}
/*
	mTextLineEdit

	mFileNameComboBox

	mFontSizeComboBox

	mAlignmentLeftPushButton
	mAlignmentCenterPushButton
	mAlignmentRightPushButton
	mVertAlignmentTopPushButton
	mVertAlignmentCenterPushButton
	mVertAlignmentBottomPushButton

	mLineSpacingComboBox
	mLabelColorHTMLLineEdit

	mLabelOpacityHorizontalSlider
	mLabelOpacityLineEdit


	mText
	mFileName
	mFontSize
	mFont
	mAlignment
	mVertAlignment
	mLineSpacing
	mColor
*/
}

