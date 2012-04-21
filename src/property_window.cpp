#include "pch.h"
#include "property_window.h"
#include "game_object.h"
#include "label.h"
#include "project.h"
#include "sprite.h"
#include "utils.h"
#include <typeinfo>

PropertyWindow::PropertyWindow(QWidget *parent)
: QDockWidget(parent)
{
	setupUi(this);

	connect(mRotationAngleComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onRotationAngleEditingFinished()));
	connect(mFontSizeComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onFontSizeEditingFinished()));
	connect(mLineSpacingComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onLineSpacingEditingFinished()));

	// для обработки сигнала щелчка по QFrame-ам
	mSpriteColorFrame->installEventFilter(this);
	mLabelColorFrame->installEventFilter(this);

	// для объединения кнопок в группы
	mHorzAlignmentButtonGroup = new QButtonGroup(this);
	mVertAlignmentButtonGroup = new QButtonGroup(this);
	mHorzAlignmentButtonGroup->addButton(mHorzAlignmentLeftPushButton);
	mHorzAlignmentButtonGroup->addButton(mHorzAlignmentCenterPushButton);
	mHorzAlignmentButtonGroup->addButton(mHorzAlignmentRightPushButton);
	mVertAlignmentButtonGroup->addButton(mVertAlignmentTopPushButton);
	mVertAlignmentButtonGroup->addButton(mVertAlignmentCenterPushButton);
	mVertAlignmentButtonGroup->addButton(mVertAlignmentBottomPushButton);
	connect(mHorzAlignmentButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(onHorzAlignmentClicked(QAbstractButton *)));
	connect(mVertAlignmentButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(onVertAlignmentClicked(QAbstractButton *)));

	// по умолчанию окно свойств пустое
	mScrollArea->setVisible(false);
	mNoSelectionObjectsLabel->setVisible(true);

	// установка валидаторов
	mPositionXLineEdit->setValidator(new QDoubleValidator(-10000, +10000, 6, this));
	mPositionYLineEdit->setValidator(new QDoubleValidator(-10000, +10000, 6, this));
	mSizeWLineEdit->setValidator(new QDoubleValidator(1, +10000, 6, this));
	mSizeHLineEdit->setValidator(new QDoubleValidator(1, +10000, 6, this));
	mRotationAngleComboBox->setValidator(new QDoubleValidator(-360, +360, 6, this));
	mRotationCenterXLineEdit->setValidator(new QDoubleValidator(-10000, +10000, 6, this));
	mRotationCenterYLineEdit->setValidator(new QDoubleValidator(-10000, +10000, 6, this));
	mFontSizeComboBox->setValidator(new QIntValidator(1, +1000, this));
	mLineSpacingComboBox->setValidator(new QDoubleValidator(-100, +100, 6, this));

	// загрузка и отображение списка доступных шрифтов в комбобоксе
	scanFonts();
}

void PropertyWindow::onEditorWindowSelectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter)
{
	// принудительно снимаем фокус с активного виджета, если он находится в окне свойств
	// чтобы он выдал сигнал editingFinished для завершения редактирования и применения изменений
	QWidget *widgetWithFocus = QApplication::focusWidget();
	for (QWidget *widget = widgetWithFocus; widget != NULL; widget = widget->parentWidget())
		if (widget == this)
			widgetWithFocus->clearFocus();

	// сохранение выделенных объектов во внутренний список
	mSelectedObjects = objects;

	// сохранение исходного центра вращения во внутреннюю переменную
	mRotationCenter = rotationCenter;

	// нет выделенных объектов
	if (objects.empty())
	{
		// установка отсутствия свойств
		mScrollArea->setVisible(false);
		// показ надписи об отсутствии выделенных объектов
		mNoSelectionObjectsLabel->setVisible(true);
		return;
	}

	// показ свойств
	mScrollArea->setVisible(true);

	// скрытие надписи об отсутствии выделенных объектов
	mNoSelectionObjectsLabel->setVisible(false);

	// FIXME:
	// логика скрытия в зависимости от выбранного языка



	// FIXME:
/*
	if (mScrollAreaLayout->count() == 1)
	{
		;
	}
	else
	{
		// деактивируем все ячейки грида окна свойств
		for (int line = 0; line < mScrollAreaLayout->count(); ++line)
			setLayoutItemEnabled(mScrollAreaLayout->itemAt(line), false);

		// активируем нужные ячейки грида окна свойств
	}
*/
	// отображение общих свойств
	updateCommonWidgets();

	// определение идентичности типа выделенных объектов
	QStringList types;
	foreach (GameObject *object, objects)
		types.push_back(typeid(*object).name());
	if (types.count(types.front()) != types.size())
	{
		// если выделенные объекты имеют разные типы, скрываем специфичные свойства
		setSpriteWidgetsVisible(false);
		setLabelWidgetsVisible(false);
		return;
	}

	if (dynamic_cast<Sprite *>(objects.front()) != NULL)
	{
		// отображение свойств спрайтов
		setSpriteWidgetsVisible(true);
		setLabelWidgetsVisible(false);
		updateSpriteWidgets();
	}
	else if (dynamic_cast<Label *>(objects.front()) != NULL)
	{
		// отображение свойств надписей
		setLabelWidgetsVisible(true);
		setSpriteWidgetsVisible(false);
		updateLabelWidgets();
	}
	else
	{
		qWarning() << "Error can't find type of object";
	}

	// запрещаем дальнейшее сжатие последней колонки
	int width = qMax(mScrollAreaLayout->columnMinimumWidth(5), mScrollAreaLayout->cellRect(0, 5).width());
	mScrollAreaLayout->setColumnMinimumWidth(5, width);
}

void PropertyWindow::onEditorWindowObjectsChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter)
{
	// сохранение выделенных объектов во внутренний список
	mSelectedObjects = objects;

	// сохранение исходного центра вращения во внутреннюю переменную
	mRotationCenter = rotationCenter;

	// считывание заново общих свойств
	if (!objects.empty())
		updateCommonWidgets();
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
				QColorDialog colorDialog;
				colorDialog.setCurrentColor(mSpriteColorFrame->palette().color(QPalette::Window));

				// вызов диалогового окна выбора цвета
				if (colorDialog.exec() == colorDialog.Accepted)
				{
					QColor color = colorDialog.currentColor();

					// установка выбранного цвета ГУЮ
					QPalette palette = mSpriteColorFrame->palette();
					palette.setColor(QPalette::Window, color);
					mSpriteColorFrame->setPalette(palette);

					// установка выбранного цвета выбранным объектам
					foreach (GameObject *obj, mSelectedObjects)
					{
						Sprite *it = dynamic_cast<Sprite *>(obj);
						color.setAlpha(it->getColor().alpha());
						it->setColor(color);
					}
				}

				return true;
			}
			else if (object == mLabelColorFrame)
			{
				QColorDialog colorDialog;
				colorDialog.setCurrentColor(mLabelColorFrame->palette().color(QPalette::Window));

				// вызов диалогового окна выбора цвета
				if (colorDialog.exec() == colorDialog.Accepted)
				{
					QColor color = colorDialog.currentColor();

					// установка выбранного цвета ГУЮ
					QPalette palette = mLabelColorFrame->palette();
					palette.setColor(QPalette::Window, color);
					mLabelColorFrame->setPalette(palette);

					// установка выбранного цвета выбранным объектам
					foreach (GameObject *obj, mSelectedObjects)
					{
						Label *it = dynamic_cast<Label *>(obj);
						color.setAlpha(it->getColor().alpha());
						it->setColor(color);
					}
				}

				return true;
			}
		}
	}

	return QObject::eventFilter(object, event);
}

void PropertyWindow::on_mNameLineEdit_editingFinished()
{
	qDebug() << mNameLineEdit->text();

	Q_ASSERT(mSelectedObjects.size() == 1);

	if (mNameLineEdit->text() == "")
	{
		mNameLineEdit->setText(mSelectedObjects.first()->getName());
		return;
	}

	// FIXME: delete foreach
	foreach (GameObject *obj, mSelectedObjects)
		obj->setName(mNameLineEdit->text());
}

void PropertyWindow::on_mTextEditPushButton_clicked(bool checked)
{
	qDebug() << "on_mTextEditPushButton_clicked";

	// FIXME:
}

void PropertyWindow::on_mPositionXLineEdit_editingFinished()
{
	// просчет расположения старого центра вращения в процентах
	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	foreach (GameObject *obj, mSelectedObjects)
	{
		QPointF pos = obj->getPosition();
		pos.setX(mPositionXLineEdit->text().toFloat());
		obj->setPosition(pos);
	}

	// просчет нового расположения центра вращения по процентам
	mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);
	}

	// сохранение значения нового центра вращения в GUI
	mRotationCenterXLineEdit->setText(QString::number(mRotationCenter.x(), 'g', PRECISION));

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mPositionYLineEdit_editingFinished()
{
	// просчет расположения старого центра вращения в процентах
	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	foreach (GameObject *obj, mSelectedObjects)
	{
		QPointF pos = obj->getPosition();
		pos.setY(mPositionYLineEdit->text().toFloat());
		obj->setPosition(pos);
	}

	// просчет нового расположения центра вращения по процентам
	mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);
	}


	// сохранение значения нового центра вращения в GUI
	mRotationCenterYLineEdit->setText(QString::number(mRotationCenter.y(), 'g', PRECISION));

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mSizeWLineEdit_editingFinished()
{
	// входящее значение W должно быть > 0

	// просчет расположения старого центра вращения в процентах
	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	float newW = mSizeWLineEdit->text().toFloat();
	foreach (GameObject *obj, mSelectedObjects)
		obj->setSize(QSizeF(newW, obj->getSize().height()));

	// просчет нового расположения центра вращения по процентам
	mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);
	}

	// сохранение значения нового центра вращения в GUI
	mRotationCenterXLineEdit->setText(QString::number(mRotationCenter.x(), 'g', PRECISION));

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mSizeHLineEdit_editingFinished()
{
	// входящее значение H должно быть > 0

	// просчет расположения старого центра вращения в процентах
	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	float newH = mSizeHLineEdit->text().toFloat();
	foreach (GameObject *obj, mSelectedObjects)
		obj->setSize(QSizeF(obj->getSize().width(), newH));

	// просчет нового расположения центра вращения по процентам
	mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);
	}

	// сохранение значения нового центра вращения в GUI
	mRotationCenterYLineEdit->setText(QString::number(mRotationCenter.y(), 'g', PRECISION));

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mFlipXCheckBox_clicked(bool checked)
{
	// просчет расположения старого центра вращения в процентах
	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	foreach (GameObject *obj, mSelectedObjects)
	{
		QSizeF size = obj->getSize();
		size.setWidth(checked ? -qAbs(size.width()) : qAbs(size.width()));
		obj->setSize(size);
	}

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);
	}

	// сохранение значения нового центра вращения в GUI
	mRotationCenterXLineEdit->setText(QString::number(mRotationCenter.x(), 'g', PRECISION));

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mFlipYCheckBox_clicked(bool checked)
{
	// просчет расположения старого центра вращения в процентах
	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	foreach (GameObject *obj, mSelectedObjects)
	{
		QSizeF size = obj->getSize();
		size.setHeight(checked ? -qAbs(size.height()) : qAbs(size.height()));
		obj->setSize(size);
	}

	// просчет нового расположения центра вращения по процентам
	mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);
	}

	// сохранение значения нового центра вращения в GUI
	mRotationCenterYLineEdit->setText(QString::number(mRotationCenter.y(), 'g', PRECISION));

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::onRotationAngleEditingFinished()
{
	foreach (GameObject *obj, mSelectedObjects)
		obj->setRotationAngle(mRotationAngleComboBox->lineEdit()->text().toFloat());

	emit objectsChanged(mRotationCenter);

	// обновление окна общих свойств (позиция)
	updateCommonWidgets();
}

void PropertyWindow::on_mRotationAngleComboBox_currentIndexChanged(const QString &arg)
{
	if (arg == "")
		return;

	foreach (GameObject *obj, mSelectedObjects)
		obj->setRotationAngle(arg.toFloat());

	emit objectsChanged(mRotationCenter);

	// обновление окна общих свойств (позиция)
	updateCommonWidgets();
}

void PropertyWindow::on_mRotationCenterXLineEdit_editingFinished()
{
	float newRotationCenterX = mRotationCenterXLineEdit->text().toFloat();

	mRotationCenter = QPointF(newRotationCenterX, mRotationCenterYLineEdit->text().toFloat());

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mRotationCenterYLineEdit_editingFinished()
{
	float newRotationCenterY = mRotationCenterYLineEdit->text().toFloat();

	mRotationCenter = QPointF(mRotationCenterXLineEdit->text().toFloat(), newRotationCenterY);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);

	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mSpriteFileNameBrowsePushButton_clicked()
{
	// показываем диалоговое окно открытия файла
	QString fileName = getRootPath();
	if (mSpriteFileNameLineEdit->text() != "")
		fileName += mSpriteFileNameLineEdit->text();

	QString filter = "Файлы изображений (*.jpg *.jpeg *.png);;Все файлы (*)";

	fileName = QFileDialog::getOpenFileName(this, "Открыть", fileName, filter);
	if (!Utils::isFileNameValid(fileName, getSpritesPath(), this))
		return;

	// удаление из имени файла имени коренной директории
	QString relativePath = fileName.mid(getRootPath().size());

	// просчет расположения старого центра вращения в процентах
	QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

	// установка новоq текстуры объектам
	foreach (GameObject *obj, mSelectedObjects)
	{
		Sprite *it = dynamic_cast<Sprite *>(obj);
		it->setFileName(relativePath);
	}

	// просчет нового расположения центра вращения по процентам
	mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
		mSelectedObjects.front()->setRotationCenter(mRotationCenter);
	}

	// отображение относительного пути к спрайту в ГУИ
	mSpriteFileNameLineEdit->setText(relativePath);

	emit objectsChanged(mRotationCenter);

	// обновление окна общих свойств
	updateCommonWidgets();
}

void PropertyWindow::on_mSpriteOpacitySlider_valueChanged(int value)
{
	foreach (GameObject *obj, mSelectedObjects)
	{
		Sprite *it = dynamic_cast<Sprite *>(obj);
		QColor color = it->getColor();
		color.setAlpha(value);
		it->setColor(color);
	}

	mSpriteOpacityValueLabel->setText(QString::number(value * 100 / 255) + "%");
}

void PropertyWindow::on_mLabelFileNameComboBox_currentIndexChanged(const QString &arg)
{
	if (arg == "")
		return;

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setFileName(Project::getSingleton().getFontsDirectory() + arg);
	}
}

void PropertyWindow::onFontSizeEditingFinished()
{
	int fontSize = mFontSizeComboBox->lineEdit()->text().toInt();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setFontSize(fontSize);
	}
}

void PropertyWindow::on_mFontSizeComboBox_currentIndexChanged(const QString &arg)
{
	if (arg == "")
		return;

	int fontSize = arg.toInt();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setFontSize(fontSize);
	}
}

void PropertyWindow::onHorzAlignmentClicked(QAbstractButton *button)
{
	Label::HorzAlignment horzAlignment;

	if (button == mHorzAlignmentLeftPushButton)
	{
		mHorzAlignmentLeftPushButton->setChecked(true);
		horzAlignment = Label::HORZ_ALIGN_LEFT;
	}
	else if (button == mHorzAlignmentCenterPushButton)
	{
		mHorzAlignmentCenterPushButton->setChecked(true);
		horzAlignment = Label::HORZ_ALIGN_CENTER;
	}
	else if (button == mHorzAlignmentRightPushButton)
	{
		mHorzAlignmentRightPushButton->setChecked(true);
		horzAlignment = Label::HORZ_ALIGN_RIGHT;
	}

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setHorzAlignment(horzAlignment);
	}
}

void PropertyWindow::onVertAlignmentClicked(QAbstractButton *button)
{
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

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setVertAlignment(vertAlignment);
	}
}

void PropertyWindow::onLineSpacingEditingFinished()
{
	float lineSpacing = mLineSpacingComboBox->lineEdit()->text().toFloat();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setLineSpacing(lineSpacing);
	}
}

void PropertyWindow::on_mLineSpacingComboBox_currentIndexChanged(const QString &arg)
{
	if (arg == "")
		return;

	float lineSpacing = arg.toFloat();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setLineSpacing(lineSpacing);
	}
}

void PropertyWindow::on_mLabelOpacitySlider_valueChanged(int value)
{
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		QColor color = it->getColor();
		color.setAlpha(value);
		it->setColor(color);
	}

	mLabelOpacityValueLabel->setText(QString::number(value * 100 / 255) + "%");
}



void PropertyWindow::setSpriteWidgetsVisible(bool visible)
{
	setGridLayoutRowsVisible(mScrollAreaLayout, 7, 3, visible);
}

void PropertyWindow::setLabelWidgetsVisible(bool visible)
{
	setGridLayoutRowsVisible(mScrollAreaLayout, 10, 8, visible);
}

void PropertyWindow::setGridLayoutRowsVisible(QGridLayout *layout, int firstRow, int numRows, bool visible)
{
	// устанавливаем видимость для заданного диапазона строк
	for (int row = firstRow; row < firstRow + numRows; ++row)
		for (int col = 0; col < layout->columnCount(); ++col)
			setLayoutItemVisible(layout->itemAtPosition(row, col), visible);
}

void PropertyWindow::setLayoutItemVisible(QLayoutItem *item, bool visible)
{
	if (item != NULL)
	{
		if (item->widget() != NULL)
		{
			// устанавливаем видимость для виджета в элементе лейаута
			item->widget()->setVisible(visible);
		}
		else if (item->layout() != NULL)
		{
			// рекурсивно устанавливаем видимость для всех вложенных элементов лейаута
			for (int i = 0; i < item->layout()->count(); ++i)
				setLayoutItemVisible(item->layout()->itemAt(i), visible);
		}
	}
}

void PropertyWindow::setLayoutItemEnabled(QLayoutItem *item, bool enable)
{
	if (item != NULL)
	{
		if (item->widget() != NULL)
		{
			// устанавливаем видимость для виджета в элементе лейаута
			item->widget()->setEnabled(enable);
		}
		else if (item->layout() != NULL)
		{
			// рекурсивно устанавливаем видимость для всех вложенных элементов лейаута
			for (int i = 0; i < item->layout()->count(); ++i)
				setLayoutItemEnabled(item->layout()->itemAt(i), enable);
		}
	}
}

void PropertyWindow::updateCommonWidgets()
{
	// флаги идентичности значений свойств объектов в группе
	bool equalName = true;
	bool equalPosition = true;
	bool equalSize = true;
	bool isPositiveWSize = true;
	bool isNegativeWSize = true;
	bool isPositiveHSize = true;
	bool isNegativeHSize = true;
	bool equalRotationAngle = true;

	Q_ASSERT(!mSelectedObjects.empty());

	GameObject *first = mSelectedObjects.front();

	foreach (GameObject *obj, mSelectedObjects)
	{
		GameObject *it = obj;

		if (it->getName() != mSelectedObjects.front()->getName())
			equalName = false;

		if (!Utils::isEqual(it->getPosition().x(), first->getPosition().x()) ||
			!Utils::isEqual(it->getPosition().y(), first->getPosition().y()))
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

	// отображение id объекта(-ов)
	QString strId;
	foreach (GameObject *obj, mSelectedObjects)
	{
		if (obj == first)
			strId += QString::number(obj->getObjectID());
		else
			strId += ", " + QString::number(obj->getObjectID());
	}
	mObjectIDLineEdit->setText(strId);

	mPositionXLineEdit->setText(equalPosition ? QString::number(first->getPosition().x(), 'g', PRECISION) : "");
	mPositionYLineEdit->setText(equalPosition ? QString::number(first->getPosition().y(), 'g', PRECISION) : "");

	mSizeWLineEdit->setText(equalSize ? QString::number(qAbs(first->getSize().width()), 'g', PRECISION) : "");
	mSizeHLineEdit->setText(equalSize ? QString::number(qAbs(first->getSize().height()), 'g', PRECISION) : "");

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

	if (equalRotationAngle)
	{
		int index = mRotationAngleComboBox->findText(QString::number(first->getRotationAngle(), 'g', PRECISION));
		mRotationAngleComboBox->setCurrentIndex(index);
		if (index == -1)
			mRotationAngleComboBox->lineEdit()->setText(QString::number(first->getRotationAngle(), 'g', PRECISION));
	}
	else
	{
		mRotationAngleComboBox->setCurrentIndex(-1);
		mRotationAngleComboBox->lineEdit()->setText("");
	}

	mRotationCenterXLineEdit->setText(QString::number(mRotationCenter.x(), 'g', PRECISION));
	mRotationCenterYLineEdit->setText(QString::number(mRotationCenter.y(), 'g', PRECISION));
}

void PropertyWindow::updateSpriteWidgets()
{
	// флаги идентичности значений свойств объектов в группе
	bool equalFileName = true;
	bool equalSpriteColor = true;
	bool equalSpriteOpacity = true;

	Q_ASSERT(!mSelectedObjects.empty());

	Sprite *first = dynamic_cast<Sprite *>(mSelectedObjects.front());

	foreach (GameObject *obj, mSelectedObjects)
	{
		Sprite *it = dynamic_cast<Sprite *>(obj);

		if (it->getFileName() != first->getFileName())
			equalFileName = false;

		if (it->getColor().rgb() != first->getColor().rgb())
			equalSpriteColor = false;

		if (it->getColor().alpha() != first->getColor().alpha())
			equalSpriteOpacity = false;
	}

	mSpriteFileNameLineEdit->setText(equalFileName ? first->getFileName() : "");

	// установка текущего цвета
	QPalette palette = mSpriteColorFrame->palette();
	if (equalSpriteColor)
	{
		QColor color = first->getColor();
		color.setAlpha(255);
		palette.setColor(QPalette::Window, color);
	}
	else
	{
		palette.setColor(QPalette::Window, Qt::black);
	}
	mSpriteColorFrame->setPalette(palette);

	if (equalSpriteOpacity)
	{
		int alpha = first->getColor().alpha();
		mSpriteOpacitySlider->setSliderPosition(alpha);
		mSpriteOpacityValueLabel->setText(QString::number(alpha * 100 / 255) + "%");
	}
	else
	{
		mSpriteOpacitySlider->setSliderPosition(0);
		mSpriteOpacityValueLabel->setText("");
	}
}

void PropertyWindow::updateLabelWidgets()
{
	// флаги идентичности значений свойств объектов в группе
	bool equalText = true;
	bool equalFileName = true;
	bool equalFontSize = true;
	bool equalHorzAlignment = true;
	bool equalVertAlignment = true;
	bool equalLineSpacing = true;
	bool equalLabelColor = true;
	bool equalLabelOpacity = true;

	Q_ASSERT(!mSelectedObjects.empty());

	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);

		if (it->getText() != first->getText())
			equalText = false;

		if (it->getFileName() != first->getFileName())
			equalFileName = false;

		if (it->getFontSize() != first->getFontSize())
			 equalFontSize = false;

		if (it->getHorzAlignment() != first->getHorzAlignment())
			equalHorzAlignment = false;

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

	if (equalFileName)
	{
		int index = mLabelFileNameComboBox->findText(QFileInfo(first->getFileName()).fileName());
		mLabelFileNameComboBox->setCurrentIndex(index);
		if (index == -1)
			mLabelFileNameComboBox->lineEdit()->setText(QFileInfo(first->getFileName()).fileName());
	}
	else
	{
		mLabelFileNameComboBox->setCurrentIndex(-1);
	}

	if (equalFontSize)
	{
		int index = mFontSizeComboBox->findText(QString::number(first->getFontSize(), 10));
		mFontSizeComboBox->setCurrentIndex(index);
		if (index == -1)
			mLineSpacingComboBox->lineEdit()->setText(QString::number(first->getFontSize(), 10));
	}
	else
	{
		mFontSizeComboBox->setCurrentIndex(-1);
	}

	// деактивация кнопок выравнивания
	mHorzAlignmentButtonGroup->setExclusive(false);
	mHorzAlignmentLeftPushButton->setChecked(false);
	mHorzAlignmentCenterPushButton->setChecked(false);
	mHorzAlignmentRightPushButton->setChecked(false);
	// активация общей кнопоки выравнивания
	mHorzAlignmentButtonGroup->setExclusive(true);
	if (equalHorzAlignment)
		switch (first->getHorzAlignment())
		{
		case Label::HORZ_ALIGN_LEFT:
			mHorzAlignmentLeftPushButton->setChecked(true);
			break;
		case Label::HORZ_ALIGN_CENTER:
			mHorzAlignmentCenterPushButton->setChecked(true);
			break;
		case Label::HORZ_ALIGN_RIGHT:
			mHorzAlignmentRightPushButton->setChecked(true);
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

	if (equalLineSpacing)
	{
		int index = mLineSpacingComboBox->findText(QString::number(first->getLineSpacing()));
		mLineSpacingComboBox->setCurrentIndex(index);
		if (index == -1)
			mLineSpacingComboBox->lineEdit()->setText(QString::number(first->getLineSpacing()));
	}
	else
	{
		mLineSpacingComboBox->setCurrentIndex(-1);
	}

	// установка текущего цвета
	QPalette palette = mLabelColorFrame->palette();
	if (equalLabelColor)
	{
		QColor color = first->getColor();
		color.setAlpha(255);
		palette.setColor(QPalette::Window, color);
	}
	else
	{
		palette.setColor(QPalette::Window, Qt::black);
	}
	mLabelColorFrame->setPalette(palette);

	if (equalLabelOpacity)
	{
		int alpha = first->getColor().alpha();
		mLabelOpacitySlider->setSliderPosition(alpha);
		mLabelOpacityValueLabel->setText(QString::number(alpha * 100 / 255) + "%");
	}
	else
	{
		mLabelOpacitySlider->setSliderPosition(0);
		mLabelOpacityValueLabel->setText("");
	}
}

QRectF PropertyWindow::calculateCurrentBoundingRect()
{
	Q_ASSERT(!mSelectedObjects.empty());

	QRectF rect;
	if (!mSelectedObjects.empty())
	{
		// пересчитываем общий прямоугольник выделения
		rect = mSelectedObjects.front()->getBoundingRect();
		foreach (GameObject *object, mSelectedObjects)
			rect |= object->getBoundingRect();
	}

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

QString PropertyWindow::getRootPath() const
{
	return Project::getSingleton().getRootDirectory();
}

QString PropertyWindow::getSpritesPath() const
{
	return Project::getSingleton().getRootDirectory() + Project::getSingleton().getSpritesDirectory();
}

QString PropertyWindow::getFontsPath() const
{
	return Project::getSingleton().getRootDirectory() + Project::getSingleton().getFontsDirectory();
}

void PropertyWindow::scanFonts()
{
	// очистка списка комбобокса
	mLabelFileNameComboBox->clear();

	QDir currentDir = QDir(getFontsPath());
	currentDir.setFilter(QDir::Files);
	QStringList fileNameFilters;
	fileNameFilters << "*.ttf";
	currentDir.setNameFilters(fileNameFilters);
	QStringList listEntries = currentDir.entryList();

	mLabelFileNameComboBox->addItems(listEntries);
}
