#include "property_window.h"
#include "game_object.h"
#include "label.h"
#include "sprite.h"
#include "utils.h"
#include <typeinfo>

PropertyWindow::PropertyWindow(QWidget *parent)
: QDockWidget(parent), PRECISION(8)
{
	setupUi(this);

	// запрещение редактирования комбобокса
	mFileNameComboBox->lineEdit()->setReadOnly(true);

	connect(mRotationAngleComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onRotationAngleEditingFinished()));

	// для обработки сигнала щелчка по QFrame-ам
	mSpriteColorFrame->installEventFilter(this);
	mLabelColorFrame->installEventFilter(this);

	// по умолчанию окно свойств пустое
	mStackedWidget->setVisible(false);
	mScrollArea->setVisible(false);
	mNoSelectionObjectsLabel->setVisible(true);



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

void PropertyWindow::onEditorWindowSelectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter)
{
	//qDebug() << objects;

	// нет выделенных объектов
	if (objects.empty())
	{
		// установка минимального набора общих свойств
		mStackedWidget->setVisible(false);
		// установка отсутствия свойств
		mScrollArea->setVisible(false);
		// показ надписи об отсутствии выделенных объектов
		mNoSelectionObjectsLabel->setVisible(true);
		return;
	}
	else
	{
		mScrollArea->setVisible(true);
		mNoSelectionObjectsLabel->setVisible(false);
	}

	// отображение общих свойств
	updateCommonWidgets(objects, rotationCenter);

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

void PropertyWindow::onEditorWindowObjectsChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter)
{
	updateCommonWidgets(objects, rotationCenter);
}

bool PropertyWindow::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
		if (mouseEvent->button() == Qt::LeftButton)
		{
			if (object == mSpriteColorFrame)
			{
				// вызов диалогового окна выбора цвета
				QColorDialog colorDialog;
				QString color;
				if (colorDialog.exec() == colorDialog.Accepted)
				{
					color = colorDialog.currentColor().name();
				}

				return true;
			}
			else
			if (object == mLabelColorFrame)
			{
				// вызов диалогового окна выбора цвета
				QColorDialog colorDialog;
				QString color;
				if (colorDialog.exec() == colorDialog.Accepted)
				{
					color = colorDialog.currentColor().name();
				}

				return true;
			}
		}
	}

	return QObject::eventFilter(object, event);
}

void PropertyWindow::updateCommonWidgets(const QList<GameObject *> &objects, const QPointF &rotationCenter)
{
	// флаги идентичности значений свойств объектов в группе
	bool equalName = true;
	bool equalObjectID = true;
	bool equalPosition = true;
	bool equalSize = true;
	bool isPositiveWSize = true;
	bool isNegativeWSize = true;
	bool isPositiveHSize = true;
	bool isNegativeHSize = true;
	bool equalRotationAngle = true;

	Q_ASSERT(!objects.empty());

	GameObject *first = objects.front();

	foreach (GameObject *obj, objects)
	{
		GameObject *it = obj;

		if (it->getName() != objects.front()->getName())
			equalName = false;

		if (it->getObjectID() != objects.front()->getObjectID())
			equalObjectID = false;

		if (!Utils::isEqual(it->getPosition().x(), objects.front()->getPosition().x()) ||
			!Utils::isEqual(it->getPosition().y(), objects.front()->getPosition().y()))
			equalPosition = false;

		if (!Utils::isEqual(it->getSize().width(), first->getSize().width()) ||
			!Utils::isEqual(it->getSize().height(), first->getSize().height()))
			equalSize = false;

		if (it->getSize().width() < 0)
			isPositiveWSize = false;
		if (it->getSize().width() >= 0)
			isNegativeWSize = false;
		if (it->getSize().height() < 0)
			isPositiveHSize = false;
		if (it->getSize().height() >= 0)
			isNegativeHSize = false;

		if (!Utils::isEqual(it->getRotationAngle(), first->getRotationAngle()))
			equalRotationAngle = false;
	}

	mNameLineEdit->setText(equalName ? first->getName() : "");

	mObjectIDLineEdit->setText(equalObjectID ? QString::number(first->getObjectID()) : "");

	mPositionXLineEdit->setText(equalPosition ? QString::number(first->getPosition().x(), 'g', PRECISION) : "");
	mPositionYLineEdit->setText(equalPosition ? QString::number(first->getPosition().y(), 'g', PRECISION) : "");

	mSizeWLineEdit->setText(equalSize ? QString::number(first->getSize().width(), 'g', PRECISION) : "");
	mSizeHLineEdit->setText(equalSize ? QString::number(first->getSize().height(), 'g', PRECISION) : "");

	if (isPositiveWSize)
		mFlipXCheckBox->setCheckState(Qt::Unchecked);

	else if (isNegativeWSize)
		mFlipXCheckBox->setCheckState(Qt::Checked);
	else
		mFlipXCheckBox->setCheckState(Qt::PartiallyChecked);

	if (isPositiveHSize)
		mFlipYCheckBox->setCheckState(Qt::Unchecked);
	else if (isNegativeHSize)
		mFlipYCheckBox->setCheckState(Qt::Checked);
	else
		mFlipYCheckBox->setCheckState(Qt::PartiallyChecked);

	mRotationAngleComboBox->lineEdit()->setText(equalRotationAngle ? QString::number(first->getRotationAngle(), 'g', PRECISION) : "");

	mRotationCenterXLineEdit->setText(QString::number(rotationCenter.x(), 'g', PRECISION));
	mRotationCenterYLineEdit->setText(QString::number(rotationCenter.y(), 'g', PRECISION));
}

void PropertyWindow::updateSpriteWidgets(const QList<GameObject *> &objects)
{
	// флаги идентичности значений свойств объектов в группе
	bool equalFileName = true;
	bool equalSpriteColor = true;
	bool equalSpriteOpacity = true;

	Q_ASSERT(!objects.empty());

	Sprite *first = dynamic_cast<Sprite *>(objects.front());

	foreach (GameObject *obj, objects)
	{
		Sprite *it = dynamic_cast<Sprite *>(obj);

		if (it->getFileName() != first->getFileName())
			equalFileName = false;

		if (it->getColor().rgb() != first->getColor().rgb())
			equalSpriteColor = false;

		if (it->getColor().alpha() != first->getColor().alpha())
			equalSpriteOpacity = false;
	}

	mFileNameLineEdit->setText(equalFileName ? first->getFileName() : "");

	if (equalSpriteColor)
	{
		// FIXME:
		mSpriteColorFrame;
	}
	else
	{
		// FIXME:
		mSpriteColorFrame;
	}

	if (equalSpriteOpacity)
	{
		// FIXME:
		mSpriteOpacityHorizontalSlider;
		mSpriteOpacityLineEdit->setText(QString::number(first->getColor().alpha(), 'g', PRECISION));
	}
	else
	{
		// FIXME:
		mSpriteOpacityHorizontalSlider;
		mSpriteOpacityLineEdit->setText("");
	}
}

void PropertyWindow::updateLabelWidgets(const QList<GameObject *> &objects)
{
	// флаги идентичности значений свойств объектов в группе
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


	mTextLineEdit->setText(equalText ? first->getText() : "");

	mFileNameComboBox->lineEdit()->setText(equalFileName ? first->getFileName() : "");

	mFontSizeComboBox->lineEdit()->setText(equalFontSize ? QString::number(first->getFontSize(), 'g', PRECISION) : "");

	// деактивация кнопок выравнивания
	mAlignmentLeftPushButton->setDown(false);
	mAlignmentCenterPushButton->setDown(false);
	mAlignmentRightPushButton->setDown(false);
	// активация общей кнопоки выравнивания
	if (equalAlignment)
		switch (first->getAlignment())
		{
		case FTGL::ALIGN_LEFT:
			mAlignmentLeftPushButton->setDown(true);
			break;
		case FTGL::ALIGN_CENTER:
			mAlignmentCenterPushButton->setDown(true);
			break;
		case FTGL::ALIGN_RIGHT:
			mAlignmentRightPushButton->setDown(true);
			break;
		case FTGL::ALIGN_JUSTIFY:
			break;
		}

	// деактивация кнопок выравнивания по вертикали
	mVertAlignmentTopPushButton->setDown(false);
	mVertAlignmentCenterPushButton->setDown(false);
	mVertAlignmentBottomPushButton->setDown(false);
	// активация общей кнопоки выравнивания по вертикали
	if (equalVertAlignment)
		switch (first->getVertAlignment())
		{
		case Label::VERT_ALIGN_TOP:
			mVertAlignmentTopPushButton->setDown(true);
			break;
		case Label::VERT_ALIGN_CENTER:
			mVertAlignmentCenterPushButton->setDown(true);
			break;
		case Label::VERT_ALIGN_BOTTOM:
			mVertAlignmentBottomPushButton->setDown(true);
			break;
		}

	mLineSpacingComboBox->lineEdit()->setText(equalLineSpacing ? QString::number(first->getLineSpacing(), 'g', PRECISION) : "");

	if (equalLabelColor)
	{
		// FIXME:
		mLabelColorFrame;
	}
	else
	{
		// FIXME:
		mLabelColorFrame;
	}

	if (equalLabelOpacity)
	{
		// FIXME:
		mLabelOpacityHorizontalSlider;
		mLabelOpacityLineEdit->setText(QString::number(first->getColor().alpha(), 'g', PRECISION));
	}
	else
	{
		// FIXME:
		mLabelOpacityHorizontalSlider;
		mLabelOpacityLineEdit->setText("");
	}

}


void PropertyWindow::on_mNameLineEdit_editingFinished()
{
	qDebug() << "on_mNameLineEdit_editingFinished";
}

void PropertyWindow::on_mPositionXLineEdit_editingFinished()
{
	qDebug() << "on_mPositionXLineEdit_editingFinished";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mPositionYLineEdit_editingFinished()
{
	qDebug() << "on_mPositionYLineEdit_editingFinished";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mSizeWLineEdit_editingFinished()
{
	qDebug() << "on_mSizeWLineEdit_editingFinished";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mSizeHLineEdit_editingFinished()
{
	qDebug() << "on_mSizeHLineEdit_editingFinished";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mFlipXCheckBox_clicked(bool checked)
{
	qDebug() << "on_mFlipXCheckBox_clicked";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mFlipYCheckBox_clicked(bool checked)
{
	qDebug() << "on_mFlipYCheckBox_clicked";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::onRotationAngleEditingFinished()
{
	qDebug() << "onRotationAngleEditingFinished";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mRotationCenterXLineEdit_editingFinished()
{
	qDebug() << "on_mRotationCenterXLineEdit_editingFinished";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mRotationCenterYLineEdit_editingFinished()
{
	qDebug() << "on_mRotationCenterYLineEdit_editingFinished";

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mFileNameLineEdit_editingFinished()
{
	qDebug() << "on_mFileNameLineEdit_editingFinished";
}

void PropertyWindow::on_mFileNameBrowsePushButton_clicked()
{
	qDebug() << "on_mFileNameBrowsePushButton_clicked";
}

void PropertyWindow::on_mSpriteOpacityHorizontalSlider_sliderMoved(int position)
{
	qDebug() << "on_mSpriteOpacityHorizontalSlider_sliderMoved";
}
