#include "pch.h"
#include "property_window.h"
#include "label.h"
#include "layer.h"
#include "project.h"
#include "sprite.h"
#include "utils.h"

PropertyWindow::PropertyWindow(QWidget *parent)
: QDockWidget(parent)
{
	setupUi(this);

	// загрузка иконок кнопок
	mLockTextureSizeIcon = QIcon(":/images/size_locked.png");
	mUnlockTextureSizeIcon = QIcon(":/images/size_unlocked.png");

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
	mNoSelectedObjectsLabel->setVisible(true);
	mLocalizationWidget->setVisible(false);

	// установка валидаторов
	mPositionXLineEdit->setValidator(new PropertyDoubleValidator(-1000000.0, 1000000.0, PRECISION, this, &PropertyWindow::getCurrentPositionX));
	mPositionYLineEdit->setValidator(new PropertyDoubleValidator(-1000000.0, 1000000.0, PRECISION, this, &PropertyWindow::getCurrentPositionY));
	mSizeWLineEdit->setValidator(new PropertyDoubleValidator(1.0, 1000000.0, PRECISION, this, &PropertyWindow::getCurrentSizeW));
	mSizeHLineEdit->setValidator(new PropertyDoubleValidator(1.0, 1000000.0, PRECISION, this, &PropertyWindow::getCurrentSizeH));
	mRotationAngleComboBox->setValidator(new PropertyDoubleValidator(0.0, 359.999999, PRECISION, this, &PropertyWindow::getCurrentRotationAngle));
	mRotationCenterXLineEdit->setValidator(new PropertyDoubleValidator(-1000000.0, 1000000.0, PRECISION, this, &PropertyWindow::getCurrentRotationCenterX));
	mRotationCenterYLineEdit->setValidator(new PropertyDoubleValidator(-1000000.0, 1000000.0, PRECISION, this, &PropertyWindow::getCurrentRotationCenterY));
	mFontSizeComboBox->setValidator(new PropertyIntValidator(1, 1000, this, &PropertyWindow::getCurrentFontSize));
	mLineSpacingComboBox->setValidator(new PropertyDoubleValidator(0.0, 10.0, PRECISION, this, &PropertyWindow::getCurrentLineSpacing));

	// настройка свойств
	mLabelFileNameComboBox->lineEdit()->setPlaceholderText("Разные значения");
	mLabelFileNameComboBox->lineEdit()->setReadOnly(true);
	mFontSizeComboBox->lineEdit()->setPlaceholderText("Разные значения");
	mLineSpacingComboBox->lineEdit()->setPlaceholderText("Разные значения");

	// загрузка и отображение списка доступных шрифтов в комбобоксе
	scanFonts();
}

void PropertyWindow::clearChildWidgetFocus()
{
	// сбрасываем фокус с текущего активного виджета, если он находится в окне свойств
	QWidget *widgetWithFocus = QApplication::focusWidget();
	for (QWidget *widget = widgetWithFocus; widget != NULL; widget = widget->parentWidget())
		if (widget == this)
			widgetWithFocus->clearFocus();
}

void PropertyWindow::onEditorWindowSelectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter)
{
	// принудительно снимаем фокус с активного виджета, если он находится в окне свойств
	// чтобы он выдал сигнал editingFinished для завершения редактирования и применения изменений
	clearChildWidgetFocus();

	// сохранение выделенных объектов во внутренний список
	mSelectedObjects = objects;

	// сохранение исходного центра вращения во внутреннюю переменную
	mRotationCenter = rotationCenter;

	// целиком обновляем окно свойств
	updateWidgetsVisibleAndEnabled();

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

	// обновляем только общие свойства объектов
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
			if (object == mSpriteColorFrame && mSpriteColorFrame->isEnabled())
			{
				QColorDialog colorDialog;
				colorDialog.setCurrentColor(mSpriteColorFrame->palette().color(QPalette::Window));

				// вызов диалогового окна выбора цвета
				if (colorDialog.exec() == QColorDialog::Accepted)
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
			else if (object == mLabelColorFrame && mLabelColorFrame->isEnabled())
			{
				QColorDialog colorDialog;
				colorDialog.setCurrentColor(mLabelColorFrame->palette().color(QPalette::Window));

				// вызов диалогового окна выбора цвета
				if (colorDialog.exec() == QColorDialog::Accepted)
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
	Q_ASSERT(mSelectedObjects.size() == 1);

	// откатываем изменения, если имя пустое
	if (mNameLineEdit->text().isEmpty())
	{
		mNameLineEdit->setText(mSelectedObjects.front()->getName());
		return;
	}

	// изменяем имя объекта
	if (mNameLineEdit->text() != mSelectedObjects.front()->getName())
	{
		mSelectedObjects.front()->setName(mNameLineEdit->text());
		emit locationChanged("Изменение имени объекта");
	}
}

void PropertyWindow::on_mCopyIDPushButton_clicked()
{
	QApplication::clipboard()->setText(mObjectIDLineEdit->text());
}

void PropertyWindow::on_mPositionXLineEdit_editingFinished()
{
	setNewPosition();
}

void PropertyWindow::on_mPositionYLineEdit_editingFinished()
{
	setNewPosition();
}

void PropertyWindow::on_mSizeWLineEdit_editingFinished()
{
	setNewSize(true);
}

void PropertyWindow::on_mSizeHLineEdit_editingFinished()
{
	setNewSize(false);
}

void PropertyWindow::on_mLockSizePushButton_clicked()
{
	Q_ASSERT(mSelectedObjects.size() == 1);

	// инвертируем флаг блокировки изменения размеров
	Sprite *sprite = dynamic_cast<Sprite *>(mSelectedObjects.front());
	Q_ASSERT(sprite != NULL);
	bool sizeLocked = !sprite->isSizeLocked();
	sprite->setSizeLocked(sizeLocked);

	// обновляем гуи
	mSizeWLineEdit->setReadOnly(sizeLocked);
	mSizeHLineEdit->setReadOnly(sizeLocked);

	mLockSizePushButton->setIcon(sizeLocked ? mLockTextureSizeIcon : mUnlockTextureSizeIcon);

	// отправляем сигнал об изменении разрешенных операций редактирования
	emit allowedEditorActionsChanged();
}

void PropertyWindow::on_mFlipXCheckBox_clicked(bool checked)
{
	// TODO: поддержка множественного выделения
	Q_ASSERT(mSelectedObjects.size() == 1);

	// отражаем объект по горизонтали относительно центра
	GameObject *obj = mSelectedObjects.front();
	QSizeF size = obj->getSize();
	qreal angle = Utils::degToRad(obj->getRotationAngle());
	QPointF delta(size.width() * qCos(angle), size.width() * qSin(angle));
	obj->setPosition(obj->getPosition() + delta);
	obj->setSize(QSizeF(-size.width(), size.height()));
	mRotationCenter = obj->getRotationCenter();

	// обновляем гуи
	mPositionXLineEdit->setText(getCurrentPositionX());
	mPositionYLineEdit->setText(getCurrentPositionY());
	mRotationCenterXLineEdit->setText(getCurrentRotationCenterX());
	mRotationCenterYLineEdit->setText(getCurrentRotationCenterY());

	// посылаем сигналы об изменении локации, слоев и объектов
	emitLocationAndLayerChangedSignals("Отражение объектов по горизонтали");
	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::on_mFlipYCheckBox_clicked(bool checked)
{
	// TODO: поддержка множественного выделения
	Q_ASSERT(mSelectedObjects.size() == 1);

	// отражаем объект по вертикали относительно центра
	GameObject *obj = mSelectedObjects.front();
	QSizeF size = obj->getSize();
	qreal angle = Utils::degToRad(obj->getRotationAngle());
	QPointF delta(-size.height() * qSin(angle), size.height() * qCos(angle));
	obj->setPosition(obj->getPosition() + delta);
	obj->setSize(QSizeF(size.width(), -size.height()));
	mRotationCenter = obj->getRotationCenter();

	// обновляем гуи
	mPositionXLineEdit->setText(getCurrentPositionX());
	mPositionYLineEdit->setText(getCurrentPositionY());
	mRotationCenterXLineEdit->setText(getCurrentRotationCenterX());
	mRotationCenterYLineEdit->setText(getCurrentRotationCenterY());

	// посылаем сигналы об изменении локации, слоев и объектов
	emitLocationAndLayerChangedSignals("Отражение объектов по вертикали");
	emit objectsChanged(mRotationCenter);
}

void PropertyWindow::onRotationAngleEditingFinished()
{
	// TODO: поддержка множественного выделения
	Q_ASSERT(mSelectedObjects.size() == 1);

	// проверяем, что юзер действительно ввел новое значение
	qreal oldAngle = getCurrentRotationAngle().toDouble();
	qreal newAngle = mRotationAngleComboBox->lineEdit()->text().toDouble();
	if (newAngle != oldAngle)
	{
		// поворачиваем объект вокруг центра вращения на разницу между новым и старым углом
		GameObject *obj = mSelectedObjects.front();
		qreal angle = Utils::degToRad(newAngle - oldAngle);
		QPointF vec = obj->getPosition() - mRotationCenter;
		obj->setPosition(QPointF(vec.x() * qCos(angle) - vec.y() * qSin(angle) + mRotationCenter.x(),
			vec.x() * qSin(angle) + vec.y() * qCos(angle) + mRotationCenter.y()));
		obj->setRotationAngle(newAngle);

		// посылаем сигналы об изменении локации, слоев и объектов
		emitLocationAndLayerChangedSignals("Изменение угла поворота объектов");
		emit objectsChanged(mRotationCenter);
	}

	// обновляем гуи
	mPositionXLineEdit->setText(getCurrentPositionX());
	mPositionYLineEdit->setText(getCurrentPositionY());
	int index = mRotationAngleComboBox->findText(getCurrentRotationAngle());
	mRotationAngleComboBox->setCurrentIndex(index);
	if (index == -1)
		mRotationAngleComboBox->lineEdit()->setText(getCurrentRotationAngle());
}

void PropertyWindow::on_mRotationAngleComboBox_activated(const QString &text)
{
	onRotationAngleEditingFinished();
}

void PropertyWindow::on_mRotationCenterXLineEdit_editingFinished()
{
	setNewRotationCenter();
}

void PropertyWindow::on_mRotationCenterYLineEdit_editingFinished()
{
	setNewRotationCenter();
}

void PropertyWindow::on_mResetRotationCenterPushButton_clicked()
{
	Q_ASSERT(!mSelectedObjects.empty());

	// определяем координаты центра выделения
	QPointF center;
	if (mSelectedObjects.size() == 1)
	{
		mSelectedObjects.front()->resetRotationCenter();
		center = mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		center = calculateCurrentBoundingRect().center();
	}

	// сохранение значения нового центра вращения в GUI
	mRotationCenterXLineEdit->setText(QString::number(center.x(), 'g', PRECISION));
	mRotationCenterYLineEdit->setText(QString::number(center.y(), 'g', PRECISION));

	// устанавливаем новый центр вращения
	setNewRotationCenter();
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

	// обновление значения нового центра вращения для объекта, если он один
	if (mSelectedObjects.size() == 1)
	{
		mRotationCenter == mSelectedObjects.front()->getRotationCenter();
	}
	else
	{
		// просчет нового расположения центра вращения по процентам
		mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);
	}

	// отображение относительного пути к спрайту в ГУИ
	mSpriteFileNameLineEdit->setText(relativePath);

	emit objectsChanged(mRotationCenter);

	// обновление окна общих свойств
	updateCommonWidgets();
}

void PropertyWindow::on_mSpriteOpacitySlider_actionTriggered(int action)
{
	if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
	{
		int position = mSpriteOpacitySlider->sliderPosition();
		on_mSpriteOpacitySlider_sliderMoved(position);
	}
}

void PropertyWindow::on_mSpriteOpacitySlider_sliderMoved(int position)
{
	foreach (GameObject *obj, mSelectedObjects)
	{
		Sprite *it = dynamic_cast<Sprite *>(obj);
		QColor color = it->getColor();
		color.setAlpha(position);
		it->setColor(color);
	}

	mSpriteOpacityValueLabel->setText(QString::number(position * 100 / 255) + "%");
}

void PropertyWindow::on_mTextLineEdit_editingFinished()
{
	QString text = mTextLineEdit->text();

	// смена последовательности \n на '13'
	text.replace("\\n", "\n");

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setText(text);
	}
}

void PropertyWindow::on_mTextEditPushButton_clicked()
{
	// считывание текста из GUI
	QString text = mTextLineEdit->text();

	// смена последовательности \n на '13'
	text.replace("\\n", "\n");

	//create a custom dialog box with multi line input
	QDialog *textDialog = new QDialog(this);

	//local variable
	QVBoxLayout *vBoxLayout = new QVBoxLayout(textDialog);

	//class variable
	QPlainTextEdit *multilinePlainTextEdit = new QPlainTextEdit();
	multilinePlainTextEdit->setPlainText(text);

	vBoxLayout->addWidget(multilinePlainTextEdit);
	QDialogButtonBox *buttonBox = new QDialogButtonBox();
	buttonBox->setOrientation(Qt::Horizontal);
	buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

	vBoxLayout->addWidget(buttonBox);

	connect(buttonBox, SIGNAL(accepted()), textDialog, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), textDialog, SLOT(reject()));

	textDialog->setModal(true);

	if (textDialog->exec()== QDialog::Accepted)
	{
		text = multilinePlainTextEdit->toPlainText();

		foreach (GameObject *obj, mSelectedObjects)
		{
			Label *it = dynamic_cast<Label *>(obj);
			it->setText(text);
		}

		// смена '13' на последовательность \n
		text.replace("\n", "\\n");

		mTextLineEdit->setText(text);
	}

	delete textDialog;
}

void PropertyWindow::on_mLabelFileNameComboBox_activated(const QString &arg)
{
   qDebug() << "on_mLabelFileNameComboBox_activated" << arg;

   foreach (GameObject *obj, mSelectedObjects)
   {
	   Label *it = dynamic_cast<Label *>(obj);
	   it->setFileName(Project::getSingleton().getFontsDirectory() + arg);
   }
}

void PropertyWindow::onFontSizeEditingFinished()
{
	qDebug() << "on_mPositionXLineEdit_editingFinished";

	int fontSize = mFontSizeComboBox->lineEdit()->text().toInt();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setFontSize(fontSize);
	}
}

void PropertyWindow::on_mFontSizeComboBox_activated(const QString &arg)
{
	qDebug() << "on_mFontSizeComboBox_activated" << arg;

	int fontSize = arg.toInt();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setFontSize(fontSize);
	}
}

void PropertyWindow::onHorzAlignmentClicked(QAbstractButton *button)
{
	Label::HorzAlignment horzAlignment = Label::HORZ_ALIGN_LEFT;

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
	Label::VertAlignment vertAlignment = Label::VERT_ALIGN_TOP;

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
	qDebug() << "onLineSpacingEditingFinished";

	float lineSpacing = mLineSpacingComboBox->lineEdit()->text().toDouble();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setLineSpacing(lineSpacing);
	}
}

void PropertyWindow::on_mLineSpacingComboBox_activated(const QString &arg)
{
	qDebug() << "on_mLineSpacingComboBox_activated";

	qreal lineSpacing = arg.toDouble();

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		it->setLineSpacing(lineSpacing);
	}
}

void PropertyWindow::on_mLabelOpacitySlider_actionTriggered(int action)
{
	if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
	{
		int position = mSpriteOpacitySlider->sliderPosition();
		on_mLabelOpacitySlider_sliderMoved(position);
	}
}

void PropertyWindow::on_mLabelOpacitySlider_sliderMoved(int position)
{
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		QColor color = it->getColor();
		color.setAlpha(position);
		it->setColor(color);
	}

	mLabelOpacityValueLabel->setText(QString::number(position * 100 / 255) + "%");
}

void PropertyWindow::on_mLocalizationPushButton_clicked()
{
	foreach (GameObject *object, mSelectedObjects)
		if (!object->isLocalized())
			object->setLocalized(true);

	// FIXME: не забыть про дубликат в onEditorWindowSelectionChanged
	updateWidgetsVisibleAndEnabled();

	// отправляем сигнал об изменении разрешенных операций редактирования
	emit allowedEditorActionsChanged();
}

void PropertyWindow::updateWidgetsVisibleAndEnabled()
{
	// invisible localization button
	mLocalizationWidget->setVisible(false);

	if (mSelectedObjects.empty())
	{
		// invisible property window
		mScrollArea->setVisible(false);
		// visible noselection text
		mNoSelectedObjectsLabel->setVisible(true);
		return;
	}

	// visible property window
	mScrollArea->setVisible(true);

	// invisible noselection text
	mNoSelectedObjectsLabel->setVisible(false);

	QString currentLanguage = Project::getSingleton().getCurrentLanguage();
	QString defaultLanguage = Project::getSingleton().getDefaultLanguage();

	// определение идентичности типа выделенных объектов
	QStringList types;
	foreach (GameObject *object, mSelectedObjects)
		types.push_back(typeid(*object).name());
	bool isAllOneType = types.count(types.front()) == types.size();

	// определение локализованы ли все выделенные объекты или нет
	bool isAllLocalized = true;
	foreach (GameObject *object, mSelectedObjects)
		isAllLocalized = isAllLocalized && object->isLocalized();

	if (currentLanguage == defaultLanguage)
	{
		// enable all properties
		setGridLayoutRowsEnabled(mScrollAreaLayout, 0, mScrollAreaLayout->rowCount(), true);
	}
	else
	{
		if (isAllLocalized)
		{
			// dissable all
			setGridLayoutRowsEnabled(mScrollAreaLayout, 0, mScrollAreaLayout->rowCount(), false);

			if (isAllOneType)
			{
				// - pos, size, flip
				setGridLayoutRowsEnabled(mScrollAreaLayout, 2, 3, true);

				if (dynamic_cast<Sprite *>(mSelectedObjects.front()) != NULL)
				{
					// - fileName(Sprite)
					setGridLayoutRowsEnabled(mScrollAreaLayout, 7, 1, true);
				}
				else if (dynamic_cast<Label *>(mSelectedObjects.front()) != NULL)
				{
					// - fileName(Label), fontSize
					setGridLayoutRowsEnabled(mScrollAreaLayout, 12, 2, true);
				}
				else
				{
					qWarning() << "Error can't find type of object";
				}
			}
			else
			{
				// - pos, size
				setGridLayoutRowsEnabled(mScrollAreaLayout, 2, 2, true);
			}
		}
		else
		{
			// invisible property window
			mScrollArea->setVisible(false);
			// visible localization button
			mLocalizationWidget->setVisible(true);
			return;
		}
	}

	// invisible all properties
	setGridLayoutRowsVisible(mScrollAreaLayout, 0, mScrollAreaLayout->rowCount(), false);

	// проверяем, есть ли спрайты в выделении
	bool hasSprites = false;
	foreach (GameObject *obj, mSelectedObjects)
		if (dynamic_cast<Sprite *>(obj) != NULL)
			hasSprites = true;

	// visible common properties
	if (mSelectedObjects.size() > 1)
	{
		// visible common properties for multi selection
		setGridLayoutRowsVisible(mScrollAreaLayout, 2, 2, true);
		setGridLayoutRowsVisible(mScrollAreaLayout, 6, 1, true);

		// если выделение содержит хотя бы один спрайт, то деактивируем кнопку блокировки размера, иначе скрываем ее
		if (hasSprites)
			mLockSizePushButton->setEnabled(false);
		else
			mLockSizePushButton->setVisible(false);
	}
	else
	{
		// visible common properties for single selection
		setGridLayoutRowsVisible(mScrollAreaLayout, 0, 7, true);

		// скрываем кнопку блокировки размера, если выделен не спрайт
		if (!hasSprites)
			mLockSizePushButton->setVisible(false);
	}
	updateCommonWidgets();

	if (!isAllOneType)
	{
		// выделенные объекты имеют разные типы
		return;
	}

	if (dynamic_cast<Sprite *>(mSelectedObjects.front()) != NULL)
	{
		// visible Sprite properties
		setGridLayoutRowsVisible(mScrollAreaLayout, 7, 4, true);
		updateSpriteWidgets();
	}
	else if (dynamic_cast<Label *>(mSelectedObjects.front()) != NULL)
	{
		// visible Label properties
		setGridLayoutRowsVisible(mScrollAreaLayout, 11, 8, true);
		updateLabelWidgets();
	}
	else
	{
		qWarning() << "Error can't find type of object";
	}
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

void PropertyWindow::setGridLayoutRowsEnabled(QGridLayout *layout, int firstRow, int numRows, bool enabled)
{
	// устанавливаем активность для заданного диапазона строк
	for (int row = firstRow; row < firstRow + numRows; ++row)
		for (int col = 0; col < layout->columnCount(); ++col)
			setLayoutItemEnabled(layout->itemAtPosition(row, col), enabled);
}

void PropertyWindow::setLayoutItemEnabled(QLayoutItem *item, bool enabled)
{
	if (item != NULL)
	{
		if (item->widget() != NULL)
		{
			// устанавливаем активность для виджета в элементе лейаута
			item->widget()->setEnabled(enabled);
		}
		else if (item->layout() != NULL)
		{
			// рекурсивно устанавливаем активность для всех вложенных элементов лейаута
			for (int i = 0; i < item->layout()->count(); ++i)
				setLayoutItemEnabled(item->layout()->itemAt(i), enabled);
		}
	}
}

QString PropertyWindow::getCurrentPositionX() const
{
	return QString::number(mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getPosition().x() : calculateCurrentBoundingRect().x(), 'g', PRECISION);
}

QString PropertyWindow::getCurrentPositionY() const
{
	return QString::number(mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getPosition().y() : calculateCurrentBoundingRect().y(), 'g', PRECISION);
}

QString PropertyWindow::getCurrentSizeW() const
{
	return QString::number(qAbs(mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getSize().width() : calculateCurrentBoundingRect().width()), 'g', PRECISION);
}

QString PropertyWindow::getCurrentSizeH() const
{
	return QString::number(qAbs(mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getSize().height() : calculateCurrentBoundingRect().height()), 'g', PRECISION);
}

QString PropertyWindow::getCurrentRotationAngle() const
{
	// TODO: поддержка множественного выделения
	return mSelectedObjects.size() == 1 ? QString::number(mSelectedObjects.front()->getRotationAngle(), 'g', PRECISION) : "";
}

QString PropertyWindow::getCurrentRotationCenterX() const
{
	return QString::number(mRotationCenter.x(), 'g', PRECISION);
}

QString PropertyWindow::getCurrentRotationCenterY() const
{
	return QString::number(mRotationCenter.y(), 'g', PRECISION);
}

QString PropertyWindow::getCurrentFontSize() const
{
	bool equalFontSize = true;
	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it->getFontSize() != first->getFontSize())
			equalFontSize = false;
	}
	return equalFontSize ? QString::number(first->getFontSize()) : "";
}

QString PropertyWindow::getCurrentLineSpacing() const
{
	bool equalLineSpacing = true;
	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it->getLineSpacing() != first->getLineSpacing())
			equalLineSpacing = false;
	}
	return equalLineSpacing ? QString::number(first->getLineSpacing()) : "";
}

void PropertyWindow::setNewPosition()
{
	Q_ASSERT(!mSelectedObjects.empty());

	// проверяем, что юзер действительно ввел новые значения
	QPointF oldPosition(getCurrentPositionX().toDouble(), getCurrentPositionY().toDouble());
	QPointF newPosition(mPositionXLineEdit->text().toDouble(), mPositionYLineEdit->text().toDouble());
	if (newPosition != oldPosition)
	{
		if (mSelectedObjects.size() == 1)
		{
			// одиночное выделение
			GameObject *obj = mSelectedObjects.front();
			obj->setPosition(newPosition);
			mRotationCenter = obj->getRotationCenter();
		}
		else
		{
			// множественное выделение
			QRectF currentBoundingRect = calculateCurrentBoundingRect();

			// просчет смещения
			QPointF delta = newPosition - currentBoundingRect.topLeft();
			foreach (GameObject *obj, mSelectedObjects)
				obj->setPosition(obj->getPosition() + delta);

			// просчет нового расположения центра вращения
			mRotationCenter += delta;
		}

		// посылаем сигналы об изменении локации, слоев и объектов
		emitLocationAndLayerChangedSignals("Изменение позиции объектов");
		emit objectsChanged(mRotationCenter);
	}

	// обновляем гуи
	mPositionXLineEdit->setText(getCurrentPositionX());
	mPositionYLineEdit->setText(getCurrentPositionY());
	mRotationCenterXLineEdit->setText(getCurrentRotationCenterX());
	mRotationCenterYLineEdit->setText(getCurrentRotationCenterY());
}

void PropertyWindow::setNewSize(bool widthChanged)
{
	Q_ASSERT(!mSelectedObjects.empty());

	// проверяем, что юзер действительно ввел новые значения
	QSizeF oldSize(getCurrentSizeW().toDouble(), getCurrentSizeH().toDouble());
	QSizeF newSize(mSizeWLineEdit->text().toDouble(), mSizeHLineEdit->text().toDouble());
	if (newSize != oldSize)
	{
		if (mSelectedObjects.size() == 1)
		{
			// одиночное выделение
			GameObject *obj = mSelectedObjects.front();
			QSizeF size = obj->getSize();
			obj->setSize(QSizeF(Utils::sign(size.width()) * newSize.width(), Utils::sign(size.height()) * newSize.height()));
			mRotationCenter = obj->getRotationCenter();
		}
		else
		{
			// если хотя бы один повернут
			bool keepProportions = false;
			foreach (GameObject *obj, mSelectedObjects)
			{
				qreal angle = obj->getRotationAngle();
				if (angle != 0.0 && angle != 90.0 && angle != 180.0 && angle != 270.0)
					keepProportions = true;
			}

			// вычисляем коэффициент масштабирования
			QPointF scale(newSize.width() / oldSize.width(), newSize.height() / oldSize.height());
			if (keepProportions)
			{
				if (widthChanged)
					scale.ry() = scale.x();
				else
					scale.rx() = scale.y();
			}

			// пересчитываем положение и размеры выделенных объектов
			QRectF oldBoundingRect = calculateCurrentBoundingRect();
			foreach (GameObject *obj, mSelectedObjects)
			{
				// устанавливаем новую позицию
				QPointF position = obj->getPosition();
				obj->setPosition(QPointF(oldBoundingRect.left() + (position.x() - oldBoundingRect.left()) * scale.x(),
					oldBoundingRect.top() + (position.y() - oldBoundingRect.top()) * scale.y()));

				// устанавливаем новый размер
				QSizeF size = obj->getSize();
				qreal angle = obj->getRotationAngle();
				if (keepProportions || angle == 0.0 || angle == 180.0)
					obj->setSize(QSizeF(size.width() * scale.x(), size.height() * scale.y()));
				else
					obj->setSize(QSizeF(size.width() * scale.y(), size.height() * scale.x()));
			}

			// пересчитываем координаты центра вращения
			mRotationCenter = QPointF(oldBoundingRect.left() + (mRotationCenter.x() - oldBoundingRect.left()) * scale.x(),
				oldBoundingRect.top() + (mRotationCenter.y() - oldBoundingRect.top()) * scale.y());
		}

		// посылаем сигналы об изменении локации, слоев и объектов
		emitLocationAndLayerChangedSignals("Изменение размеров объектов");
		emit objectsChanged(mRotationCenter);
	}

	// обновляем гуи
	mPositionXLineEdit->setText(getCurrentPositionX());
	mPositionYLineEdit->setText(getCurrentPositionY());
	mSizeWLineEdit->setText(getCurrentSizeW());
	mSizeHLineEdit->setText(getCurrentSizeH());
	mRotationCenterXLineEdit->setText(getCurrentRotationCenterX());
	mRotationCenterYLineEdit->setText(getCurrentRotationCenterY());
}

void PropertyWindow::setNewRotationCenter()
{
	Q_ASSERT(!mSelectedObjects.empty());

	// проверяем, что юзер действительно ввел новые значения
	QPointF oldRotationCenter(getCurrentRotationCenterX().toDouble(), getCurrentRotationCenterY().toDouble());
	QPointF newRotationCenter(mRotationCenterXLineEdit->text().toDouble(), mRotationCenterYLineEdit->text().toDouble());
	if (newRotationCenter != oldRotationCenter)
	{
		// сохраняем координаты нового центра вращения
		mRotationCenter = newRotationCenter;

		// обновление значения нового центра вращения для объекта, если он один
		if (mSelectedObjects.size() == 1)
			mSelectedObjects.front()->setRotationCenter(mRotationCenter);

		// посылаем сигналы об изменении локации и объектов
		emit locationChanged("Изменение центра вращения");
		emit objectsChanged(mRotationCenter);
	}

	// обновляем гуи
	mRotationCenterXLineEdit->setText(getCurrentRotationCenterX());
	mRotationCenterYLineEdit->setText(getCurrentRotationCenterY());
}

void PropertyWindow::emitLocationAndLayerChangedSignals(const QString &commandName)
{
	// выдаем сигнал об изменении локации
	emit locationChanged(commandName);

	// формируем список измененных слоев
	QSet<BaseLayer *> layers;
	foreach (GameObject *object, mSelectedObjects)
		layers |= object->getParentLayer();

	// выдаем сигналы об изменении слоев
	foreach (BaseLayer *layer, layers)
		emit layerChanged(layer);
}

void PropertyWindow::updateCommonWidgets()
{
	Q_ASSERT(!mSelectedObjects.empty());

	GameObject *first = mSelectedObjects.front();

	bool sizeLocked = false;
	foreach (GameObject *obj, mSelectedObjects)
	{
		Sprite *sprite = dynamic_cast<Sprite *>(obj);
		if (sprite != NULL && sprite->isSizeLocked())
			sizeLocked = true;
	}

	// имя объекта
	mNameLineEdit->setText(first->getName());

	// id объекта
	mObjectIDLineEdit->setText(QString::number(first->getObjectID()));

	// позиция
	mPositionXLineEdit->setText(getCurrentPositionX());
	mPositionYLineEdit->setText(getCurrentPositionY());

	// размер
	mSizeWLineEdit->setText(getCurrentSizeW());
	mSizeHLineEdit->setText(getCurrentSizeH());
	mSizeWLineEdit->setReadOnly(sizeLocked);
	mSizeHLineEdit->setReadOnly(sizeLocked);

	// кнопка блокировки размера
	mLockSizePushButton->setIcon(sizeLocked ? mLockTextureSizeIcon : mUnlockTextureSizeIcon);

	// галочки отражения
	mFlipXCheckBox->setCheckState(first->getSize().width() >= 0.0 ? Qt::Unchecked : Qt::Checked);
	mFlipYCheckBox->setCheckState(first->getSize().height() >= 0.0 ? Qt::Unchecked : Qt::Checked);

	// угол поворота
	int index = mRotationAngleComboBox->findText(getCurrentRotationAngle());
	mRotationAngleComboBox->setCurrentIndex(index);
	if (index == -1)
		mRotationAngleComboBox->lineEdit()->setText(getCurrentRotationAngle());

	// центр вращения
	mRotationCenterXLineEdit->setText(getCurrentRotationCenterX());
	mRotationCenterYLineEdit->setText(getCurrentRotationCenterY());
}

void PropertyWindow::updateSpriteWidgets()
{
	// флаги идентичности значений свойств объектов в группе
	bool equalFileName = true;
	bool equalSpriteColor = true;
	bool equalSpriteOpacity = true;
	bool equalSpriteTextureSizeW = true;
	bool equalSpriteTextureSizeH = true;

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

		if (it->getTextureSize().width() != first->getTextureSize().width())
			equalSpriteTextureSizeW = false;

		if (it->getTextureSize().height() != first->getTextureSize().height())
			equalSpriteTextureSizeH = false;
	}

	// имя файла с текстурой
	mSpriteFileNameLineEdit->setText(equalFileName ? first->getFileName() : "");

	// размер текстуры
	mTextureSizeWLineEdit->setText(equalSpriteTextureSizeW ? QString::number(first->getTextureSize().width()) : "");
	mTextureSizeHLineEdit->setText(equalSpriteTextureSizeH ? QString::number(first->getTextureSize().height()) : "");

	// цвет
	QPalette palette = mSpriteColorFrame->palette();
	if (equalSpriteColor)
	{
		QColor color = first->getColor();
		color.setAlpha(255);
		palette.setColor(QPalette::Window, color);
	}
	else
	{
		palette.setColor(QPalette::Window, this->palette().color(QPalette::Window));
	}
	mSpriteColorFrame->setPalette(palette);

	// прозрачность
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
	bool equalHorzAlignment = true;
	bool equalVertAlignment = true;
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

		if (it->getHorzAlignment() != first->getHorzAlignment())
			equalHorzAlignment = false;

		if (it->getVertAlignment() != first->getVertAlignment())
			equalVertAlignment = false;

		if (it->getColor().rgb() != first->getColor().rgb())
			equalLabelColor = false;

		if (it->getColor().alpha() != first->getColor().alpha())
			equalLabelOpacity = false;
	}

	// текст надписи
	QString text = first->getText();
	text.replace("\n", "\\n");
	mTextLineEdit->setText(equalText ? text : "");

	// имя файла со шрифтом
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

	// размер шрифта
	int index = mFontSizeComboBox->findText(getCurrentFontSize());
	mFontSizeComboBox->setCurrentIndex(index);
	if (index == -1)
		mFontSizeComboBox->lineEdit()->setText(getCurrentFontSize());

	// горизонтальное выравнивание
	mHorzAlignmentButtonGroup->setExclusive(false);
	mHorzAlignmentLeftPushButton->setChecked(false);
	mHorzAlignmentCenterPushButton->setChecked(false);
	mHorzAlignmentRightPushButton->setChecked(false);
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

	// вертикальное выравнивание
	mVertAlignmentButtonGroup->setExclusive(false);
	mVertAlignmentTopPushButton->setChecked(false);
	mVertAlignmentCenterPushButton->setChecked(false);
	mVertAlignmentBottomPushButton->setChecked(false);
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

	// межстрочный интервал
	index = mLineSpacingComboBox->findText(getCurrentLineSpacing());
	mLineSpacingComboBox->setCurrentIndex(index);
	if (index == -1)
		mLineSpacingComboBox->lineEdit()->setText(getCurrentLineSpacing());

	// цвет
	QPalette palette = mLabelColorFrame->palette();
	if (equalLabelColor)
	{
		QColor color = first->getColor();
		color.setAlpha(255);
		palette.setColor(QPalette::Window, color);
	}
	else
	{
		palette.setColor(QPalette::Window, this->palette().color(QPalette::Window));
	}
	mLabelColorFrame->setPalette(palette);

	// прозрачность
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

QRectF PropertyWindow::calculateCurrentBoundingRect() const
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

PropertyWindow::PropertyIntValidator::PropertyIntValidator(int bottom, int top, PropertyWindow *parent, FixupFunc fixupFunc)
: QIntValidator(bottom, top, parent), mParent(parent), mFixupFunc(fixupFunc)
{
}

void PropertyWindow::PropertyIntValidator::fixup(QString &input) const
{
	input = (mParent->*mFixupFunc)();
}

PropertyWindow::PropertyDoubleValidator::PropertyDoubleValidator(double bottom, double top, int decimals, PropertyWindow *parent, FixupFunc fixupFunc)
: QDoubleValidator(bottom, top, decimals, parent), mParent(parent), mFixupFunc(fixupFunc)
{
	setNotation(StandardNotation);
}

void PropertyWindow::PropertyDoubleValidator::fixup(QString &input) const
{
	input = (mParent->*mFixupFunc)();
}
