#include "pch.h"
#include "property_window.h"
#include "layer.h"
#include "project.h"
#include "sprite.h"
#include "utils.h"

PropertyWindow::PropertyWindow(QWidget *parent)
: QDockWidget(parent), mOpacitySliderMoved(false)
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
				// вызов диалогового окна выбора цвета
				QColor color = QColorDialog::getColor(mSpriteColorFrame->palette().color(QPalette::Window), this);
				if (color.isValid() && color != mSpriteColorFrame->palette().color(QPalette::Window))
				{
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

					// посылаем сигналы об изменении сцены и слоев
					emitSceneAndLayerChangedSignals("Изменение цвета спрайтов");
				}

				return true;
			}
			else if (object == mLabelColorFrame && mLabelColorFrame->isEnabled())
			{
				// вызов диалогового окна выбора цвета
				QColor color = QColorDialog::getColor(mLabelColorFrame->palette().color(QPalette::Window), this);
				if (color.isValid() && color != mLabelColorFrame->palette().color(QPalette::Window))
				{
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

					// посылаем сигналы об изменении сцены и слоев
					emitSceneAndLayerChangedSignals("Изменение цвета надписей");
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
		emit sceneChanged("Изменение имени объекта");
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

	// посылаем сигналы об изменении сцены, слоев и объектов
	emitSceneAndLayerChangedSignals("Отражение объектов по горизонтали");
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

	// посылаем сигналы об изменении сцены, слоев и объектов
	emitSceneAndLayerChangedSignals("Отражение объектов по вертикали");
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

		// посылаем сигналы об изменении сцены, слоев и объектов
		emitSceneAndLayerChangedSignals("Изменение угла поворота объектов");
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
	QString fileName = getSpritesPath() + mSpriteFileNameLineEdit->text();
	QString filter = "Файлы изображений (*.jpg *.jpeg *.png);;Все файлы (*)";
	fileName = QFileDialog::getOpenFileName(this, "Открыть", fileName, filter);
	if (!Utils::isFileNameValid(fileName, getSpritesPath(), this))
		return;

	// проверяем, что юзер действительно ввел новое значение
	QString oldPath = Project::getSingleton().getSpritesDirectory() + mSpriteFileNameLineEdit->text();
	QString newPath = fileName.mid(getRootPath().size());
	if (newPath != oldPath)
	{
		// просчет расположения старого центра вращения в процентах
		QPointF percentCenter = calculatePercentPosition(calculateCurrentBoundingRect(), mRotationCenter);

		// установка новой текстуры объектам
		foreach (GameObject *obj, mSelectedObjects)
		{
			Sprite *it = dynamic_cast<Sprite *>(obj);
			it->setFileName(newPath);
		}

		// обновление значения нового центра вращения для объекта
		if (mSelectedObjects.size() == 1)
			mRotationCenter = mSelectedObjects.front()->getRotationCenter();
		else
			mRotationCenter = calculatePosition(calculateCurrentBoundingRect(), percentCenter);

		// обновляем гуи
		updateCommonWidgets();
		updateSpriteWidgets();

		// посылаем сигналы об изменении сцены, слоев и объектов
		emitSceneAndLayerChangedSignals("Изменение текстуры спрайтов");
		emit objectsChanged(mRotationCenter);
	}
}

void PropertyWindow::on_mSpriteOpacitySlider_sliderMoved(int value)
{
	// устанавливаем прозрачность всем выделенным объектам
	foreach (GameObject *obj, mSelectedObjects)
	{
		Sprite *it = dynamic_cast<Sprite *>(obj);
		QColor color = it->getColor();
		color.setAlpha(value);
		it->setColor(color);
	}

	// устанавливаем текст подписи
	mSpriteOpacityValueLabel->setText(QString::number(value * 100 / 255) + "%");

	// устанавливаем флаг перемещения ползунка
	mOpacitySliderMoved = true;
}

void PropertyWindow::on_mSpriteOpacitySlider_valueChanged(int value)
{
	// проверяем, что прозрачность действительно изменилась
	if (value != getCurrentSpriteOpacity() || mOpacitySliderMoved)
	{
		on_mSpriteOpacitySlider_sliderMoved(value);
		mOpacitySliderMoved = false;
		emitSceneAndLayerChangedSignals("Изменение прозрачности спрайтов");
	}
}

void PropertyWindow::on_mTextLineEdit_editingFinished()
{
	// откатываем изменения, если поле ввода пустое
	if (mTextLineEdit->text().isEmpty())
	{
		QString text = getCurrentText();
		text.replace("\n", "\\n");
		mTextLineEdit->setText(text);
		return;
	}

	// проверяем, что юзер действительно ввел новое значение
	QString oldText = getCurrentText();
	QString newText = mTextLineEdit->text();
	newText.replace("\\n", "\n");
	if (newText != oldText)
	{
		// устанавливаем новый текст для выделенных объектов
		foreach (GameObject *obj, mSelectedObjects)
		{
			Label *it = dynamic_cast<Label *>(obj);
			it->setText(newText);
		}

		// посылаем сигналы об изменении сцены и слоев
		emitSceneAndLayerChangedSignals("Изменение текста надписей");
	}
}

void PropertyWindow::on_mTextEditPushButton_clicked()
{
	// создаем новое диалоговое окно
	QDialog *textDialog = new QDialog(this);
	textDialog->setModal(true);

	// лейаут для диалогового окна
	QVBoxLayout *vBoxLayout = new QVBoxLayout(textDialog);

	// многострочное поле ввода
	mPlainTextEdit = new QPlainTextEdit(getCurrentText());
	vBoxLayout->addWidget(mPlainTextEdit);

	// кнопки OK/Cancel
	mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	vBoxLayout->addWidget(mDialogButtonBox);
	connect(mDialogButtonBox, SIGNAL(accepted()), textDialog, SLOT(accept()));
	connect(mDialogButtonBox, SIGNAL(rejected()), textDialog, SLOT(reject()));

	// разрешаем/запрещаем кнопку OK в зависимости от содержимого поля ввода
	connect(mPlainTextEdit, SIGNAL(textChanged()), this, SLOT(onPlainTextEditTextChanged()));
	onPlainTextEditTextChanged();

	// показываем диалоговое окно
	if (textDialog->exec() == QDialog::Accepted)
	{
		QString text = mPlainTextEdit->toPlainText();
		text.replace("\n", "\\n");
		mTextLineEdit->setText(text);
		on_mTextLineEdit_editingFinished();
	}

	// удаляем диалог по завершению ввода
	delete textDialog;
}

void PropertyWindow::onPlainTextEditTextChanged()
{
	// запрещаем кнопку OK, если юзер очистил поле ввода
	mDialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!mPlainTextEdit->toPlainText().isEmpty());
}

void PropertyWindow::on_mLabelFileNameComboBox_activated(const QString &text)
{
	// проверяем, что юзер действительно ввел новое значение
	QString oldPath = getCurrentLabelFileName();
	QString newPath = Project::getSingleton().getFontsDirectory() + text;
	if (newPath != oldPath)
	{
		// устанавливаем новый шрифт для выделенных объектов
		foreach (GameObject *obj, mSelectedObjects)
		{
			Label *it = dynamic_cast<Label *>(obj);
			it->setFileName(newPath);
		}

		// посылаем сигналы об изменении сцены и слоев
		emitSceneAndLayerChangedSignals("Изменение шрифта надписей");
	}
}

void PropertyWindow::onFontSizeEditingFinished()
{
	// проверяем, что юзер действительно ввел новое значение
	int oldFontSize = getCurrentFontSize().toInt();
	int newFontSize = mFontSizeComboBox->lineEdit()->text().toInt();
	if (newFontSize != oldFontSize)
	{
		// устанавливаем новый размер шрифта для выделенных объектов
		foreach (GameObject *obj, mSelectedObjects)
		{
			Label *it = dynamic_cast<Label *>(obj);
			it->setFontSize(newFontSize);
		}

		// посылаем сигналы об изменении сцены и слоев
		emitSceneAndLayerChangedSignals("Изменение размера шрифта надписей");
	}

	// обновляем гуи
	int index = mFontSizeComboBox->findText(getCurrentFontSize());
	mFontSizeComboBox->setCurrentIndex(index);
	if (index == -1)
		mFontSizeComboBox->lineEdit()->setText(getCurrentFontSize());
}

void PropertyWindow::on_mFontSizeComboBox_activated(const QString &text)
{
	onFontSizeEditingFinished();
}

void PropertyWindow::onHorzAlignmentClicked(QAbstractButton *button)
{
	// проверяем, что юзер действительно ввел новое значение
	bool equalHorzAlignment;
	Label::HorzAlignment oldHorzAlignment = getCurrentHorzAlignment(&equalHorzAlignment);
	Label::HorzAlignment newHorzAlignment;
	if (button == mHorzAlignmentLeftPushButton)
		newHorzAlignment = Label::HORZ_ALIGN_LEFT;
	else if (button == mHorzAlignmentCenterPushButton)
		newHorzAlignment = Label::HORZ_ALIGN_CENTER;
	else
		newHorzAlignment = Label::HORZ_ALIGN_RIGHT;
	if (!equalHorzAlignment || newHorzAlignment != oldHorzAlignment)
	{
		// устанавливаем новое горизонтальное выравнивание для выделенных объектов
		foreach (GameObject *obj, mSelectedObjects)
		{
			Label *it = dynamic_cast<Label *>(obj);
			it->setHorzAlignment(newHorzAlignment);
		}

		// посылаем сигналы об изменении сцены и слоев
		emitSceneAndLayerChangedSignals("Изменение выравнивания надписей");
	}
}

void PropertyWindow::onVertAlignmentClicked(QAbstractButton *button)
{
	// проверяем, что юзер действительно ввел новое значение
	bool equalVertAlignment;
	Label::VertAlignment oldVertAlignment = getCurrentVertAlignment(&equalVertAlignment);
	Label::VertAlignment newVertAlignment;
	if (button == mVertAlignmentTopPushButton)
		newVertAlignment = Label::VERT_ALIGN_TOP;
	else if (button == mVertAlignmentCenterPushButton)
		newVertAlignment = Label::VERT_ALIGN_CENTER;
	else
		newVertAlignment = Label::VERT_ALIGN_BOTTOM;
	if (!equalVertAlignment || newVertAlignment != oldVertAlignment)
	{
		// устанавливаем новое вертикальное выравнивание для выделенных объектов
		foreach (GameObject *obj, mSelectedObjects)
		{
			Label *it = dynamic_cast<Label *>(obj);
			it->setVertAlignment(newVertAlignment);
		}

		// посылаем сигналы об изменении сцены и слоев
		emitSceneAndLayerChangedSignals("Изменение выравнивания надписей");
	}
}

void PropertyWindow::onLineSpacingEditingFinished()
{
	// проверяем, что юзер действительно ввел новое значение
	qreal oldLineSpacing = getCurrentLineSpacing().toDouble();
	qreal newLineSpacing = mLineSpacingComboBox->lineEdit()->text().toDouble();
	if (newLineSpacing != oldLineSpacing)
	{
		// устанавливаем новый межстрочный интервал для выделенных объектов
		foreach (GameObject *obj, mSelectedObjects)
		{
			Label *it = dynamic_cast<Label *>(obj);
			it->setLineSpacing(newLineSpacing);
		}

		// посылаем сигналы об изменении сцены и слоев
		emitSceneAndLayerChangedSignals("Изменение межстрочного интервала надписей");
	}

	// обновляем гуи
	int index = mLineSpacingComboBox->findText(getCurrentLineSpacing());
	mLineSpacingComboBox->setCurrentIndex(index);
	if (index == -1)
		mLineSpacingComboBox->lineEdit()->setText(getCurrentLineSpacing());
}

void PropertyWindow::on_mLineSpacingComboBox_activated(const QString &text)
{
	onLineSpacingEditingFinished();
}

void PropertyWindow::on_mLabelOpacitySlider_sliderMoved(int value)
{
	// устанавливаем прозрачность всем выделенным объектам
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		QColor color = it->getColor();
		color.setAlpha(value);
		it->setColor(color);
	}

	// устанавливаем текст подписи
	mLabelOpacityValueLabel->setText(QString::number(value * 100 / 255) + "%");

	// устанавливаем флаг перемещения ползунка
	mOpacitySliderMoved = true;
}

void PropertyWindow::on_mLabelOpacitySlider_valueChanged(int value)
{
	// проверяем, что прозрачность действительно изменилась
	if (value != getCurrentLabelOpacity() || mOpacitySliderMoved)
	{
		on_mLabelOpacitySlider_sliderMoved(value);
		mOpacitySliderMoved = false;
		emitSceneAndLayerChangedSignals("Изменение прозрачности надписей");
	}
}

void PropertyWindow::on_mLocalizationPushButton_clicked()
{
	// создаем локализацию для всех еще не локализованных объектов
	foreach (GameObject *object, mSelectedObjects)
		if (!object->isLocalized())
			object->setLocalized(true);

	// целиком обновляем окно свойств
	updateWidgetsVisibleAndEnabled();

	// отправляем сигнал об изменении разрешенных операций редактирования
	emit allowedEditorActionsChanged();
}

void PropertyWindow::updateWidgetsVisibleAndEnabled()
{
	// скрыть кнопку локализации
	mLocalizationWidget->setVisible(false);

	if (mSelectedObjects.empty())
	{
		// скрыть окно свойств
		mScrollArea->setVisible(false);
		// показать надпись "нет выделения"
		mNoSelectedObjectsLabel->setVisible(true);
		return;
	}

	// показать окно свойств
	mScrollArea->setVisible(true);

	// скрыть надпись "нет выделения"
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
			// disable all properties
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
			// скрыть окно свойств
			mScrollArea->setVisible(false);
			// показать кнопку локализации
			mLocalizationWidget->setVisible(true);
			return;
		}
	}

	// скрыть
	setGridLayoutRowsVisible(mScrollAreaLayout, 0, mScrollAreaLayout->rowCount(), false);

	// проверяем, есть ли спрайты в выделении
	bool hasSprites = false;
	foreach (GameObject *obj, mSelectedObjects)
		if (dynamic_cast<Sprite *>(obj) != NULL)
			hasSprites = true;

	if (mSelectedObjects.size() > 1)
	{
		// множественное выделение
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
		// одиночное выделение
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
		// установка видимости свойтвам спрайта
		setGridLayoutRowsVisible(mScrollAreaLayout, 7, 4, true);
		updateSpriteWidgets();
	}
	else if (dynamic_cast<Label *>(mSelectedObjects.front()) != NULL)
	{
		// установка видимости свойтвам надписи
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

int PropertyWindow::getCurrentSpriteOpacity(bool *equal) const
{
	bool equalSpriteOpacity = true;
	Sprite *first = dynamic_cast<Sprite *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Sprite *it = dynamic_cast<Sprite *>(obj);
		if (it->getColor().alpha() != first->getColor().alpha())
			equalSpriteOpacity = false;
	}
	if (equal != NULL)
		*equal = equalSpriteOpacity;
	return equalSpriteOpacity ? first->getColor().alpha() : 0;
}

QString PropertyWindow::getCurrentText() const
{
	bool equalText = true;
	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it->getText() != first->getText())
			equalText = false;
	}
	return equalText ? first->getText() : "";
}

QString PropertyWindow::getCurrentLabelFileName() const
{
	bool equalLabelFileName = true;
	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it->getFileName() != first->getFileName())
			equalLabelFileName = false;
	}
	return equalLabelFileName ? first->getFileName() : "";
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

Label::HorzAlignment PropertyWindow::getCurrentHorzAlignment(bool *equal) const
{
	bool equalHorzAlignment = true;
	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it->getHorzAlignment() != first->getHorzAlignment())
			equalHorzAlignment = false;
	}
	if (equal != NULL)
		*equal = equalHorzAlignment;
	return equalHorzAlignment ? first->getHorzAlignment() : Label::HORZ_ALIGN_LEFT;
}

Label::VertAlignment PropertyWindow::getCurrentVertAlignment(bool *equal) const
{
	bool equalVertAlignment = true;
	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it->getVertAlignment() != first->getVertAlignment())
			equalVertAlignment = false;
	}
	if (equal != NULL)
		*equal = equalVertAlignment;
	return equalVertAlignment ? first->getVertAlignment() : Label::VERT_ALIGN_TOP;
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

int PropertyWindow::getCurrentLabelOpacity(bool *equal) const
{
	bool equalLabelOpacity = true;
	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());
	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);
		if (it->getColor().alpha() != first->getColor().alpha())
			equalLabelOpacity = false;
	}
	if (equal != NULL)
		*equal = equalLabelOpacity;
	return equalLabelOpacity ? first->getColor().alpha() : 0;
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

		// посылаем сигналы об изменении сцены, слоев и объектов
		emitSceneAndLayerChangedSignals("Изменение позиции объектов");
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

		// посылаем сигналы об изменении сцены, слоев и объектов
		emitSceneAndLayerChangedSignals("Изменение размеров объектов");
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

		// посылаем сигналы об изменении сцены и объектов
		emit sceneChanged("Изменение центра вращения");
		emit objectsChanged(mRotationCenter);
	}

	// обновляем гуи
	mRotationCenterXLineEdit->setText(getCurrentRotationCenterX());
	mRotationCenterYLineEdit->setText(getCurrentRotationCenterY());
}

void PropertyWindow::emitSceneAndLayerChangedSignals(const QString &commandName)
{
	// выдаем сигнал об изменении сцены
	emit sceneChanged(commandName);

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

		if (it->getTextureSize().width() != first->getTextureSize().width())
			equalSpriteTextureSizeW = false;

		if (it->getTextureSize().height() != first->getTextureSize().height())
			equalSpriteTextureSizeH = false;
	}

	// имя файла с текстурой
	QString fileName = equalFileName ? first->getFileName() : "";
	if (fileName.startsWith(Project::getSingleton().getSpritesDirectory()))
		fileName.remove(0, Project::getSingleton().getSpritesDirectory().size());
	mSpriteFileNameLineEdit->setText(fileName);

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
	bool equalSpriteOpacity;
	int alpha = getCurrentSpriteOpacity(&equalSpriteOpacity);
	mSpriteOpacitySlider->setValue(alpha);
	mSpriteOpacityValueLabel->setText(equalSpriteOpacity ? QString::number(alpha * 100 / 255) + "%" : "");
}

void PropertyWindow::updateLabelWidgets()
{
	// флаги идентичности значений свойств объектов в группе
	bool equalLabelColor = true;

	Q_ASSERT(!mSelectedObjects.empty());

	Label *first = dynamic_cast<Label *>(mSelectedObjects.front());

	foreach (GameObject *obj, mSelectedObjects)
	{
		Label *it = dynamic_cast<Label *>(obj);

		if (it->getColor().rgb() != first->getColor().rgb())
			equalLabelColor = false;
	}

	// текст надписи
	QString text = getCurrentText();
	text.replace("\n", "\\n");
	mTextLineEdit->setText(text);

	// имя файла со шрифтом
	QString fileName = getCurrentLabelFileName();
	if (fileName.startsWith(Project::getSingleton().getFontsDirectory()))
		fileName.remove(0, Project::getSingleton().getFontsDirectory().size());
	int index = mLabelFileNameComboBox->findText(fileName);
	mLabelFileNameComboBox->setCurrentIndex(index);
	if (index == -1)
		mLabelFileNameComboBox->lineEdit()->setText(fileName);

	// размер шрифта
	index = mFontSizeComboBox->findText(getCurrentFontSize());
	mFontSizeComboBox->setCurrentIndex(index);
	if (index == -1)
		mFontSizeComboBox->lineEdit()->setText(getCurrentFontSize());

	// горизонтальное выравнивание
	bool equalHorzAlignment;
	Label::HorzAlignment horzAlignment = getCurrentHorzAlignment(&equalHorzAlignment);
	if (equalHorzAlignment)
	{
		if (horzAlignment == Label::HORZ_ALIGN_LEFT)
			mHorzAlignmentLeftPushButton->setChecked(true);
		else if (horzAlignment == Label::HORZ_ALIGN_CENTER)
			mHorzAlignmentCenterPushButton->setChecked(true);
		else
			mHorzAlignmentRightPushButton->setChecked(true);
	}
	else
	{
		mHorzAlignmentButtonGroup->setExclusive(false);
		mHorzAlignmentLeftPushButton->setChecked(false);
		mHorzAlignmentCenterPushButton->setChecked(false);
		mHorzAlignmentRightPushButton->setChecked(false);
		mHorzAlignmentButtonGroup->setExclusive(true);
	}

	// вертикальное выравнивание
	bool equalVertAlignment;
	Label::VertAlignment vertAlignment = getCurrentVertAlignment(&equalVertAlignment);
	if (equalVertAlignment)
	{
		if (vertAlignment == Label::VERT_ALIGN_TOP)
			mVertAlignmentTopPushButton->setChecked(true);
		else if (vertAlignment == Label::VERT_ALIGN_CENTER)
			mVertAlignmentCenterPushButton->setChecked(true);
		else
			mVertAlignmentBottomPushButton->setChecked(true);
	}
	else
	{
		mVertAlignmentButtonGroup->setExclusive(false);
		mVertAlignmentTopPushButton->setChecked(false);
		mVertAlignmentCenterPushButton->setChecked(false);
		mVertAlignmentBottomPushButton->setChecked(false);
		mVertAlignmentButtonGroup->setExclusive(true);
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
	bool equalLabelOpacity;
	int alpha = getCurrentLabelOpacity(&equalLabelOpacity);
	mLabelOpacitySlider->setValue(alpha);
	mLabelOpacityValueLabel->setText(equalLabelOpacity ? QString::number(alpha * 100 / 255) + "%" : "");
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
