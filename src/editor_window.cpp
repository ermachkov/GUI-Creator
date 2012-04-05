#include "pch.h"
#include "editor_window.h"
#include "game_object.h"
#include "layer.h"
#include "location.h"
#include "options.h"
#include "project.h"
#include "utils.h"

EditorWindow::EditorWindow(QWidget *parent, QGLWidget *shareWidget, const QString &fileName, QWidget *spriteWidget, QWidget *fontWidget)
: QGLWidget(shareWidget->format(), parent, shareWidget), mFileName(fileName), mChanged(false), mSaved(false), mEditorState(STATE_IDLE),
  mCameraPos(0.0, 0.0), mZoom(1.0), mRotationMode(false), mSpriteWidget(spriteWidget), mFontWidget(fontWidget)
{
	// разрешаем события клавиатуры и перемещения мыши
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	// запрещаем очистку фона перед рисованием
	setAutoFillBackground(false);

	// разрешаем бросание объектов
	setAcceptDrops(true);

	// создаем курсор поворота
	mRotateCursor = QCursor(QPixmap(":/images/rotate_cursor.png"));

	// создаем новую локацию
	mLocation = new Location(this);
}

bool EditorWindow::load(const QString &fileName)
{
	if (mLocation->load(fileName))
	{
		// устанавливаем имя файла и флаги изменения/сохранения
		mFileName = fileName;
		mChanged = false;
		mSaved = true;
		return true;
	}

	return false;
}

bool EditorWindow::save(const QString &fileName)
{
	if (mLocation->save(fileName))
	{
		// устанавливаем имя файла и флаги изменения/сохранения
		mFileName = fileName;
		mChanged = false;
		mSaved = true;

		// посылаем сигнал о сохранении локации
		emit locationChanged(mChanged);
		return true;
	}

	return false;
}

bool EditorWindow::loadTranslationFile(const QString &fileName)
{
	return mLocation->loadTranslationFile(fileName);
}

Location *EditorWindow::getLocation() const
{
	return mLocation;
}

QString EditorWindow::getFileName() const
{
	return mFileName;
}

bool EditorWindow::isChanged() const
{
	return mChanged;
}

void EditorWindow::setChanged(bool changed)
{
	mChanged = changed;
	emit locationChanged(mChanged);
}

bool EditorWindow::isSaved() const
{
	return mSaved;
}

qreal EditorWindow::getZoom() const
{
	return mZoom;
}

void EditorWindow::setZoom(qreal zoom)
{
	// игнорируем разницу в полпроцента, вызванную округлением
	if (Utils::isEqual(zoom, mZoom, 0.005))
		return;

	// смещение текущего положения камеры, чтобы новый центр экрана совпадал с текущим центром экрана
	QPointF size(width(), height());
	mCameraPos = mCameraPos + size / 2.0 / mZoom - size / 2.0 / zoom;
	mZoom = zoom;

	// обновляем курсор мыши
	QPointF pos = windowToWorld(mapFromGlobal(QCursor::pos()));
	updateMouseCursor(pos);

	// отправляем сигнал об изменении координат мыши
	emit mouseMoved(pos);
}

QList<GameObject *> EditorWindow::getSelectedObjects() const
{
	return mSelectedObjects;
}

QPointF EditorWindow::getRotationCenter() const
{
	return mSnappedCenter;
}

void EditorWindow::cut()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		copy();
		clear();
	}
}

void EditorWindow::copy()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		// сохраняем объекты в байтовый массив
		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		foreach (GameObject *object, mSelectedObjects)
			if (!object->save(stream))
				return;

		// копируем данные в буфер обмена
		QMimeData *mimeData = new QMimeData();
		mimeData->setData("application/x-gameobject", data);
		QApplication::clipboard()->setMimeData(mimeData);
	}
}

void EditorWindow::paste()
{
	Layer *layer = mLocation->getAvailableLayer();
	if (mEditorState == STATE_IDLE && layer != NULL)
	{
		// получаем байтовый массив
		QByteArray data = QApplication::clipboard()->mimeData()->data("application/x-gameobject");
		if (data.isEmpty())
			return;

		// создаем и загружаем объекты из буфера обмена
		QDataStream stream(data);
		QList<GameObject *> objects;
		while (!stream.atEnd())
		{
			GameObject *object = mLocation->loadGameObject(stream);
			if (object != NULL)
			{
				objects.push_back(object);
			}
			else
			{
				qDeleteAll(objects);
				return;
			}
		}

		// генерируем новые имена и ID для созданных объектов и добавляем их в активный слой
		for (int i = objects.size() - 1; i >= 0; --i)
		{
			objects[i]->setName(mLocation->generateDuplicateName(objects[i]->getName()));
			objects[i]->setObjectID(mLocation->generateDuplicateObjectID());
			layer->insertGameObject(0, objects[i]);
		}

		// выделяем вставленные объекты
		selectGameObjects(objects);

		// обновляем курсор мыши и посылаем сигналы об изменении локации и слоев
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));
		emitLayerChangedSignals(getParentLayers());
	}
}

void EditorWindow::clear()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		QSet<BaseLayer *> layers = getParentLayers();
		qDeleteAll(mSelectedObjects);
		deselectAll();
		emitLayerChangedSignals(layers);
	}
}

void EditorWindow::selectAll()
{
	if (mEditorState == STATE_IDLE)
	{
		selectGameObjects(mLocation->getRootLayer()->findActiveGameObjects());
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));
	}
}

void EditorWindow::deselectAll()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		selectGameObjects(QList<GameObject *>());
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));
	}
}

void EditorWindow::bringToFront()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		// перемещаем выделенные объекты в начало списка у родительского слоя
		for (int i = mSelectedObjects.size() - 1; i >= 0; --i)
		{
			Layer *parent = mSelectedObjects[i]->getParentLayer();
			parent->removeGameObject(parent->indexOfGameObject(mSelectedObjects[i]));
			parent->insertGameObject(0, mSelectedObjects[i]);
		}

		// обновляем курсор мыши и посылаем сигналы об изменении локации и слоев
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));
		emitLayerChangedSignals(getParentLayers());
	}
}

void EditorWindow::sendToBack()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		// перемещаем выделенные объекты в конец списка у родительского слоя
		for (int i = 0; i < mSelectedObjects.size(); ++i)
		{
			Layer *parent = mSelectedObjects[i]->getParentLayer();
			parent->removeGameObject(parent->indexOfGameObject(mSelectedObjects[i]));
			parent->addGameObject(mSelectedObjects[i]);
		}

		// обновляем курсор мыши и посылаем сигналы об изменении локации и слоев
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));
		emitLayerChangedSignals(getParentLayers());
	}
}

void EditorWindow::moveUp()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		// перемещаем выделенные объекты на одну позицию вверх
		for (int i = 0; i < mSelectedObjects.size(); ++i)
		{
			Layer *parent = mSelectedObjects[i]->getParentLayer();
			int index = parent->indexOfGameObject(mSelectedObjects[i]);
			if (index > 0)
			{
				parent->removeGameObject(index);
				parent->insertGameObject(index - 1, mSelectedObjects[i]);
			}
		}

		// обновляем курсор мыши и посылаем сигналы об изменении локации и слоев
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));
		emitLayerChangedSignals(getParentLayers());
	}
}

void EditorWindow::moveDown()
{
	if (mEditorState == STATE_IDLE && !mSelectedObjects.empty())
	{
		// перемещаем выделенные объекты на одну позицию вниз
		for (int i = mSelectedObjects.size() - 1; i >= 0; --i)
		{
			Layer *parent = mSelectedObjects[i]->getParentLayer();
			int index = parent->indexOfGameObject(mSelectedObjects[i]);
			if (index < parent->getNumGameObjects() - 1)
			{
				parent->removeGameObject(index);
				parent->insertGameObject(index + 1, mSelectedObjects[i]);
			}
		}

		// обновляем курсор мыши и посылаем сигналы об изменении локации и слоев
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));
		emitLayerChangedSignals(getParentLayers());
	}
}

void EditorWindow::setCurrentLanguage(const QString &language)
{
	// устанавливаем новый язык
	mLocation->getRootLayer()->setCurrentLanguage(language);

	// пересчитываем прямоугольник выделения и центр вращения
	if (!mSelectedObjects.empty())
	{
		QPointF scale((mOriginalCenter.x() - mOriginalRect.left()) / mOriginalRect.width(),
			(mOriginalCenter.y() - mOriginalRect.top()) / mOriginalRect.height());
		selectGameObjects(mSelectedObjects);
		QPointF rotationCenter(mOriginalRect.width() * scale.x() + mOriginalRect.left(),
			mOriginalRect.height() * scale.y() + mOriginalRect.top());
		mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : rotationCenter;
	}
}

QStringList EditorWindow::getMissedFiles() const
{
	return mLocation->getRootLayer()->getMissedFiles();
}

void EditorWindow::changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем текстуры во всех объектах
	QList<GameObject *> objects = mLocation->getRootLayer()->changeTexture(fileName, texture);

	// формируем список измененных слоев
	QSet<BaseLayer *> layers;
	foreach (GameObject *object, objects)
		layers |= object->getParentLayer();

	// выдаем сигналы об изменении слоев
	foreach (BaseLayer *layer, layers)
		emit layerChanged(mLocation, layer);
}

void EditorWindow::updateSelection(const QPointF &rotationCenter)
{
	// обновляем прямоугольник выделения и текущий центр вращения
	selectGameObjects(mSelectedObjects);
	mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : rotationCenter;
}

void EditorWindow::paintEvent(QPaintEvent *event)
{
	// очищаем окно
	qglClearColor(QColor(33, 40, 48));
	glClear(GL_COLOR_BUFFER_BIT);

	// устанавливаем стандартную систему координат (0, 0, width, height) с началом координат в левом верхнем углу
	glViewport(0, 0, width(), height());
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width(), height(), 0.0);

	// устанавливаем камеру
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScaled(mZoom, mZoom, 1.0);
	glTranslated(-mCameraPos.x(), -mCameraPos.y(), 0.0);

	// определение видимой области
	QRectF visibleRect(mCameraPos.x(), mCameraPos.y(), width() / mZoom, height() / mZoom);

	// рисуем сетку
	Options &options = Options::getSingleton();
	if (options.isShowGrid())
	{
		// подбираем подходящий шаг сетки
		int gridSpacing = options.getGridSpacing();
		while (gridSpacing * mZoom < MIN_GRID_SPACING)
			gridSpacing *= GRID_SPACING_COEFF;

		// определение номеров линий для отрисовки
		int left = static_cast<int>(visibleRect.left() / gridSpacing);
		int top = static_cast<int>(visibleRect.top() / gridSpacing);
		int right = static_cast<int>(visibleRect.right() / gridSpacing);
		int bottom = static_cast<int>(visibleRect.bottom() / gridSpacing);

		// устанавливаем сдвиг в 0.5 пикселя
		glPushMatrix();
		glTranslated(0.5 / mZoom, 0.5 / mZoom, 0.0);

		// рисуем сетку из линий или точек
		int interval = options.getMajorLinesInterval();
		if (!options.isShowDots())
		{
			glBegin(GL_LINES);

			// рисуем вспомогательные линии
			qglColor(QColor(39, 45, 56));
			for (int i = left; i <= right; ++i)
				if (i % interval != 0)
				{
					glVertex2d(i * gridSpacing, visibleRect.top());
					glVertex2d(i * gridSpacing, visibleRect.bottom());
				}

			for (int i = top; i <= bottom; ++i)
				if (i % interval != 0)
				{
					glVertex2d(visibleRect.left(), i * gridSpacing);
					glVertex2d(visibleRect.right(), i * gridSpacing);
				}

			// рисуем основные линии
			qglColor(QColor(51, 57, 73));
			for (int i = left; i <= right; ++i)
				if (i % interval == 0)
				{
					glVertex2d(i * gridSpacing, visibleRect.top());
					glVertex2d(i * gridSpacing, visibleRect.bottom());
				}

			for (int i = top; i <= bottom; ++i)
				if (i % interval == 0)
				{
					glVertex2d(visibleRect.left(), i * gridSpacing);
					glVertex2d(visibleRect.right(), i * gridSpacing);
				}

			glEnd();
		}
		else
		{
			glBegin(GL_POINTS);

			// рисуем вспомогательные точки
			qglColor(QColor(51, 57, 73));
			for (int i = left; i <= right; ++i)
				for (int j = top; j <= bottom; ++j)
					if (i % interval != 0 && j % interval != 0)
						glVertex2d(i * gridSpacing, j * gridSpacing);

			// рисуем основные точки
			qglColor(QColor(77, 86, 110));
			for (int i = left; i <= right; ++i)
				for (int j = top; j <= bottom; ++j)
					if (i % interval == 0 || j % interval == 0)
						glVertex2d(i * gridSpacing, j * gridSpacing);

			glEnd();
		}

		// рисуем оси координат
		glBegin(GL_LINES);
		qglColor(QColor(109, 36, 38));
		glVertex2d(visibleRect.left(), 0.0);
		glVertex2d(visibleRect.right(), 0.0);
		qglColor(QColor(35, 110, 38));
		glVertex2d(0.0, visibleRect.top());
		glVertex2d(0.0, visibleRect.bottom());
		glEnd();

		// восстанавливаем матрицу трансформации
		glPopMatrix();
	}

	// рисуем локацию
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	mLocation->getRootLayer()->draw();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	// создаем painter для дополнительного рисования
	QPainter painter(this);

	// рисуем прямоугольник выделения
	if (!mSelectedObjects.empty())
	{
		// рисуем прямоугольники выделения объектов
		painter.setPen(QColor(0, 127, 255));
		foreach (GameObject *object, mSelectedObjects)
		{
			QRectF rect = worldRectToWindow(object->getBoundingRect());
			painter.drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));
		}

		// рисуем общий прямоугольник выделения
		QRectF rect = worldRectToWindow(mSnappedRect);
		painter.drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));

		// рисуем маркеры выделения
		QPointF center(qFloor(rect.center().x()) + 0.5, qFloor(rect.center().y()) + 0.5);
		painter.setPen(QColor(100, 100, 100));
		painter.setBrush(QBrush(QColor(0, 127, 255)));
		drawSelectionMarker(rect.left() + 0.5, rect.top() + 0.5, painter);
		drawSelectionMarker(rect.left() + 0.5, center.y(), painter);
		drawSelectionMarker(rect.left() + 0.5, rect.bottom() - 0.5, painter);
		drawSelectionMarker(center.x(), rect.bottom() - 0.5, painter);
		drawSelectionMarker(rect.right() - 0.5, rect.bottom() - 0.5, painter);
		drawSelectionMarker(rect.right() - 0.5, center.y(), painter);
		drawSelectionMarker(rect.right() - 0.5, rect.top() + 0.5, painter);
		drawSelectionMarker(center.x(), rect.top() + 0.5, painter);

		// рисуем центр вращения
		if (mRotationMode)
		{
			QPointF center = worldToWindow(mSnappedCenter);
			qreal x = qFloor(center.x()) + 0.5, y = qFloor(center.y()) + 0.5;
			qreal offset = qFloor(CENTER_SIZE / 2.0);
			painter.setPen(QColor(0, 127, 255));
			painter.drawLine(QPointF(x - offset, y), QPointF(x + offset, y));
			painter.drawLine(QPointF(x, y - offset), QPointF(x, y + offset));
		}
	}

	// рисуем рамку выделения
	if (mEditorState == STATE_SELECT && !mSelectionRect.isNull())
	{
		QRectF rect = worldRectToWindow(mSelectionRect.normalized());
		painter.setPen(QColor(0, 127, 255));
		painter.setBrush(QBrush(QColor(0, 100, 255, 60)));
		painter.drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));
	}

	// рисуем направляющие
	if (options.isShowGuides())
	{
		// рисуем горизонтальные направляющие
		painter.setPen(QColor(0, 192, 192));
		int numHorzGuides = mLocation->getNumGuides(true);
		for (int i = 0; i < numHorzGuides; ++i)
		{
			qreal y = qFloor((mLocation->getGuide(true, i) - mCameraPos.y()) * mZoom) + 0.5;
			painter.drawLine(QPointF(0.5, y), QPointF(width() - 0.5, y));
		}

		// рисуем вертикальные направляющие
		int numVertGuides = mLocation->getNumGuides(false);
		for (int i = 0; i < numVertGuides; ++i)
		{
			qreal x = qFloor((mLocation->getGuide(false, i) - mCameraPos.x()) * mZoom) + 0.5;
			painter.drawLine(QPointF(x, 0.5), QPointF(x, height() - 0.5));
		}
	}

	// рисуем линии привязки
	if (mEditorState == STATE_MOVE || mEditorState == STATE_RESIZE || mEditorState == STATE_MOVE_CENTER
		|| mEditorState == STATE_HORZ_GUIDE || mEditorState == STATE_VERT_GUIDE)
	{
		painter.setPen(Qt::green);
		if (!mHorzSnapLine.isNull())
			drawSmartGuide(mHorzSnapLine, painter);
		if (!mVertSnapLine.isNull())
			drawSmartGuide(mVertSnapLine, painter);
	}

	// рисуем линейки
	if (options.isShowGuides())
	{
		// рисуем фон для линеек
		painter.fillRect(0, 0, width(), RULER_SIZE, Qt::white);
		painter.fillRect(0, 0, RULER_SIZE, height(), Qt::white);

		// подбираем цену деления
		qreal rulerSpacing = 8.0;
		while (rulerSpacing * mZoom < 8.0)
			rulerSpacing *= 4.0;
		while (rulerSpacing * mZoom > 32.0)
			rulerSpacing /= 4.0;

		// определяем номера делений для отрисовки
		int left = static_cast<int>(qCeil((visibleRect.left() + RULER_SIZE / mZoom) / rulerSpacing));
		int top = static_cast<int>(qCeil((visibleRect.top() + RULER_SIZE / mZoom) / rulerSpacing));
		int right = static_cast<int>(visibleRect.right() / rulerSpacing);
		int bottom = static_cast<int>(visibleRect.bottom() / rulerSpacing);

		// рисуем горизонтальную линейку
		painter.setPen(Qt::black);
		for (int i = left; i <= right; ++i)
		{
			qreal x = qFloor((i * rulerSpacing - mCameraPos.x()) * mZoom) + 0.5;
			if (i % 5 != 0)
				painter.drawLine(QPointF(x, RULER_SIZE - DIVISION_SIZE + 0.5), QPointF(x, RULER_SIZE - 0.5));
			else
				painter.drawLine(QPointF(x, 0.5), QPointF(x, RULER_SIZE - 0.5));
		}

		// рисуем вертикальную линейку
		for (int i = top; i <= bottom; ++i)
		{
			qreal y = qFloor((i * rulerSpacing - mCameraPos.y()) * mZoom) + 0.5;
			if (i % 5 != 0)
				painter.drawLine(QPointF(RULER_SIZE - DIVISION_SIZE + 0.5, y), QPointF(RULER_SIZE - 0.5, y));
			else
				painter.drawLine(QPointF(0.5, y), QPointF(RULER_SIZE - 0.5, y));
		}
	}
}

void EditorWindow::mousePressEvent(QMouseEvent *event)
{
	// обрабатываем нажатие кнопок
	mLastPos = event->pos();
	QPointF pos = windowToWorld(event->pos());
	if (event->button() == Qt::LeftButton)
	{
		// сохраняем начальные оконные координаты мыши и сбрасываем флаг разрешения редактирования
		mFirstPos = event->pos();
		mEnableEdit = false;
		mHorzSnapLine = mVertSnapLine = QLineF();

		bool showGuides = Options::getSingleton().isShowGuides();
		qreal distance = GUIDE_DISTANCE / mZoom;
		qreal size = CENTER_SIZE / mZoom;
		qreal offset = size / 2.0;
		GameObject *object;

		if (showGuides && event->pos().y() < RULER_SIZE)
		{
			// создаем горизонтальную направляющую и переходим в режим перетаскивания
			mEditorState = STATE_HORZ_GUIDE;
			mGuideIndex = mLocation->addGuide(true, qRound(pos.y()));
		}
		else if (showGuides && event->pos().x() < RULER_SIZE)
		{
			// создаем вертикальную направляющую и переходим в режим перетаскивания
			mEditorState = STATE_VERT_GUIDE;
			mGuideIndex = mLocation->addGuide(false, qRound(pos.x()));
		}
		else if (showGuides && (mGuideIndex = mLocation->findGuide(true, pos.y(), distance)) != -1)
		{
			// переходим в режим перетаскивания горизонтальной направляющей
			mEditorState = STATE_HORZ_GUIDE;
		}
		else if (showGuides && (mGuideIndex = mLocation->findGuide(false, pos.x(), distance)) != -1)
		{
			// переходим в режим перетаскивания вертикальной направляющей
			mEditorState = STATE_VERT_GUIDE;
		}
		else if (mRotationMode && QRectF(mSnappedCenter.x() - offset, mSnappedCenter.y() - offset, size, size).contains(pos))
		{
			// переходим в состояние перетаскивания центра вращения
			mEditorState = STATE_MOVE_CENTER;
		}
		else if ((mSelectionMarker = findSelectionMarker(pos)) != MARKER_NONE)
		{
			// переходим в состояние поворота или изменения размера объектов
			if (mRotationMode)
			{
				mEditorState = STATE_ROTATE;
				mRotationVector = QVector2D(pos - mOriginalCenter).normalized();
			}
			else
			{
				mEditorState = STATE_RESIZE;
			}
		}
		else if ((object = mLocation->getRootLayer()->findGameObjectByPoint(pos)) != NULL)
		{
			if ((event->modifiers() & Qt::ControlModifier) != 0)
			{
				// устанавливаем/снимаем выделение по щелчку с Ctrl
				mEditorState = STATE_IDLE;
				QList<GameObject *> objects = mSelectedObjects;
				if (objects.contains(object))
				{
					objects.removeOne(object);
				}
				else
				{
					objects.push_back(object);
					objects = mLocation->getRootLayer()->sortGameObjects(objects);
				}
				selectGameObjects(objects);
				updateMouseCursor(pos);
			}
			else
			{
				// выделяем найденный объект и переходим в состояние перетаскивания объекта
				mEditorState = STATE_MOVE;
				mFirstTimeSelected = !mSelectedObjects.contains(object);
				if (mFirstTimeSelected)
				{
					mRotationMode = false;
					selectGameObject(object);
				}
				setCursor(Qt::ClosedHandCursor);
			}
		}
		else
		{
			// ничего не нашли - сбрасываем текущее выделение и запускаем рамку выделения
			deselectAll();
			mEditorState = STATE_SELECT;
			mSelectionRect = QRectF(pos, pos);
		}
	}
}

void EditorWindow::mouseReleaseEvent(QMouseEvent *event)
{
	// обрабатываем отпускание кнопок
	QPointF pos = windowToWorld(event->pos());
	if (event->button() == Qt::LeftButton)
	{
		// обрабатываем текущее состояние редактирования
		if (mEnableEdit)
		{
			if (mEditorState == STATE_SELECT)
			{
				// выделяем игровые объекты, попавшие в рамку
				selectGameObjects(mLocation->getRootLayer()->findGameObjectsByRect(mSelectionRect.normalized()));
			}
			else if (mEditorState == STATE_MOVE || mEditorState == STATE_RESIZE || mEditorState == STATE_ROTATE)
			{
				// обновляем текущее выделение
				selectGameObjects(mSelectedObjects);

				// обновляем координаты центра вращения
				mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mSnappedCenter;

				// посылаем сигналы об изменении локации и слоев
				emitLayerChangedSignals(getParentLayers());
			}
			else if (mEditorState == STATE_MOVE_CENTER)
			{
				// обновляем координаты центра вращения
				mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mSnappedCenter;

				// посылаем сигнал об изменении локации
				mChanged = true;
				emit locationChanged(mChanged);
			}
			else if (mEditorState == STATE_HORZ_GUIDE)
			{
				// удаляем горизонтальную направляющую, если ее вытащили за пределы окна
				bool outside = event->pos().y() < RULER_SIZE || event->pos().y() > height();
				if (outside)
					mLocation->removeGuide(true, mGuideIndex);

				// посылаем сигнал об изменении локации, кроме случая, когда направляющую перетащили с линейки за пределы окна
				if ((mFirstPos.x() >= RULER_SIZE && mFirstPos.y() >= RULER_SIZE) || !outside)
				{
					mChanged = true;
					emit locationChanged(mChanged);
				}
			}
			else if (mEditorState == STATE_VERT_GUIDE)
			{
				// удаляем вертикальную направляющую, если ее вытащили за пределы окна
				bool outside = event->pos().x() < RULER_SIZE || event->pos().x() > width();
				if (outside)
					mLocation->removeGuide(false, mGuideIndex);

				// посылаем сигнал об изменении локации, кроме случая, когда направляющую перетащили с линейки за пределы окна
				if ((mFirstPos.x() >= RULER_SIZE && mFirstPos.y() >= RULER_SIZE) || !outside)
				{
					mChanged = true;
					emit locationChanged(mChanged);
				}
			}
		}
		else if (mEditorState == STATE_MOVE && !mFirstTimeSelected)
		{
			// переключаем режим поворота при щелчках по объекту
			mRotationMode = !mRotationMode;
		}
		else if (mEditorState == STATE_HORZ_GUIDE || mEditorState == STATE_VERT_GUIDE)
		{
			// удаляем созданную направляющую при щелчке по линейке без перетаскивания
			if (mFirstPos.x() < RULER_SIZE || mFirstPos.y() < RULER_SIZE)
				mLocation->removeGuide(mEditorState == STATE_HORZ_GUIDE, mGuideIndex);
		}

		// возвращаемся в состояние простоя
		mEditorState = STATE_IDLE;
		updateMouseCursor(pos);
	}
}

void EditorWindow::mouseMoveEvent(QMouseEvent *event)
{
	// определяем смещение мыши с прошлого раза
	QPointF delta = QPointF(event->pos() - mLastPos) / mZoom;
	mLastPos = event->pos();

	// разрешаем редактирование, если мышь передвинулась достаточно далеко от начального положения при зажатой левой кнопке
	if ((event->buttons() & Qt::LeftButton) != 0 && (qAbs(mLastPos.x() - mFirstPos.x()) > 2 || qAbs(mLastPos.y() - mFirstPos.y()) > 2))
		mEnableEdit = true;

	// перемещаем камеру при зажатой правой кнопке мыши
	if ((event->buttons() & Qt::RightButton) != 0)
		mCameraPos -= delta;

	// обрабатываем текущее состояние редактирования
	QPointF pos = windowToWorld(event->pos());
	if (mEnableEdit)
	{
		if (mEditorState == STATE_SELECT)
		{
			mSelectionRect.setBottomRight(pos);
		}
		else if (mEditorState == STATE_MOVE)
		{
			// определяем вектор перемещения
			bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
			QVector2D axis;
			QPointF offset = calcTranslation(QPointF(mLastPos - mFirstPos) / mZoom, shift, axis);

			// определяем координаты привязываемого прямоугольника
			QRectF rect = mOriginalRect.translated(offset);
			mHorzSnapLine = mVertSnapLine = QLineF();

			// привязываем прямоугольник по оси X
			if (!shift || Utils::isEqual(axis.y(), 0.0))
			{
				// вычисляем расстояния привязок для левого края, центра и правого края прямоугольника выделения
				QLineF leftLine, centerLine, rightLine;
				qreal leftDistance, centerDistance, rightDistance;
				qreal left = snapXCoord(rect.left(), rect.top(), rect.bottom(), true, &leftLine, &leftDistance);
				qreal center = snapXCoord(rect.center().x(), rect.center().y(), rect.center().y(), true, &centerLine, &centerDistance);
				qreal right = snapXCoord(rect.right(), rect.top(), rect.bottom(), true, &rightLine, &rightDistance);

				// привязываем край с наименьшим расстоянием привязки
				if (centerDistance < SNAP_DISTANCE / mZoom && centerDistance <= leftDistance && centerDistance <= rightDistance)
				{
					offset.rx() += center - rect.center().x();
					mVertSnapLine = centerLine;
				}
				else if (leftDistance < SNAP_DISTANCE / mZoom && leftDistance <= centerDistance && leftDistance <= rightDistance)
				{
					offset.rx() += left - rect.left();
					mVertSnapLine = leftLine;
				}
				else if (rightDistance < SNAP_DISTANCE / mZoom && rightDistance <= leftDistance && rightDistance <= centerDistance)
				{
					offset.rx() += right - rect.right();
					mVertSnapLine = rightLine;
				}
				else
				{
					offset.rx() += left - rect.left();
				}
			}

			// привязываем прямоугольник по оси Y
			if (!shift || Utils::isEqual(axis.x(), 0.0))
			{
				// вычисляем расстояния привязок для верхнего края, центра и нижнего края прямоугольника выделения
				QLineF topLine, centerLine, bottomLine;
				qreal topDistance, centerDistance, bottomDistance;
				qreal top = snapYCoord(rect.top(), rect.left(), rect.right(), true, &topLine, &topDistance);
				qreal center = snapYCoord(rect.center().y(), rect.center().x(), rect.center().x(), true, &centerLine, &centerDistance);
				qreal bottom = snapYCoord(rect.bottom(), rect.left(), rect.right(), true, &bottomLine, &bottomDistance);

				// привязываем край с наименьшим расстоянием привязки
				if (centerDistance < SNAP_DISTANCE / mZoom && centerDistance <= topDistance && centerDistance <= bottomDistance)
				{
					offset.ry() += center - rect.center().y();
					mHorzSnapLine = centerLine;
				}
				else if (topDistance < SNAP_DISTANCE / mZoom && topDistance <= centerDistance && topDistance <= bottomDistance)
				{
					offset.ry() += top - rect.top();
					mHorzSnapLine = topLine;
				}
				else if (bottomDistance < SNAP_DISTANCE / mZoom && bottomDistance <= topDistance && bottomDistance <= centerDistance)
				{
					offset.ry() += bottom - rect.bottom();
					mHorzSnapLine = bottomLine;
				}
				else
				{
					offset.ry() += top - rect.top();
				}
			}

			// перемещаем все выделенные объекты
			for (int i = 0; i < mSelectedObjects.size(); ++i)
			{
				QPointF position = mOriginalPositions[i] + offset;
				mSelectedObjects[i]->setPosition(QPointF(qRound(position.x()), qRound(position.y())));
			}

			// пересчитываем общий прямоугольник выделения
			mSnappedRect = mSelectedObjects.front()->getBoundingRect();
			foreach (GameObject *object, mSelectedObjects)
				mSnappedRect |= object->getBoundingRect();

			// перемещаем центр вращения
			mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mOriginalCenter + offset;
		}
		else if (mEditorState == STATE_RESIZE)
		{
			// определяем значение масштаба и координаты точки, относительно которой он производится
			bool keepProportions = (event->modifiers() & Qt::ShiftModifier) != 0 || mHasRotatedObjects;
			QPointF pivot, scale;
			mHorzSnapLine = mVertSnapLine = QLineF();
			switch (mSelectionMarker)
			{
			case MARKER_TOP_LEFT:
				pivot = mOriginalRect.bottomRight();
				scale = calcScale(pos, pivot, -1.0, -1.0, keepProportions);
				break;

			case MARKER_CENTER_LEFT:
				pivot = QPointF(mOriginalRect.right(), mOriginalRect.center().y());
				scale.rx() = (pivot.x() - snapXCoord(pos.x(), pivot.y(), pivot.y(), true, &mVertSnapLine)) / mOriginalRect.width();
				scale.ry() = keepProportions ? qAbs(scale.x()) : 1.0;
				break;

			case MARKER_BOTTOM_LEFT:
				pivot = mOriginalRect.topRight();
				scale = calcScale(pos, pivot, -1.0, 1.0, keepProportions);
				break;

			case MARKER_BOTTOM_CENTER:
				pivot = QPointF(mOriginalRect.center().x(), mOriginalRect.top());
				scale.ry() = (snapYCoord(pos.y(), pivot.x(), pivot.x(), true, &mHorzSnapLine) - pivot.y()) / mOriginalRect.height();
				scale.rx() = keepProportions ? qAbs(scale.y()) : 1.0;
				break;

			case MARKER_BOTTOM_RIGHT:
				pivot = mOriginalRect.topLeft();
				scale = calcScale(pos, pivot, 1.0, 1.0, keepProportions);
				break;

			case MARKER_CENTER_RIGHT:
				pivot = QPointF(mOriginalRect.left(), mOriginalRect.center().y());
				scale.rx() = (snapXCoord(pos.x(), pivot.y(), pivot.y(), true, &mVertSnapLine) - pivot.x()) / mOriginalRect.width();
				scale.ry() = keepProportions ? qAbs(scale.x()) : 1.0;
				break;

			case MARKER_TOP_RIGHT:
				pivot = mOriginalRect.bottomLeft();
				scale = calcScale(pos, pivot, 1.0, -1.0, keepProportions);
				break;

			case MARKER_TOP_CENTER:
				pivot = QPointF(mOriginalRect.center().x(), mOriginalRect.bottom());
				scale.ry() = (pivot.y() - snapYCoord(pos.y(), pivot.x(), pivot.x(), true, &mHorzSnapLine)) / mOriginalRect.height();
				scale.rx() = keepProportions ? qAbs(scale.y()) : 1.0;
				break;

			default:
				break;
			}

			// пересчитываем положение и размеры выделенных объектов
			for (int i = 0; i < mSelectedObjects.size(); ++i)
			{
				// вычисляем новую позицию
				QPointF position((mOriginalPositions[i].x() - pivot.x()) * scale.x() + pivot.x(),
					(mOriginalPositions[i].y() - pivot.y()) * scale.y() + pivot.y());

				// вычисляем новый размер
				QSizeF size;
				if (keepProportions || mOriginalAngles[i] == 0.0 || mOriginalAngles[i] == 180.0)
					size = QSizeF(mOriginalSizes[i].width() * scale.x(), mOriginalSizes[i].height() * scale.y());
				else
					size = QSizeF(mOriginalSizes[i].width() * scale.y(), mOriginalSizes[i].height() * scale.x());

				// устанавливаем новую позицию и размер, если он не менее одного пикселя
				if (qAbs(size.width()) >= 1.0 && qAbs(size.height()) >= 1.0)
				{
					mSelectedObjects[i]->setPosition(QPointF(qRound(position.x()), qRound(position.y())));
					mSelectedObjects[i]->setSize(QSizeF(qRound(size.width()), qRound(size.height())));
				}
			}

			// пересчитываем общий прямоугольник выделения
			mSnappedRect = mSelectedObjects.front()->getBoundingRect();
			foreach (GameObject *object, mSelectedObjects)
				mSnappedRect |= object->getBoundingRect();

			// пересчитываем координаты центра вращения
			mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter()
				: QPointF((mOriginalCenter.x() - pivot.x()) * scale.x() + pivot.x(), (mOriginalCenter.y() - pivot.y()) * scale.y() + pivot.y());
		}
		else if (mEditorState == STATE_ROTATE)
		{
			// определяем текущий угол поворота в радианах относительно начального вектора
			QVector2D currVector = QVector2D(pos - mOriginalCenter).normalized();
			qreal angle = qAcos(QVector2D::dotProduct(currVector, mRotationVector));
			if (QString::number(angle) == "nan" || Utils::isEqual(angle, 0.0))
				angle = 0.0;

			// приводим угол к диапазону [-PI; PI]
			QVector2D normalVector(-mRotationVector.y(), mRotationVector.x());
			if (QVector2D::dotProduct(currVector, normalVector) < 0.0)
				angle = -angle;

			// если зажат шифт, округляем угол до величины, кратной 45 градусам
			if ((event->modifiers() & Qt::ShiftModifier) != 0)
				angle = qFloor((angle + Utils::PI / 8.0) / (Utils::PI / 4.0)) * (Utils::PI / 4.0);

			// поворачиваем все выделенные объекты вокруг центра вращения
			for (int i = 0; i < mSelectedObjects.size(); ++i)
			{
				// определяем абсолютный угол поворота в градусах и приводим его к диапазону [0; 360)
				qreal absAngle = fmod(mOriginalAngles[i] + Utils::radToDeg(angle), 360.0);
				if (absAngle < 0.0)
					absAngle += 360.0;

				// привязываем абсолютный угол поворота к 0/90/180/270 градусам
				const qreal ANGLE_EPS = 0.5;
				if (Utils::isEqual(absAngle, 0.0, ANGLE_EPS) || Utils::isEqual(absAngle, 360.0, ANGLE_EPS))
					absAngle = 0.0;
				else if (Utils::isEqual(absAngle, 90.0, ANGLE_EPS))
					absAngle = 90.0;
				else if (Utils::isEqual(absAngle, 180.0, ANGLE_EPS))
					absAngle = 180.0;
				else if (Utils::isEqual(absAngle, 270.0, ANGLE_EPS))
					absAngle = 270.0;

				// поворачиваем объект на привязанный угол
				qreal newAngle = Utils::degToRad(absAngle - mOriginalAngles[i]);
				QPointF pt = mOriginalPositions[i] - mOriginalCenter;
				QPointF position(pt.x() * qCos(newAngle) - pt.y() * qSin(newAngle) + mOriginalCenter.x(),
					pt.x() * qSin(newAngle) + pt.y() * qCos(newAngle) + mOriginalCenter.y());
				if (absAngle == 0.0 || absAngle == 90.0 || absAngle == 180.0 || absAngle == 270.0)
					position = QPointF(qRound(position.x()), qRound(position.y()));

				// устанавливаем новую позицию и угол поворота
				mSelectedObjects[i]->setPosition(position);
				mSelectedObjects[i]->setRotationAngle(absAngle);
			}

			// пересчитываем общий прямоугольник выделения
			mSnappedRect = mSelectedObjects.front()->getBoundingRect();
			foreach (GameObject *object, mSelectedObjects)
				mSnappedRect |= object->getBoundingRect();
		}
		else if (mEditorState == STATE_MOVE_CENTER)
		{
			// определяем вектор перемещения
			bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
			QVector2D axis;
			QPointF offset = calcTranslation(pos - mOriginalCenter, shift, axis);

			// определяем координаты привязываемой точки
			QPointF pt = mOriginalCenter + offset;
			mHorzSnapLine = mVertSnapLine = QLineF();

			// привязываем точку по оси X
			if (!shift || Utils::isEqual(axis.y(), 0.0))
				offset.rx() += snapXCoord(pt.x(), pt.y(), pt.y(), false, &mVertSnapLine) - pt.x();

			// привязываем точку по оси Y
			if (!shift || Utils::isEqual(axis.x(), 0.0))
				offset.ry() += snapYCoord(pt.y(), pt.x(), pt.x(), false, &mHorzSnapLine) - pt.y();

			// перемещаем центр вращения
			mSnappedCenter = mOriginalCenter + offset;
			if (mSelectedObjects.size() == 1)
				mSelectedObjects.front()->setRotationCenter(mSnappedCenter);
		}
		else if (mEditorState == STATE_HORZ_GUIDE)
		{
			// перемещаем горизонтальную направляющую
			qreal snappedY = pos.y();
			if (Options::getSingleton().isEnableSmartGuides())
			{
				const qreal MAX_COORD = 1.0E+8;
				qreal distance = SNAP_DISTANCE / mZoom;
				mHorzSnapLine = mVertSnapLine = QLineF();
				mLocation->getRootLayer()->snapYCoord(pos.y(), MAX_COORD, -MAX_COORD, QList<GameObject *>(), snappedY, distance, mHorzSnapLine);
			}
			mLocation->setGuide(true, mGuideIndex, qRound(snappedY));
		}
		else if (mEditorState == STATE_VERT_GUIDE)
		{
			// перемещаем вертикальную направляющую
			qreal snappedX = pos.x();
			if (Options::getSingleton().isEnableSmartGuides())
			{
				const qreal MAX_COORD = 1.0E+8;
				qreal distance = SNAP_DISTANCE / mZoom;
				mHorzSnapLine = mVertSnapLine = QLineF();
				mLocation->getRootLayer()->snapXCoord(pos.x(), MAX_COORD, -MAX_COORD, QList<GameObject *>(), snappedX, distance, mVertSnapLine);
			}
			mLocation->setGuide(false, mGuideIndex, qRound(snappedX));
		}

		// отправляем сигнал об изменении объектов
		if (mEditorState == STATE_MOVE || mEditorState == STATE_RESIZE || mEditorState == STATE_ROTATE || mEditorState == STATE_MOVE_CENTER)
			emit objectsChanged(mSelectedObjects, mSnappedCenter);
	}

	// обновляем курсор мыши
	updateMouseCursor(pos);

	// отправляем сигнал об изменении координат мыши
	emit mouseMoved(pos);
}

void EditorWindow::wheelEvent(QWheelEvent *event)
{
	const qreal SCROLL_SPEED = 16.0;
	if ((event->modifiers() & Qt::AltModifier) != 0)
	{
		// рассчитываем новый зум
		qreal zoom = qBound(0.1, mZoom * qPow(1.1, event->delta() / 120.0), 32.0);

		// применяем новый зум, чтобы объектное положение мышки не изменилось
		QPointF mousePos = event->pos();
		mCameraPos = mCameraPos + mousePos / mZoom - mousePos / zoom;
		mZoom = zoom;

		// отправляем сигнал об изменении масштаба
		QString zoomStr = QString::number(qRound(mZoom * 100.0)) + '%';
		emit zoomChanged(zoomStr);
	}
	else
	{
		// перемещаем камеру по горизонтали или вертикали
		Qt::Orientation orientation = event->orientation();
		if ((event->modifiers() & Qt::ControlModifier) != 0)
			orientation = orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal;
		if (orientation == Qt::Horizontal)
			mCameraPos.rx() -= (event->delta() / 120.0) * SCROLL_SPEED / mZoom;
		else
			mCameraPos.ry() -= (event->delta() / 120.0) * SCROLL_SPEED / mZoom;

		// обновляем курсор мыши и посылаем сигнал об изменении координат
		QPointF pos = windowToWorld(event->pos());
		updateMouseCursor(pos);
		emit mouseMoved(pos);
	}
}

void EditorWindow::keyPressEvent(QKeyEvent *event)
{
	// стрелки клавиатуры
	if ((event->key() == Qt::Key_Left || event->key() == Qt::Key_Right || event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) && !mSelectedObjects.empty())
	{
		// выбираем направление перемещения
		QPointF offset;
		if (event->key() == Qt::Key_Left)
			offset = QPointF(-1.0, 0.0);
		else if (event->key() == Qt::Key_Right)
			offset = QPointF(1.0, 0.0);
		else if (event->key() == Qt::Key_Up)
			offset = QPointF(0.0, -1.0);
		else
			offset = QPointF(0.0, 1.0);

		// перемещаем все выделенные объекты
		for (int i = 0; i < mSelectedObjects.size(); ++i)
			mSelectedObjects[i]->setPosition(mOriginalPositions[i] + offset);

		// перемещаем центр вращения
		mOriginalCenter = mSnappedCenter = mOriginalCenter + offset;

		// обновляем выделение и курсор мыши
		selectGameObjects(mSelectedObjects);
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));

		// посылаем сигналы об изменении локации и слоев
		emitLayerChangedSignals(getParentLayers());

		// отправляем сигнал об изменении объектов
		emit objectsChanged(mSelectedObjects, mSnappedCenter);
	}
}

void EditorWindow::dragEnterEvent(QDragEnterEvent *event)
{
	// проверяем, что текущий активный слой доступен
	if (mLocation->getAvailableLayer() == NULL)
		return;

	// проверяем, что перетаскивается элемент из браузера спрайтов или шрифтов
	if (event->source() == mSpriteWidget || event->source() == mFontWidget)
		event->acceptProposedAction();
}

void EditorWindow::dropEvent(QDropEvent *event)
{
	// проверяем, что текущий активный слой доступен
	if (mLocation->getAvailableLayer() == NULL)
		return;

	// получаем полный путь к ресурсному файлу
	QString path;
	if (event->source() == mSpriteWidget)
	{
		// получаем MIME-закодированные данные для перетаскиваемого элемента
		QByteArray data = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
		if (data.isEmpty())
			return;

		// распаковываем полученные данные
		int row, col;
		QMap<int, QVariant> roles;
		QDataStream stream(data);
		stream >> row >> col >> roles;

		// извлекаем путь к ресурсному файлу из UserRole
		path = roles.value(Qt::UserRole).toString();
	}
	else if (event->source() == mFontWidget)
	{
		// получаем список URL для перетаскиваемого элемента
		QList<QUrl> urls = event->mimeData()->urls();
		if (urls.empty())
			return;

		// извлекаем путь к ресурсному файлу из первого URL в списке
		path = urls.front().toLocalFile();
	}
	else
	{
		return;
	}

	// определяем имя файла относительно корневого каталога
	QString rootDirectory = Project::getSingleton().getRootDirectory();
	if (!path.startsWith(rootDirectory))
		return;
	QString fileName = path.mid(rootDirectory.size());

	// проверяем имя файла на валидность
	if (!Utils::isFileNameValid(fileName))
	{
		QMessageBox::warning(this, "", "Неверный путь к файлу " + fileName + "\nПереименуйте файлы и папки так, чтобы они "
			"начинались с буквы и состояли только из маленьких латинских букв, цифр и знаков подчеркивания");
		return;
	}

	// создаем объект нужного типа в окне редактора
	GameObject *object;
	QPointF pos = windowToWorld(event->pos());
	if (event->source() == mSpriteWidget)
		object = mLocation->createSprite(pos, fileName);
	else
		object = mLocation->createLabel(pos, fileName, 32);

	// выделяем созданный объект
	selectGameObject(object);

	// обновляем курсор мыши и посылаем сигналы об изменении локации и слоев
	updateMouseCursor(pos);
	emitLayerChangedSignals(getParentLayers());

	// устанавливаем фокус на окне редактирования
	setFocus();

	// принимаем перетаскивание
	event->acceptProposedAction();
}

QPointF EditorWindow::worldToWindow(const QPointF &pt) const
{
	return (pt - mCameraPos) * mZoom;
}

QPointF EditorWindow::windowToWorld(const QPointF &pt) const
{
	return mCameraPos + pt / mZoom;
}

QRectF EditorWindow::worldRectToWindow(const QRectF &rect) const
{
	QPointF topLeft = worldToWindow(rect.topLeft());
	QPointF bottomRight = worldToWindow(rect.bottomRight());

	// округляем координаты в меньшую или большую сторону в соответствии с правилами растеризации полигонов
	topLeft.rx() = topLeft.x() <= qFloor(topLeft.x()) + 0.5 ? qFloor(topLeft.x()) : qFloor(topLeft.x()) + 1.0;
	topLeft.ry() = topLeft.y() <= qFloor(topLeft.y()) + 0.5 ? qFloor(topLeft.y()) : qFloor(topLeft.y()) + 1.0;
	bottomRight.rx() = bottomRight.x() < qFloor(bottomRight.x()) + 0.5 ? qFloor(bottomRight.x()) : qFloor(bottomRight.x()) + 1.0;
	bottomRight.ry() = bottomRight.y() < qFloor(bottomRight.y()) + 0.5 ? qFloor(bottomRight.y()) : qFloor(bottomRight.y()) + 1.0;

	return QRectF(topLeft, bottomRight);
}

qreal EditorWindow::snapXCoord(qreal x, qreal y1, qreal y2, bool excludeSelection, QLineF *linePtr, qreal *distancePtr)
{
	Options &options = Options::getSingleton();

	qreal snappedX = x;
	qreal distance = SNAP_DISTANCE / mZoom;
	QLineF line;

	// привязываем координату к направляющим
	if (options.isSnapToGuides())
	{
		int index = mLocation->findGuide(false, x, distance);
		if (index != -1)
		{
			snappedX = mLocation->getGuide(false, index);
			line = QLineF(snappedX, y1, snappedX, y2);
		}
	}

	// привязываем координату к умным направляющим
	if (options.isEnableSmartGuides())
	{
		const QList<GameObject *> &excludedObjects = excludeSelection ? mSelectedObjects : QList<GameObject *>();
		mLocation->getRootLayer()->snapXCoord(x, y1, y2, excludedObjects, snappedX, distance, line);
	}

	// привязываем координату к сетке
	if (options.isSnapToGrid() && distance == SNAP_DISTANCE / mZoom)
	{
		// подбираем подходящий шаг сетки
		int gridSpacing = options.getGridSpacing();
		if (options.isSnapToVisibleLines())
		{
			while (gridSpacing * mZoom < MIN_GRID_SPACING)
				gridSpacing *= GRID_SPACING_COEFF;
		}

		// округляем координату до выбранного шага сетки
		snappedX = qFloor((x + gridSpacing / 2.0) / gridSpacing) * gridSpacing;
	}

	// возвращаем полученные значения
	if (linePtr != NULL)
		*linePtr = line;
	if (distancePtr != NULL)
		*distancePtr = distance;

	return snappedX;
}

qreal EditorWindow::snapYCoord(qreal y, qreal x1, qreal x2, bool excludeSelection, QLineF *linePtr, qreal *distancePtr)
{
	Options &options = Options::getSingleton();

	qreal snappedY = y;
	qreal distance = SNAP_DISTANCE / mZoom;
	QLineF line;

	// привязываем координату к направляющим
	if (options.isSnapToGuides())
	{
		int index = mLocation->findGuide(true, y, distance);
		if (index != -1)
		{
			snappedY = mLocation->getGuide(true, index);
			line = QLineF(x1, snappedY, x2, snappedY);
		}
	}

	// привязываем координату к умным направляющим
	if (options.isEnableSmartGuides())
	{
		const QList<GameObject *> &excludedObjects = excludeSelection ? mSelectedObjects : QList<GameObject *>();
		mLocation->getRootLayer()->snapYCoord(y, x1, x2, excludedObjects, snappedY, distance, line);
	}

	// привязываем координату к сетке
	if (options.isSnapToGrid() && distance == SNAP_DISTANCE / mZoom)
	{
		// подбираем подходящий шаг сетки
		int gridSpacing = options.getGridSpacing();
		if (options.isSnapToVisibleLines())
		{
			while (gridSpacing * mZoom < MIN_GRID_SPACING)
				gridSpacing *= GRID_SPACING_COEFF;
		}

		// округляем координату до выбранного шага сетки
		snappedY = qFloor((y + gridSpacing / 2.0) / gridSpacing) * gridSpacing;
	}

	// возвращаем полученные значения
	if (linePtr != NULL)
		*linePtr = line;
	if (distancePtr != NULL)
		*distancePtr = distance;

	return snappedY;
}

QPointF EditorWindow::calcTranslation(const QPointF &direction, bool shift, QVector2D &axis)
{
	if (shift)
	{
		// определяем угол между исходным вектором и осью OX в радианах
		QVector2D vector(direction);
		qreal angle = qAcos(vector.normalized().x());
		if (QString::number(angle) == "nan" || Utils::isEqual(angle, 0.0))
			angle = 0.0;

		// приводим угол к диапазону [-PI; PI]
		if (vector.y() < 0.0)
			angle = -angle;

		// округляем угол до величины, кратной 45 градусам
		angle = qFloor((angle + Utils::PI / 8.0) / (Utils::PI / 4.0)) * (Utils::PI / 4.0);

		// получаем ось перемещения и проецируем на нее исходный вектор
		axis = QVector2D(qCos(angle), qSin(angle));
		return (QVector2D::dotProduct(vector, axis) * axis).toPointF();
	}
	else
	{
		// возвращаем исходный вектор перемещения
		axis = QVector2D(direction).normalized();
		return direction;
	}
}

QPointF EditorWindow::calcScale(const QPointF &pos, const QPointF &pivot, qreal sx, qreal sy, bool keepProportions)
{
	// привязываем точку к осям X и Y и вычисляем масштаб
	qreal snappedX = snapXCoord(pos.x(), pos.y(), pos.y(), true, &mVertSnapLine);
	qreal snappedY = snapYCoord(pos.y(), pos.x(), pos.x(), true, &mHorzSnapLine);
	QPointF scale((snappedX - pivot.x()) * sx / mOriginalRect.width(), (snappedY - pivot.y()) * sy / mOriginalRect.height());

	// при пропорциональном масштабировании привязываем точку только по оси с наименьшим масштабом
	if (keepProportions)
	{
		if (qAbs(scale.x()) < qAbs(scale.y()))
		{
			scale.ry() = Utils::sign(scale.y()) * qAbs(scale.x());
			qreal y = mOriginalRect.height() * scale.y() * sy + pivot.y();
			snapXCoord(pos.x(), y, y, true, &mVertSnapLine);
			mHorzSnapLine = QLineF();
		}
		else
		{
			scale.rx() = Utils::sign(scale.x()) * qAbs(scale.y());
			qreal x = mOriginalRect.width() * scale.x() * sx + pivot.x();
			snapYCoord(pos.y(), x, x, true, &mHorzSnapLine);
			mVertSnapLine = QLineF();
		}
	}

	return scale;
}

void EditorWindow::selectGameObject(GameObject *object)
{
	QList<GameObject *> objects;
	objects.push_back(object);
	selectGameObjects(objects);
}

void EditorWindow::selectGameObjects(const QList<GameObject *> &objects)
{
	// находим общий ограничивающий прямоугольник для всех выделенных объектов
	if (!objects.empty())
	{
		mOriginalPositions.clear();
		mOriginalSizes.clear();
		mOriginalAngles.clear();
		mOriginalRect = objects.front()->getBoundingRect();
		mHasRotatedObjects = false;
		foreach (GameObject *object, objects)
		{
			qreal angle = object->getRotationAngle();
			if (angle != 0.0 && angle != 90.0 && angle != 180.0 && angle != 270.0)
				mHasRotatedObjects = true;
			mOriginalPositions.push_back(object->getPosition());
			mOriginalSizes.push_back(object->getSize());
			mOriginalAngles.push_back(angle);
			mOriginalRect |= object->getBoundingRect();
		}
		mSnappedRect = mOriginalRect;
	}

	// проверяем на изменение выделения
	if (objects != mSelectedObjects)
	{
		// сохраняем список выделенных объектов
		mSelectedObjects = objects;

		// обновляем состояние режима поворота
		if (mSelectedObjects.empty())
			mRotationMode = false;
		else
			mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mOriginalRect.center();

		// посылаем сигнал об изменении выделения
		emit selectionChanged(mSelectedObjects, mSnappedCenter);
	}
}

void EditorWindow::updateMouseCursor(const QPointF &pos)
{
	if (mEditorState == STATE_IDLE)
	{
		bool showGuides = Options::getSingleton().isShowGuides();
		qreal distance = GUIDE_DISTANCE / mZoom;
		qreal size = CENTER_SIZE / mZoom;
		qreal offset = size / 2.0;
		SelectionMarker marker;

		if (showGuides && ((pos.x() < mCameraPos.x() + RULER_SIZE / mZoom) || (pos.y() < mCameraPos.y() + RULER_SIZE / mZoom)))
		{
			// курсор над линейками
			setCursor(Qt::ArrowCursor);
		}
		else if (showGuides && mLocation->findGuide(true, pos.y(), distance) != -1)
		{
			// курсор над горизонтальной направляющей
			setCursor(Qt::SplitVCursor);
		}
		else if (showGuides && mLocation->findGuide(false, pos.x(), distance) != -1)
		{
			// курсор над вертикальной направляющей
			setCursor(Qt::SplitHCursor);
		}
		else if (mRotationMode && QRectF(mSnappedCenter.x() - offset, mSnappedCenter.y() - offset, size, size).contains(pos))
		{
			// курсор над центром вращения
			setCursor(Qt::CrossCursor);
		}
		else if ((marker = findSelectionMarker(pos)) != MARKER_NONE)
		{
			// курсор над маркером выделения
			if (mRotationMode)
			{
				setCursor(mRotateCursor);
			}
			else
			{
				static const Qt::CursorShape MARKER_CURSORS[8] = {Qt::SizeFDiagCursor, Qt::SizeHorCursor, Qt::SizeBDiagCursor, Qt::SizeVerCursor,
					Qt::SizeFDiagCursor, Qt::SizeHorCursor, Qt::SizeBDiagCursor, Qt::SizeVerCursor};
				setCursor(MARKER_CURSORS[marker - 1]);
			}
		}
		else if (mLocation->getRootLayer()->findGameObjectByPoint(pos) != NULL)
		{
			// курсор над игровым объектом
			setCursor(Qt::OpenHandCursor);
		}
		else
		{
			// курсор над свободным местом
			setCursor(Qt::ArrowCursor);
		}
	}
}

QSet<BaseLayer *> EditorWindow::getParentLayers() const
{
	// формируем список родительских слоев
	QSet<BaseLayer *> layers;
	foreach (GameObject *object, mSelectedObjects)
		layers |= object->getParentLayer();
	return layers;
}

void EditorWindow::emitLayerChangedSignals(const QSet<BaseLayer *> &layers)
{
	// посылаем сигналы об изменении локации и слоев
	mChanged = true;
	emit locationChanged(mChanged);
	foreach (BaseLayer *layer, layers)
		emit layerChanged(mLocation, layer);
}

EditorWindow::SelectionMarker EditorWindow::findSelectionMarker(const QPointF &pos) const
{
	if (!mSelectedObjects.empty())
	{
		qreal size = MARKER_SIZE / mZoom;
		qreal offset = size / 2.0;
		QPointF center = mSnappedRect.center();

		if (QRectF(mSnappedRect.left() - offset, mSnappedRect.top() - offset, size, size).contains(pos))
			return MARKER_TOP_LEFT;
		if (QRectF(mSnappedRect.left() - offset, center.y() - offset, size, size).contains(pos))
			return MARKER_CENTER_LEFT;
		if (QRectF(mSnappedRect.left() - offset, mSnappedRect.bottom() - offset, size, size).contains(pos))
			return MARKER_BOTTOM_LEFT;
		if (QRectF(center.x() - offset, mSnappedRect.bottom() - offset, size, size).contains(pos))
			return MARKER_BOTTOM_CENTER;
		if (QRectF(mSnappedRect.right() - offset, mSnappedRect.bottom() - offset, size, size).contains(pos))
			return MARKER_BOTTOM_RIGHT;
		if (QRectF(mSnappedRect.right() - offset, center.y() - offset, size, size).contains(pos))
			return MARKER_CENTER_RIGHT;
		if (QRectF(mSnappedRect.right() - offset, mSnappedRect.top() - offset, size, size).contains(pos))
			return MARKER_TOP_RIGHT;
		if (QRectF(center.x() - offset, mSnappedRect.top() - offset, size, size).contains(pos))
			return MARKER_TOP_CENTER;
	}

	return MARKER_NONE;
}

void EditorWindow::drawSelectionMarker(qreal x, qreal y, QPainter &painter)
{
	// рисуем круглый или прямоугольный маркер выделения
	qreal offset = qFloor(MARKER_SIZE / 2.0);
	if (mRotationMode)
		painter.drawEllipse(QPointF(x, y), offset, offset);
	else
		painter.drawRect(QRectF(x - offset, y - offset, MARKER_SIZE - 1.0, MARKER_SIZE - 1.0));
}

void EditorWindow::drawSmartGuide(const QLineF &line, QPainter &painter)
{
	// рисуем линию
	QPointF p1 = worldToWindow(line.p1()), p2 = worldToWindow(line.p2());
	p1 = QPointF(qFloor(p1.x()) + 0.5, qFloor(p1.y()) + 0.5);
	p2 = QPointF(qFloor(p2.x()) + 0.5, qFloor(p2.y()) + 0.5);
	painter.drawLine(p1, p2);

	// рисуем крестик в начале линии
	qreal offset = qFloor(CENTER_SIZE / 2.0);
	painter.drawLine(QPointF(p1.x() - offset, p1.y() - offset), QPointF(p1.x() + offset, p1.y() + offset));
	painter.drawLine(QPointF(p1.x() - offset, p1.y() + offset), QPointF(p1.x() + offset, p1.y() - offset));

	// рисуем крестик в конце линии
	painter.drawLine(QPointF(p2.x() - offset, p2.y() - offset), QPointF(p2.x() + offset, p2.y() + offset));
	painter.drawLine(QPointF(p2.x() - offset, p2.y() + offset), QPointF(p2.x() + offset, p2.y() - offset));
}
