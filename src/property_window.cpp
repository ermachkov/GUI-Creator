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

	connect(mFontSizeComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onFontSizeEditingFinished()));

	connect(mLineSpacingComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onLineSpacingEditingFinished()));

	// для обработки сигнала щелчка по QFrame-ам
	mSpriteColorFrame->installEventFilter(this);
	mLabelColorFrame->installEventFilter(this);

	// для объединения кнопок в группы
	mAlignmentButtonGroup = new QButtonGroup(this);
	mVertAlignmentButtonGroup = new QButtonGroup(this);
	mAlignmentButtonGroup->addButton(mAlignmentLeftPushButton);
	mAlignmentButtonGroup->addButton(mAlignmentCenterPushButton);
	mAlignmentButtonGroup->addButton(mAlignmentRightPushButton);
	mVertAlignmentButtonGroup->addButton(mVertAlignmentTopPushButton);
	mVertAlignmentButtonGroup->addButton(mVertAlignmentCenterPushButton);
	mVertAlignmentButtonGroup->addButton(mVertAlignmentBottomPushButton);
	connect(mAlignmentButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(onAlignmentClicked(QAbstractButton *)));
	connect(mVertAlignmentButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(onVertAlignmentClicked(QAbstractButton *)));

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
	// сохранение выделенных объектов во внутренний список
	mCurrentSelectedObjects = objects;

	// сохранение исходного центра вращения во внутреннюю переменную
	mRotationCenter = rotationCenter;

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
	// сохранение выделенных объектов во внутренний список
	mCurrentSelectedObjects = objects;

	// сохранение исходного центра вращения во внутреннюю переменную
	mRotationCenter = rotationCenter;

	// считывание общих свойств
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
	mAlignmentButtonGroup->setExclusive(false);
	mAlignmentLeftPushButton->setChecked(false);
	mAlignmentCenterPushButton->setChecked(false);
	mAlignmentRightPushButton->setChecked(false);
	// активация общей кнопоки выравнивания
	mAlignmentButtonGroup->setExclusive(true);
	if (equalAlignment)
		switch (first->getAlignment())
		{
		case FTGL::ALIGN_LEFT:
			mAlignmentLeftPushButton->setChecked(true);
			break;
		case FTGL::ALIGN_CENTER:
			mAlignmentCenterPushButton->setChecked(true);
			break;
		case FTGL::ALIGN_RIGHT:
			mAlignmentRightPushButton->setChecked(true);
			break;
		case FTGL::ALIGN_JUSTIFY:
			break;
		}

	// деактивация кнопок выравнивания по вертикали
	mVertAlignmentButtonGroup->setExclusive(false);
	mVertAlignmentTopPushButton->setChecked(false);
	mVertAlignmentCenterPushButton->setChecked(false);
	mVertAlignmentBottomPushButton->setChecked(false);
	// активация общей кнопоки выравнивания по вертикали
	mVertAlignmentButtonGroup->setExclusive(true);
	if (equalVertAlignment)
		switch (first->getVertAlignment())
		{
		case Label::VERT_ALIGN_TOP:
			mVertAlignmentTopPushButton->setChecked(true);
			break;
		case Label::VERT_ALIGN_CENTER:
			mVertAlignmentCenterPushButton->setChecked(true);
			break;
		case Label::VERT_ALIGN_BOTTOM:
			mVertAlignmentBottomPushButton->setChecked(true);
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
	foreach (GameObject *obj, mCurrentSelectedObjects)
		obj->setName(mNameLineEdit->text());
}

QRectF PropertyWindow::calculateCurrentBoundingRect()
{
	Q_ASSERT(!mCurrentSelectedObjects.empty());

	// пересчитываем общий прямоугольник выделения
	QRectF rect = mCurrentSelectedObjects.front()->getBoundingRect();
	foreach (GameObject *object, mCurrentSelectedObjects)
		rect |= object->getBoundingRect();
	return rect;
}

QPointF PropertyWindow::calculatePercentPosition(const QRectF &boundingRect, const QPointF &rotationCenter)
{
	return QPointF(
		(rotationCenter.x() - boundingRect.x()) / boundingRect.width(),
		(rotationCenter.y() - boundingRect.y()) / boundingRect.height());
}

QPointF PropertyWindow::calculatePosition(const QRectF &boundingRect, const QPointF &percentCenter)
{
	return QPointF(
		boundingRect.x() + boundingRect.width() * percentCenter.x(),
		boundingRect.y() + boundingRect.height() * percentCenter.y());
}

void PropertyWindow::on_mNameEditPushButton_clicked(bool checked)
{
	qDebug() << "on_mNameEditPushButton_clicked";

	// FIXME:
}

void PropertyWindow::on_mPositionXLineEdit_editingFinished()
{
	qDebug() << "on_mPositionXLineEdit_editingFinished";

	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		QPointF pos = obj->getPosition();
		pos.setX(mPositionXLineEdit->text().toFloat());
		obj->setPosition(pos);
	}

	mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

	// FIXME:
	// сохранение значения нового центра вращения в GUI
	// mRotationCenter

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mPositionYLineEdit_editingFinished()
{
	qDebug() << "on_mPositionYLineEdit_editingFinished";

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		QPointF pos = obj->getPosition();
		pos.setY(mPositionYLineEdit->text().toFloat());
		obj->setPosition(pos);
	}

	// FIXME:
	//emit objectsChanged();
}

void PropertyWindow::on_mSizeWLineEdit_editingFinished()
{
	qDebug() << "on_mSizeWLineEdit_editingFinished";

	foreach (GameObject *obj, mCurrentSelectedObjects)
		obj->setSize(QSizeF(mSizeWLineEdit->text().toFloat(), obj->getSize().height()));

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mSizeHLineEdit_editingFinished()
{
	qDebug() << "on_mSizeHLineEdit_editingFinished";

	foreach (GameObject *obj, mCurrentSelectedObjects)
		obj->setSize(QSizeF(obj->getSize().width(), mSizeHLineEdit->text().toFloat()));

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mFlipXCheckBox_clicked(bool checked)
{
	qDebug() << "on_mFlipXCheckBox_clicked";

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		QSizeF size = obj->getSize();
		size.setWidth(checked ? -qAbs(size.width()) : qAbs(size.width()));
		obj->setSize(size);
	}

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mFlipYCheckBox_clicked(bool checked)
{
	qDebug() << "on_mFlipYCheckBox_clicked";

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		QSizeF size = obj->getSize();
		size.setHeight(checked ? -qAbs(size.height()) : qAbs(size.height()));
		obj->setSize(size);
	}


	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::onRotationAngleEditingFinished()
{
	qDebug() << "onRotationAngleEditingFinished";

	foreach (GameObject *obj, mCurrentSelectedObjects)
		obj->setRotationAngle(mRotationAngleComboBox->lineEdit()->text().toFloat());

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mRotationCenterXLineEdit_editingFinished()
{
	qDebug() << "on_mRotationCenterXLineEdit_editingFinished";

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		QPointF rotationCenter = obj->getRotationCenter();
		rotationCenter.setX(mRotationCenterXLineEdit->text().toFloat());
		obj->setRotationCenter(rotationCenter);
	}

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mRotationCenterYLineEdit_editingFinished()
{
	qDebug() << "on_mRotationCenterYLineEdit_editingFinished";

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		QPointF rotationCenter = obj->getRotationCenter();
		rotationCenter.setY(mRotationCenterYLineEdit->text().toFloat());
		obj->setRotationCenter(rotationCenter);
	}

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mFileNameLineEdit_editingFinished()
{
	qDebug() << "on_mFileNameLineEdit_editingFinished";

	// просчет нового центра

	// FIXME:
	//emit objectsChanged()
}

void PropertyWindow::on_mFileNameBrowsePushButton_clicked()
{
	qDebug() << "on_mFileNameBrowsePushButton_clicked";
}

void PropertyWindow::on_mSpriteOpacityHorizontalSlider_sliderMoved(int position)
{
	qDebug() << "on_mSpriteOpacityHorizontalSlider_sliderMoved" << position;
}

void PropertyWindow::on_mSpriteOpacityLineEdit_editingFinished()
{

}

void PropertyWindow::on_mFileNameComboBox_currentIndexChanged(const QString &arg)
{
	qDebug() << "on_mFileNameComboBox_currentIndexChanged" << arg;
}

void PropertyWindow::onFontSizeEditingFinished()
{
	qDebug() << "onFontSizeEditingFinished";
}


void PropertyWindow::onAlignmentClicked(QAbstractButton *button)
{
	FTGL::TextAlignment alignment;

	if (button == mAlignmentLeftPushButton)
	{
		mAlignmentLeftPushButton->setChecked(true);
		alignment = FTGL::ALIGN_LEFT;
	}
	else if (button == mAlignmentCenterPushButton)
	{
		mAlignmentCenterPushButton->setChecked(true);
		alignment = FTGL::ALIGN_CENTER;
	}
	else if (button == mAlignmentRightPushButton)
	{
		mAlignmentRightPushButton->setChecked(true);
		alignment = FTGL::ALIGN_RIGHT;
	}
	//alignment = FTGL::ALIGN_JUSTIFY:

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it != NULL)
			it->setAlignment(alignment);
	}


}

void PropertyWindow::onVertAlignmentClicked(QAbstractButton *button)
{
	qDebug() << "onVertAlignmentClicked" << button;

	Label::VertAlignment vertAlignment;

	if (button == mVertAlignmentTopPushButton)
	{
		mVertAlignmentTopPushButton->setChecked(true);
		vertAlignment = Label::VERT_ALIGN_TOP;
	}
	else if (button == mVertAlignmentCenterPushButton)
	{
		mVertAlignmentCenterPushButton->setChecked(true);
		vertAlignment = Label::VERT_ALIGN_CENTER;
	}
	else if (button == mVertAlignmentBottomPushButton)
	{
		mVertAlignmentBottomPushButton->setChecked(true);
		vertAlignment = Label::VERT_ALIGN_BOTTOM;
	}

	foreach (GameObject *obj, mCurrentSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it != NULL)
			it->setVertAlignment(vertAlignment);
	}
}

void PropertyWindow::onLineSpacingEditingFinished()
{
	qDebug() << "onLineSpacingEditingFinished";
}

void PropertyWindow::on_mLabelOpacityHorizontalSlider_sliderMoved(int position)
{
	qDebug() << "on_mLabelOpacityHorizontalSlider_sliderMoved" << position;
}

void PropertyWindow::on_mLabelOpacityLineEdit_editingFinished()
{
	qDebug() << "on_mLabelOpacityLineEdit_editingFinished";
}
