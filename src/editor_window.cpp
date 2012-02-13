#include "pch.h"
#include "editor_window.h"
#include "game_object.h"
#include "layer.h"
#include "location.h"
#include "utils.h"

EditorWindow::EditorWindow(QWidget *parent, QGLWidget *shareWidget, const QString &fileName)
: QGLWidget(shareWidget->format(), parent, shareWidget), mFileName(fileName), mChanged(false), mSaved(false), mEditorState(STATE_IDLE),
  mCameraPos(0.0, 0.0), mZoom(1.0), mRotationMode(false)
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
	if (qAbs(zoom - mZoom) <= 0.005)
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

bool EditorWindow::hasSelectedObjects() const
{
	return !mSelectedObjects.empty();
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

		// добавляем объекты в активный слой
		for (int i = objects.size() - 1; i >= 0; --i)
		{
			objects[i]->setName(mLocation->generateDuplicateName(objects[i]->getName()));
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

void EditorWindow::initializeGL()
{
	qglClearColor(QColor(33, 40, 48));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EditorWindow::paintGL()
{
	// очищаем окно
	glClear(GL_COLOR_BUFFER_BIT);

	// установка позиции камеры и зума
	glLoadIdentity();
	glScaled(mZoom, mZoom, 1.0);
	glTranslated(-mCameraPos.x(), -mCameraPos.y(), 0.0);

	// определяем шаг сетки, который должен быть не меньше 8 пикселей
	int cellSize = 16;
	while (cellSize * mZoom < 8.0)
		cellSize *= 4;

	// определение видимой области
	QRectF visibleRect(mCameraPos.x(), mCameraPos.y(), width() / mZoom, height() / mZoom);

	// определение номеров линий для отрисовки
	int left = static_cast<int>(visibleRect.left() / cellSize);
	int top = static_cast<int>(visibleRect.top() / cellSize);
	int right = static_cast<int>(visibleRect.right() / cellSize);
	int bottom = static_cast<int>(visibleRect.bottom() / cellSize);

	// начинаем рисовать линии
	glBegin(GL_LINES);

	// отрисовка вспомогательных линий
	qglColor(QColor(39, 45, 56));

	for (int i = left; i <= right; ++i)
		if (i % 5 != 0)
		{
			glVertex2d(i * cellSize, visibleRect.top());
			glVertex2d(i * cellSize, visibleRect.bottom());
		}

	for (int i = top; i <= bottom; ++i)
		if (i % 5 != 0)
		{
			glVertex2d(visibleRect.left(), i * cellSize);
			glVertex2d(visibleRect.right(), i * cellSize);
		}

	// отрисовка основных линий
	qglColor(QColor(51, 57, 73));

	for (int i = left; i <= right; ++i)
		if (i % 5 == 0)
		{
			glVertex2d(i * cellSize, visibleRect.top());
			glVertex2d(i * cellSize, visibleRect.bottom());
		}

	for (int i = top; i <= bottom; ++i)
		if (i % 5 == 0)
		{
			glVertex2d(visibleRect.left(), i * cellSize);
			glVertex2d(visibleRect.right(), i * cellSize);
		}

	// рисуем ось X
	qglColor(QColor(109, 36, 38));
	glVertex2d(visibleRect.left(), 0.0);
	glVertex2d(visibleRect.right(), 0.0);

	// рисуем ось Y
	qglColor(QColor(35, 110, 38));
	glVertex2d(0.0, visibleRect.top());
	glVertex2d(0.0, visibleRect.bottom());

	// заканчиваем рисовать линии
	glEnd();

	// рисуем локацию
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	mLocation->getRootLayer()->draw();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	// сбрасываем матрицу трансформации для рисования в оконных координатах
	glLoadIdentity();

	// рисуем прямоугольник выделения
	if (!mSelectedObjects.empty())
	{
		// рисуем прямоугольники выделения объектов
		foreach (GameObject *object, mSelectedObjects)
		{
			QRectF rect = worldRectToWindow(object->getBoundingRect());
			Utils::drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), QColor(0, 127, 255));
		}

		// рисуем общий прямоугольник выделения
		QRectF rect = worldRectToWindow(mSnappedRect);
		Utils::drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), QColor(0, 127, 255));

		// рисуем маркеры выделения
		QPointF center(qFloor(rect.center().x()), qFloor(rect.center().y()));
		drawSelectionMarker(rect.left(), rect.top());
		drawSelectionMarker(rect.left(), center.y());
		drawSelectionMarker(rect.left(), rect.bottom() - 1.0);
		drawSelectionMarker(center.x(), rect.bottom() - 1.0);
		drawSelectionMarker(rect.right() - 1.0, rect.bottom() - 1.0);
		drawSelectionMarker(rect.right() - 1.0, center.y());
		drawSelectionMarker(rect.right() - 1.0, rect.top());
		drawSelectionMarker(center.x(), rect.top());

		// рисуем центр вращения
		if (mRotationMode)
		{
			QPointF center = worldToWindow(mSnappedCenter);
			qreal x = qFloor(center.x()) + 0.5, y = qFloor(center.y()) + 0.5;
			qreal offset = qFloor(CENTER_SIZE / 2.0);
			QColor color(0, 127, 255);
			Utils::drawLine(QPointF(x - offset, y), QPointF(x + offset, y), color);
			Utils::drawLine(QPointF(x, y - offset), QPointF(x, y + offset), color);
		}
	}

	// рисуем рамку выделения
	if (mEditorState == STATE_SELECT && !mSelectionRect.isNull())
	{
		glEnable(GL_BLEND);
		QRectF rect = worldRectToWindow(mSelectionRect.normalized());
		Utils::fillRect(rect, QColor(0, 100, 255, 60));
		Utils::drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), QColor(0, 127, 255));
		glDisable(GL_BLEND);
	}
}

void EditorWindow::resizeGL(int width, int height)
{
	// устанавливаем стандартную систему координат (0, 0, width, height) с началом координат в левом верхнем углу
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width, height, 0.0);
	glMatrixMode(GL_MODELVIEW);
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

		qreal size = CENTER_SIZE / mZoom;
		qreal offset = size / 2.0;
		GameObject *object;

		if (mRotationMode && QRectF(mSnappedCenter.x() - offset, mSnappedCenter.y() - offset, size, size).contains(pos))
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
				if (mRotationMode)
					mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mSnappedCenter;

				// посылаем сигналы об изменении локации и слоев
				emitLayerChangedSignals(getParentLayers());
			}
			else if (mEditorState == STATE_MOVE_CENTER)
			{
				// обновляем координаты центра вращения
				if (mRotationMode)
					mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mSnappedCenter;

				// посылаем сигнал об изменении локации
				mChanged = true;
				emit locationChanged(mChanged);
			}
		}
		else if (mEditorState == STATE_MOVE && !mFirstTimeSelected)
		{
			// переключаем режим поворота при щелчках по объекту
			mRotationMode = !mRotationMode;
			if (mRotationMode)
				mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mOriginalRect.center();
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
			mMoveRect.translate(delta);
			QPointF offset = getTranslationVector(mOriginalRect.topLeft(), mMoveRect.topLeft(), (event->modifiers() & Qt::ShiftModifier) != 0);

			// перемещаем все выделенные объекты
			for (int i = 0; i < mSelectedObjects.size(); ++i)
				mSelectedObjects[i]->setPosition(mOriginalPositions[i] + offset);
		}
		else if (mEditorState == STATE_RESIZE)
		{
			// определяем значение масштаба и координаты точки, относительно которой он производится
			bool keepProportions = (event->modifiers() & Qt::ShiftModifier) != 0 || mHasRotatedObjects;
			QPointF pivot, scale;
			switch (mSelectionMarker)
			{
			case MARKER_TOP_LEFT:
				pivot = mOriginalRect.bottomRight();
				scale = QPointF((pivot.x() - pos.x()) / mOriginalRect.width(), (pivot.y() - pos.y()) / mOriginalRect.height());
				if (keepProportions)
					scale.rx() = scale.ry() = qMin(scale.x(), scale.y());
				break;

			case MARKER_CENTER_LEFT:
				pivot = QPointF(mOriginalRect.right(), mOriginalRect.center().y());
				scale.rx() = (pivot.x() - pos.x()) / mOriginalRect.width();
				scale.ry() = keepProportions ? scale.x() : 1.0;
				break;

			case MARKER_BOTTOM_LEFT:
				pivot = mOriginalRect.topRight();
				scale = QPointF((pivot.x() - pos.x()) / mOriginalRect.width(), (pos.y() - pivot.y()) / mOriginalRect.height());
				if (keepProportions)
					scale.rx() = scale.ry() = qMin(scale.x(), scale.y());
				break;

			case MARKER_BOTTOM_CENTER:
				pivot = QPointF(mOriginalRect.center().x(), mOriginalRect.top());
				scale.ry() = (pos.y() - pivot.y()) / mOriginalRect.height();
				scale.rx() = keepProportions ? scale.y() : 1.0;
				break;

			case MARKER_BOTTOM_RIGHT:
				pivot = mOriginalRect.topLeft();
				scale = QPointF((pos.x() - pivot.x()) / mOriginalRect.width(), (pos.y() - pivot.y()) / mOriginalRect.height());
				if (keepProportions)
					scale.rx() = scale.ry() = qMin(scale.x(), scale.y());
				break;

			case MARKER_CENTER_RIGHT:
				pivot = QPointF(mOriginalRect.left(), mOriginalRect.center().y());
				scale.rx() = (pos.x() - pivot.x()) / mOriginalRect.width();
				scale.ry() = keepProportions ? scale.x() : 1.0;
				break;

			case MARKER_TOP_RIGHT:
				pivot = mOriginalRect.bottomLeft();
				scale = QPointF((pos.x() - pivot.x()) / mOriginalRect.width(), (pivot.y() - pos.y()) / mOriginalRect.height());
				if (keepProportions)
					scale.rx() = scale.ry() = qMin(scale.x(), scale.y());
				break;

			case MARKER_TOP_CENTER:
				pivot = QPointF(mOriginalRect.center().x(), mOriginalRect.bottom());
				scale.ry() = (pivot.y() - pos.y()) / mOriginalRect.height();
				scale.rx() = keepProportions ? scale.y() : 1.0;
				break;

			default:
				break;
			}

			// пересчитываем положение и размеры выделенных объектов
			for (int i = 0; i < mSelectedObjects.size(); ++i)
			{
				mSelectedObjects[i]->setPosition(QPointF((mOriginalPositions[i].x() - pivot.x()) * scale.x() + pivot.x(),
					(mOriginalPositions[i].y() - pivot.y()) * scale.y() + pivot.y()));
				if (keepProportions || mOriginalAngles[i] == 0.0 || mOriginalAngles[i] == 180.0)
					mSelectedObjects[i]->setSize(QSizeF(mOriginalSizes[i].width() * scale.x(), mOriginalSizes[i].height() * scale.y()));
				else
					mSelectedObjects[i]->setSize(QSizeF(mOriginalSizes[i].width() * scale.y(), mOriginalSizes[i].height() * scale.x()));
			}
		}
		else if (mEditorState == STATE_ROTATE)
		{
			// определяем текущий угол поворота в радианах относительно начального вектора
			QVector2D currVector = QVector2D(pos - mOriginalCenter).normalized();
			qreal angle = qAcos(QVector2D::dotProduct(currVector, mRotationVector));
			if (QString::number(angle) == "nan" || qAbs(angle) < 0.001)
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
				QPointF pt = mOriginalPositions[i] - mOriginalCenter;
				mSelectedObjects[i]->setPosition(QPointF(pt.x() * qCos(angle) - pt.y() * qSin(angle) + mOriginalCenter.x(),
					pt.x() * qSin(angle) + pt.y() * qCos(angle) + mOriginalCenter.y()));
				mSelectedObjects[i]->setRotationAngle(mOriginalAngles[i] + Utils::radToDeg(angle));
			}
		}
		else if (mEditorState == STATE_MOVE_CENTER)
		{
			// перемещаем центр вращения
			QPointF offset = getTranslationVector(mOriginalCenter, pos, (event->modifiers() & Qt::ShiftModifier) != 0);
			mSnappedCenter = mOriginalCenter + offset;
			if (mSelectedObjects.size() == 1)
				mSelectedObjects.front()->setRotationCenter(mSnappedCenter);
		}

		// пересчитываем общий прямоугольник выделения
		if (mEditorState == STATE_MOVE || mEditorState == STATE_RESIZE || mEditorState == STATE_ROTATE)
		{
			mSnappedRect = mSelectedObjects.front()->getBoundingRect();
			foreach (GameObject *object, mSelectedObjects)
				mSnappedRect |= object->getBoundingRect();
		}

		// пересчитываем координаты центра вращения
		if (mEditorState == STATE_MOVE && mRotationMode)
		{
			mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter()
				: mOriginalCenter + (mSnappedRect.topLeft() - mOriginalRect.topLeft());
		}
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
		if (mRotationMode)
			mOriginalCenter = mSnappedCenter = mOriginalCenter + offset;

		// обновляем выделение и курсор мыши
		selectGameObjects(mSelectedObjects);
		updateMouseCursor(windowToWorld(mapFromGlobal(QCursor::pos())));

		// посылаем сигналы об изменении локации и слоев
		emitLayerChangedSignals(getParentLayers());
	}
}

void EditorWindow::dragEnterEvent(QDragEnterEvent *event)
{
	// проверяем, что текущий активный слой доступен
	if (mLocation->getAvailableLayer() == NULL)
		return;

	// проверяем, что перетаскивается элемент из QListWidget/QTreeWidget
	QByteArray data = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
	if (data.isEmpty())
		return;

	// распаковываем полученные данные
	int row, col;
	QMap<int, QVariant> roles;
	QDataStream stream(data);
	stream >> row >> col >> roles;

	// проверяем строку в UserRole
	QString str = roles.value(Qt::UserRole).toString();
	if (str.startsWith("sprite://"))
		event->acceptProposedAction();
}

void EditorWindow::dropEvent(QDropEvent *event)
{
	// проверяем, что текущий активный слой доступен
	if (mLocation->getAvailableLayer() == NULL)
		return;

	// проверяем, что перетаскивается элемент из QListWidget/QTreeWidget
	QByteArray data = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
	if (data.isEmpty())
		return;

	// распаковываем полученные данные
	int row, col;
	QMap<int, QVariant> roles;
	QDataStream stream(data);
	stream >> row >> col >> roles;

	// выделяем и проверяем имя файла
	QString fileName;
	QString str = roles.value(Qt::UserRole).toString();
	if (str.startsWith("sprite://"))
		fileName = str.mid(9);
	else
		return;

	if (!Utils::isFileNameValid(fileName))
	{
		QMessageBox::warning(this, "", "Неверный путь к файлу " + fileName + "\nПереименуйте файлы и папки так, чтобы они "
			"начинались с буквы и состояли только из маленьких латинских букв, цифр и знаков подчеркивания");
		return;
	}

	// создаем нужный игровой объект в окне редактора
	GameObject *object;
	QPointF pos = windowToWorld(event->pos());
	if (str.startsWith("sprite://"))
		object = mLocation->createSprite(pos, fileName);
	else
		return;

	if (object == NULL)
	{
		QMessageBox::critical(this, "", "Ошибка загрузки файла " + fileName);
		return;
	}

	// выделяем созданный объект
	selectGameObject(object);

	// обновляем курсор мыши и посылаем сигналы об изменении локации и слоев
	updateMouseCursor(pos);
	emitLayerChangedSignals(getParentLayers());

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

QPointF EditorWindow::getTranslationVector(const QPointF &start, const QPointF &end, bool shift) const
{
	if (shift)
	{
		// определяем угол между исходным вектором и осью OX в радианах
		QVector2D vector(end - start);
		qreal angle = qAcos(vector.normalized().x());
		if (vector.y() < 0.0)
			angle = -angle;

		// округляем угол до величины, кратной 45 градусам и получаем ось перемещения
		angle = qFloor((angle + Utils::PI / 8.0) / (Utils::PI / 4.0)) * (Utils::PI / 4.0);
		QVector2D axis(qCos(angle), qSin(angle));

		// возвращаем проекцию исходного вектора на ось перемещения
		return (QVector2D::dotProduct(vector, axis) * axis).toPointF();
	}

	return end - start;
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
		mSnappedRect = mMoveRect = mOriginalRect;
	}

	// проверяем на изменение выделения
	if (objects != mSelectedObjects)
	{
		// сохраняем список выделенных объектов
		mSelectedObjects = objects;

		// обновляем состояние режима поворота
		if (mSelectedObjects.empty())
			mRotationMode = false;
		else if (mRotationMode)
			mOriginalCenter = mSnappedCenter = mSelectedObjects.size() == 1 ? mSelectedObjects.front()->getRotationCenter() : mOriginalRect.center();

		// посылаем сигнал об изменении выделения
		emit selectionChanged(!mSelectedObjects.empty());
	}
}

void EditorWindow::updateMouseCursor(const QPointF &pos)
{
	if (mEditorState == STATE_IDLE)
	{
		qreal size = CENTER_SIZE / mZoom;
		qreal offset = size / 2.0;
		SelectionMarker marker;

		if (mRotationMode && QRectF(mSnappedCenter.x() - offset, mSnappedCenter.y() - offset, size, size).contains(pos))
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

void EditorWindow::drawSelectionMarker(qreal x, qreal y)
{
	// рисуем круглый или прямоугольный маркер выделения
	QColor fillColor(0, 127, 255);
	QColor frameColor(100, 100, 100);
	if (mRotationMode)
	{
		QPointF center(x + 0.5, y + 0.5);
		qreal radius = MARKER_SIZE / 2.0;
		Utils::fillCircle(center, radius, fillColor);
		Utils::drawCircle(center, radius, frameColor);
	}
	else
	{
		qreal offset = qFloor(MARKER_SIZE / 2.0);
		QRectF rect(x - offset, y - offset, MARKER_SIZE, MARKER_SIZE);
		Utils::fillRect(rect, fillColor);
		Utils::drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), frameColor);
	}
}
