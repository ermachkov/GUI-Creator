#include "pch.h"
#include "layers_tree_widget.h"
#include "base_layer.h"
#include "location.h"
#include "layer.h"
#include "layer_group.h"
#include "utils.h"

// FIXME: условия надо полностью писать: topItem->parent() != NULL
// в любом другом языке сразу ошибка компила вывалится, потому что в if выражения должны быть только типа bool, а ему всякие указатели пихают

LayersTreeWidget::LayersTreeWidget(QWidget *parent)
: QTreeWidget(parent), mPrimaryGLWidget(NULL), mFrameBuffer(NULL)
{
	// присоединение сигналов к слотам
	connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(onCurrentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
	connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(onItemClicked(QTreeWidgetItem *, int)));
	connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)),  this, SLOT(onItemChanged(QTreeWidgetItem *, int)));
	connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(onItemExpanded(QTreeWidgetItem *)));
	connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(onItemCollapsed(QTreeWidgetItem *)));

	// запрещение редактирования 1го и 2го столбца через назначение делегата
	setItemDelegate(new EditorDelegate(this));

	// загрузка иконки для группы в окне слоёв
	mLayerGroupIcon = QIcon(":/images/layer_group.png");

	// загрузка иконок для видимости
	mLayerVisibleIcons[BaseLayer::LAYER_VISIBLE] = QIcon(":/images/layer_visible.png");
	mLayerVisibleIcons[BaseLayer::LAYER_PARTIALLY_VISIBLE] = QIcon(":/images/layer_partially_visible.png");
	mLayerVisibleIcons[BaseLayer::LAYER_INVISIBLE] = QIcon(":/images/layer_invisible.png");

	// загрузка иконок для блокировки слоя
	mLayerLockedIcons[BaseLayer::LAYER_UNLOCKED] = QIcon(":/images/layer_unlocked.png");
	mLayerLockedIcons[BaseLayer::LAYER_PARTIALLY_UNLOCKED] = QIcon(":/images/layer_partially_locked.png");
	mLayerLockedIcons[BaseLayer::LAYER_LOCKED] = QIcon(":/images/layer_locked.png");

	// настройка контекстного меню
	mContextMenu = new QMenu(this);

	mDuplicateAction = mContextMenu->addAction("&Дублирование");
	mContextMenu->addSeparator();
	mNewGroupAction = mContextMenu->addAction("Новая &группа");
	mNewLayerAction = mContextMenu->addAction("Новый &слой");
	mDeleteAction = mContextMenu->addAction("&Удалить");
	connect(mContextMenu, SIGNAL(triggered(QAction*)), SLOT(onContextMenuTriggered(QAction*)));
}

LayersTreeWidget::~LayersTreeWidget()
{
	mPrimaryGLWidget->makeCurrent();

	Q_ASSERT(mFrameBuffer);
	delete mFrameBuffer;

	Q_ASSERT(mContextMenu);
	delete mContextMenu;
}


void LayersTreeWidget::setCurrentLocation(Location *location)
{
	// сохранение текущей локации
	mCurrentLocation = location;

	// сохранение текущего слоя
	BaseLayer *baseCurrent = mCurrentLocation->getActiveLayer();

	// Отчистка окна слоев
	clear();

	// связывание с корневым элементом слоёв
	createTreeItems(invisibleRootItem(), mCurrentLocation->getRootLayer());

	// восстановление текущего слоя
	if (baseCurrent != NULL)
	{
		Q_ASSERT(findItemByBaseLayer(baseCurrent) != NULL);
		setCurrentItem(findItemByBaseLayer(baseCurrent));
	}
}

Location *LayersTreeWidget::getCurrentLocation() const
{
	return mCurrentLocation;
}

void LayersTreeWidget::setPrimaryGLWidget(QGLWidget *primaryGLWidget)
{
	mPrimaryGLWidget = primaryGLWidget;

	mPrimaryGLWidget->makeCurrent();

	delete mFrameBuffer;
	mFrameBuffer = new QGLFramebufferObject(WIDTH_THUMBNAIL, HEIGHT_THUMBNAIL);
}

void LayersTreeWidget::DEBUG_TREES()
{
	QList<QTreeWidgetItem *> stackItem;
	QList<BaseLayer *> stackBase;

	stackItem.push_back(invisibleRootItem());
	stackBase.push_back(mCurrentLocation->getRootLayer());

	while (!stackItem.empty() || !stackBase.empty())
	{
		QTreeWidgetItem *currentItem = stackItem.last();
		BaseLayer *currentBase = stackBase.last();

		stackItem.pop_back();
		stackBase.pop_back();

		if (getBaseLayer(currentItem) != currentBase
			|| currentItem->text(DATA_COLUMN) != currentBase->getName()
			|| currentItem->isExpanded() != currentBase->isExpanded()
			|| currentItem->childCount() != currentBase->getChildLayers().size())
		{
			qDebug() << "Error: Trees are not equal:";

			//if (getBaseLayer(currentItem) != currentBase)
				qDebug() << "-base:" << getBaseLayer(currentItem) << currentBase;
			//if (currentItem->text(DATA_COLUMN) != currentBase->getName())
				qDebug() << currentItem->text(DATA_COLUMN) << currentBase->getName();
			//if (currentItem->isExpanded() != currentBase->isExpanded())
				qDebug() << "-isExpanded:" << currentItem->isExpanded() << currentBase->isExpanded();
			//if (currentItem->childCount() != currentBase->getChildLayers().size())
				qDebug() << "-childCount:" << currentItem->childCount() << currentBase->getChildLayers().size();

			return;
		}

		for (int i = 0; i < currentItem->childCount(); ++i)
			stackItem.push_back(currentItem->child(i));
		stackBase += currentBase->getChildLayers();
	}

	// сравнение текущего элемента
	if (getBaseLayer(currentItem()) != mCurrentLocation->getActiveLayer())
	{
		qDebug() << "Error: Active with current elements are not equal";
		return;
	}
}

void LayersTreeWidget::onAddLayerGroup()
{
	if (currentItem() == NULL)
	{
		// текущий элемент - корень

		// добавление в корень нулевым элементом
		int index = 0;
		insertNewLayerGroup(index, invisibleRootItem());
	}
	else if (isLayerGroup(currentItem()))
	{
		// текущий элемент - группа

		// запрет добавления при превышениии допустимой вложенности
		if (getLayerDepth(currentItem()) + 1 > BaseLayer::MAX_NESTED_LAYERS)
		{
			QMessageBox::warning(this, "", "Превышение допустимого уровня вложенности.");
			return;
		}

		// добавление внутрь группы 0-вым элементом
		insertNewLayerGroup(0, currentItem());
	}
	else if (isLayer(currentItem()))
	{
		// текущий элемент - слой

		// получение родителя
		QTreeWidgetItem *currentItemParent = currentItem()->parent() ? currentItem()->parent() : invisibleRootItem();
		// получение индекса
		int index = currentIndex().row();

		insertNewLayerGroup(index, currentItemParent);
	}
	else
	{
		qDebug() << "Error in LayersWindow::onAddLayerGroup";
	}

	emit locationChanged();

	DEBUG_TREES();
}

void LayersTreeWidget::onAddLayer()
{
	if (currentItem() == NULL)
	{
		// текущий элемент - корень

		// добавление в корень нулевым элементом
		int index = 0;
		insertNewLayer(index, invisibleRootItem());
	}
	else if (isLayerGroup(currentItem()))
	{
		// текущий элемент - группа

		// запрет добавления при превышениии допустимой вложенности
		if (getLayerDepth(currentItem()) + 1 > BaseLayer::MAX_NESTED_LAYERS)
		{
			QMessageBox::warning(this, "", "Превышение допустимого уровня вложенности.");
			return;
		}

		// добавление внутрь группы 0-вым элементом
		insertNewLayer(0, currentItem());
	}
	else if (isLayer(currentItem()))
	{
		// текущий элемент - слой

		QTreeWidgetItem *currentItemParent = currentItem()->parent() ? currentItem()->parent() : invisibleRootItem();
		int index = currentIndex().row();
		insertNewLayer(index, currentItemParent);
	}
	else
	{
		qDebug() << "Error in LayersWindow::onAddLayer";
	}

	emit locationChanged();

	DEBUG_TREES();
}

void LayersTreeWidget::onDelete()
{
	// окно с подтверждением удаления
	int ret = QMessageBox::warning(this, "", "Удалить слой?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (ret == QMessageBox::Yes)
	{
		foreach (QTreeWidgetItem *item, selectedItems())
		{
			// удаление текущего BaseLayer * связанного с QTreeWidgetItem *
			delete getBaseLayer(item);
			// удаление QTreeWidgetItem
			delete item;
		}

		emit locationChanged();
		emit layerChanged();
	}

	DEBUG_TREES();
}

void LayersTreeWidget::onEditorWindowLayerChanged(Location *location, BaseLayer *layer)
{
	// генерируем иконку предпросмотра
	QIcon icon = createLayerIcon(layer);
	layer->setThumbnail(icon);

	// устанавливаем иконку для item, если локация текущая
	if (mCurrentLocation == location)
		findItemByBaseLayer(layer)->setIcon(DATA_COLUMN, icon);
}

void LayersTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
	// проверяем, что перетаскивается элемент из LayersTreeWidget
	if (event->source() != this)
		return;

	QTreeWidget::dragEnterEvent(event);

	// отладочное сравнение деревьев
	DEBUG_TREES();
}

void LayersTreeWidget::dropEvent(QDropEvent *event)
{
	// проверяем, что перетаскивается элемент из LayersTreeWidget
	if (event->source() != this)
		return;

	// сохранение эвента
	Qt::DropAction currentDropAction = event->dropAction();

	// отбрасывание обработки левых действий
	if (currentDropAction != Qt::MoveAction && currentDropAction != Qt::CopyAction)
		return;

	// создание множества неповторяющихся item выбранных элементов
	QList<QTreeWidgetItem *> listItems = selectedItems();

	Q_ASSERT(listItems.size() == 1);

	// получение QTreeWidgetItem * перетаскиваемого элемента
	QTreeWidgetItem *itemSelected = listItems.front();
	// получение родителя
	QTreeWidgetItem *itemSelectedParent = itemSelected->parent() ? itemSelected->parent() : invisibleRootItem();
	// получение BaseLayer * перетаскиваемого элемента
	BaseLayer *baseSelected = getBaseLayer(itemSelected);
	// получение родителя
	BaseLayer *baseSelectedParent = getBaseLayer(itemSelectedParent);
	// получение индекса перетаскиваемого элемента
	int indexSelected = itemSelectedParent->indexOfChild(itemSelected);

	// подмена event на копирование
	event->setDropAction(Qt::CopyAction);

	QTreeWidget::dropEvent(event);

	// itemAdded - новый добавленный элемент
	QTreeWidgetItem *itemAdded;

	// поиск нового добавленного элемента QTreeWidgetItem в дереве
	QList<QTreeWidgetItem *> stackItem;
	stackItem.push_back(invisibleRootItem());
	while (!stackItem.empty())
	{
		QTreeWidgetItem *currentItem = stackItem.last();
		stackItem.pop_back();

		if (getBaseLayer(currentItem) == baseSelected && currentItem != listItems.front())
		{
			// BaseLayer * совпадает, но сам QTreeWidgetItem * не совпадает

			// переназначение QTreeWidgetItem * добавленного элемента
			itemAdded = currentItem;
			// обнуление BaseLayer у нового элемента
			setBaseLayer(itemAdded, NULL);

			break;
		}

		for (int i = 0; i < currentItem->childCount(); ++i)
			stackItem.push_back(currentItem->child(i));
	}

	// получение нового местоположения элемента
	QTreeWidgetItem *itemAddedParent = itemAdded->parent() ? itemAdded->parent() : invisibleRootItem();
	int indexAdded = itemAddedParent->indexOfChild(itemAdded);
	BaseLayer *baseAddedParent = getBaseLayer(itemAddedParent);

	// запрет перетаскивания при превышениии допустимой вложенности
	if (getMaxDepthChilds(itemSelected) + getLayerDepth(itemAdded) > BaseLayer::MAX_NESTED_LAYERS)
	{
		QMessageBox::warning(this, "", "Превышение допустимого уровня вложенности.");

		delete itemAdded;

		return;
	}

	// синхронизация дерева BaseLayer с деревом Item

	// создание дубликата только если копирование а не перетаскивание
	BaseLayer *baseAdded;
	if (currentDropAction == Qt::CopyAction)
	{
		// дублирование дерева BaseLayer с прикреплением к новому родителю
		baseAdded = baseSelected->duplicate(baseAddedParent, indexAdded);
	}
	else if (currentDropAction == Qt::MoveAction)
	{
		// отцепление от родителя
		baseSelectedParent->removeChildLayer(indexSelected);

		// родители совпадают и предшедствующее удаление сместило индекс - добавление с учетом порядка следования
		if (itemAddedParent == itemSelectedParent && indexAdded > indexSelected)
			indexAdded = indexAdded - 1;

		baseAddedParent->insertChildLayer(indexAdded, baseSelected);

		baseAdded = baseSelected;
		baseSelected = NULL;
	}

	if (currentDropAction == Qt::CopyAction)
	{
		// модификация имени нового слоя
		QString newName = generateCopyName(itemAdded->text(DATA_COLUMN));
		baseAdded->setName(newName);
	}

	// установка развернутости
	if (isLayerGroup(itemAddedParent) && itemAddedParent != invisibleRootItem())
	{
		// ? 1. установить Expanded для родителя baseAdded (BaseLayer *)
		baseAddedParent->setExpanded(true);
		// ? 2. установить Expanded для родителя itemAdded (QTreeWidgetItem *)
		itemAddedParent->setExpanded(true);
	}

	// ручное досоздание(воссоздание) дерева
	createTreeItems(itemAdded, baseAdded);

	if (currentDropAction == Qt::MoveAction)
	{
		// удаление старой ветки item
		delete itemSelected;
	}

	emit locationChanged();

	// отладочное сравнение деревьев
	DEBUG_TREES();
}

void LayersTreeWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
	{
		// условие нажатия клавиши "Delete" если один слой или одна активная папка
		if (mCurrentLocation->getRootLayer()->getNumChildLayers() != 1
			|| mCurrentLocation->getActiveLayer() != mCurrentLocation->getRootLayer()->getChildLayer(0))
		{
			onDelete();
		}

	}

	QTreeWidget::keyPressEvent(event);

	// отладочное сравнение деревьев
	DEBUG_TREES();
}

void LayersTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
	// определение попадания по item
	QTreeWidgetItem *itemClicked = itemAt(event->pos());

	// активация/деактивация пункта контекстного меню "Удалить" если один слой или одна активная папка
	bool enableDelete = mCurrentLocation->getRootLayer()->getNumChildLayers() != 1
		|| mCurrentLocation->getActiveLayer() != mCurrentLocation->getRootLayer()->getChildLayer(0);
	mDeleteAction->setEnabled(enableDelete);

	if (itemClicked != NULL)
	{
		mContextMenu->exec(event->globalPos());
	}
}

void LayersTreeWidget::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	if (current != NULL && getBaseLayer(current) != mCurrentLocation->getActiveLayer())
	{
		// есть выделенный item и текущие слои не совпадают

		// установка текущего текущего слоя BaseLayer *
		mCurrentLocation->setActiveLayer(getBaseLayer(current));

		emit locationChanged();
	}
/*
	else
		if (mCurrentLocation != NULL)
	{
		// все вкладки закрыты

		// установка отсутствия текущего слоя
		mCurrentLocation->setActiveLayer(NULL);
	}
*/
}

void LayersTreeWidget::onItemClicked(QTreeWidgetItem *item, int column)
{
	// определение колонки щелчка мышкой
	if (column == VISIBLE_COLUMN)
	{
		// смена видимости элементу и всем зависимым элементам
		switch (getVisibleState(item))
		{
		case BaseLayer::LAYER_INVISIBLE:
		case BaseLayer::LAYER_PARTIALLY_VISIBLE:
			// подъём вверх по родителям до первой видимой группы
			for (QTreeWidgetItem *currentItem = item;
				 currentItem != NULL && getVisibleState(currentItem) != BaseLayer::LAYER_VISIBLE;
				 currentItem = currentItem->parent())
			{
				// 0 -> 1, 1/2 -> 1 = установка видимости
				setVisibleState(currentItem, BaseLayer::LAYER_VISIBLE);

				// установка видимости потомкам невидимого родителя
				changeChildrenPartiallyVisibleState(currentItem, true);
			}
			break;

		case BaseLayer::LAYER_VISIBLE:
			// 1 -> 0 = снятие видимости текущего элемента
			setVisibleState(item, BaseLayer::LAYER_INVISIBLE);

			changeChildrenPartiallyVisibleState(item, false);
			break;

		default:
			qDebug() << "Error in LayersWindow::on_mLayersTreeWidget_itemClicked (changeVisiblility)";
			break;
		}

		emit locationChanged();
		emit layerChanged();
	}
	else if	(column == LOCK_COLUMN)
	{
		// смена блокировки элемента и всем зависимым элементам
		switch (getLockState(item))
		{
		case BaseLayer::LAYER_UNLOCKED:
			// элемент разблокирован
			if (isLayerGroup(item))
			{
				changeChildrenPartiallyLockState(item, true);
			}
			// 0 -> 1
			setLockState(item, BaseLayer::LAYER_LOCKED);
			break;

		case BaseLayer::LAYER_PARTIALLY_UNLOCKED:
			// элемент заблокирован родительским элементом
			// 1/2 -> 1
			setLockState(item, BaseLayer::LAYER_LOCKED);
			break;

		case BaseLayer::LAYER_LOCKED:
			// элемент заблокирован
			if (hasLockedParent(item))
			{	// есть заблокированная родительская папка
				// 1 -> 1/2
				setLockState(item, BaseLayer::LAYER_PARTIALLY_UNLOCKED);
			}
			else
			{	// нет заблокированной родительской папки

				if (isLayerGroup(item))
				{	// элемент - группа
					changeChildrenPartiallyLockState(item, false);
				}

				// 1 -> 0
				setLockState(item, BaseLayer::LAYER_UNLOCKED);
			}
			break;

		default:
			qDebug() << "Error in LayersWindow::on_mLayersTreeWidget_itemClicked (changeLock)";
			break;
		}

		emit layerChanged();
		emit locationChanged();
	}
	else if (column == DATA_COLUMN)
	{
	}
	else
		qDebug() << "Warning in on_mLayersTreeWidget_itemClicked";
}

void LayersTreeWidget::onItemChanged(QTreeWidgetItem *item, int column)
{
	if (column == DATA_COLUMN
		&& getBaseLayer(item) != NULL
		&& item->text(DATA_COLUMN) != getBaseLayer(item)->getName())
	{
		getBaseLayer(item)->setName(item->text(DATA_COLUMN));

		emit locationChanged();

		DEBUG_TREES();
	}
}

void LayersTreeWidget::onItemExpanded(QTreeWidgetItem *item)
{
	BaseLayer *base = getBaseLayer(item);

	if (!base->isExpanded())
	{
		base->setExpanded(true);

		emit locationChanged();
	}
}

void LayersTreeWidget::onItemCollapsed(QTreeWidgetItem *item)
{
	BaseLayer *base = getBaseLayer(item);

	if (base->isExpanded())
	{
		base->setExpanded(false);

		emit locationChanged();
	}
}

void LayersTreeWidget::onContextMenuTriggered(QAction *action)
{
	if (action == mDuplicateAction)
	{
		// получение QTreeWidgetItem * и BaseLayer * перетаскиваемого элемента через выбранный
		QList<QTreeWidgetItem *> listItems = selectedItems();
		Q_ASSERT(listItems.size() == 1);
		QTreeWidgetItem *itemSelected = *listItems.begin();
		BaseLayer *baseSelected = getBaseLayer(itemSelected);

		// получение родителя
		QTreeWidgetItem *itemSelectedParent = itemSelected->parent() ? itemSelected->parent() : invisibleRootItem();
		BaseLayer *baseSelectedParent = getBaseLayer(itemSelectedParent);

		// получение индекса
		int indexSelected = itemSelectedParent->indexOfChild(itemSelected);

		// физическое создание элемента - потомка в дереве ГУИ
		QTreeWidgetItem *itemAdded = new QTreeWidgetItem;
		itemSelectedParent->insertChild(indexSelected, itemAdded);

		// дублирование дерева BaseLayer с прикреплением к новому родителю
		BaseLayer *baseAdded = baseSelected->duplicate(baseSelectedParent, indexSelected);

		// модификация имени нового слоя
		QString newName = generateCopyName(itemSelected->text(DATA_COLUMN));
		baseAdded->setName(newName);

		// ручное досоздание дерева
		createTreeItems(itemAdded, baseAdded);
	}
	else if (action == mNewGroupAction)
	{
		onAddLayerGroup();
	}
	else if (action == mNewLayerAction)
	{
		onAddLayer();
	}
	else if (action == mDeleteAction)
	{
		onDelete();
	}
	else
		Q_ASSERT(false);

	DEBUG_TREES();
}

LayersTreeWidget::EditorDelegate::EditorDelegate(QObject *parent)
: QItemDelegate(parent)
{
}

QWidget *LayersTreeWidget::EditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.column() == VISIBLE_COLUMN || index.column() == LOCK_COLUMN)
		return NULL;

	return QItemDelegate::createEditor(parent, option, index);
}

void LayersTreeWidget::EditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	// получение введенной строки
	QByteArray name = editor->metaObject()->userProperty().name();

	if (name != QByteArray("text"))
		qDebug() << "Error in setModelData";

	if (editor->property(name) == "")
		return;

	QItemDelegate::setModelData(editor, model, index);
}

QSize LayersTreeWidget::EditorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size = QItemDelegate::sizeHint(option, index);

	// добавление отступа
	size.setHeight(size.height() + 2);

	return size;
}

QTreeWidgetItem *LayersTreeWidget::insertNewLayerGroup(int index, QTreeWidgetItem *parent)
{
	QTreeWidgetItem *newItem = new QTreeWidgetItem;

	// установка иконки - маленькая папочка
	newItem->setIcon(DATA_COLUMN, mLayerGroupIcon);

	// разрешение перетаскивания на элемент, разрешение перетаскивания элемента, разрешение переименования
	newItem->setFlags(newItem->flags() | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);

	// получение указателя BaseLayer на родителя
	BaseLayer *baseParent = getBaseLayer(parent);
	// создание BaseLayer с прикреплением к родителю
	BaseLayer *baseGroup = mCurrentLocation->createLayerGroup(baseParent, index);

	// установка имени папки
	newItem->setText(DATA_COLUMN, baseGroup->getName());

	//newItem->setFlags(newItem->flags());

	// установка текущести
	//currentLocation->setActiveLayer(baseGroup);
	// сохранение указателя на BaseLayer в поле DATA_COLUMN
	setBaseLayer(newItem, baseGroup);

	// добавление с родителем
	parent->insertChild(index, newItem);

	// добавление "галочки" видимости
	if (hasInvisibleParent(newItem))
	{
		// есть невидимый родитель

		setVisibleState(newItem, BaseLayer::LAYER_PARTIALLY_VISIBLE);
	}
	else
	{
		// нет невидимого родителя

		setVisibleState(newItem, BaseLayer::LAYER_VISIBLE);
	}

	// добавление "галочки" замочка
	if (hasLockedParent(newItem))
	{
		// есть заблокированный родитель

		setLockState(newItem, BaseLayer::LAYER_PARTIALLY_UNLOCKED);
	}
	else
	{
		// нет заблокированного родителя

		setLockState(newItem, BaseLayer::LAYER_UNLOCKED);
	}

	// сохранение иконки предпросмотра
	baseGroup->setThumbnail(mLayerGroupIcon);

	// установка текущим элементом нового добавленного
	setCurrentItem(newItem);

	return newItem;
}

void LayersTreeWidget::insertNewLayer(int index, QTreeWidgetItem *parent)
{
	QTreeWidgetItem *newItem = new QTreeWidgetItem;

	// запрещение перетаскивания на элемент, разрешение перетаскивания элемента, разрешение переименования
	newItem->setFlags((newItem->flags() & ~Qt::ItemIsDropEnabled) | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);

	// получение указателя BaseLayer на родителя
	BaseLayer *baseParent = getBaseLayer(parent);
	// создание BaseLayer с прикреплением к родителю
	BaseLayer *baseLayer = mCurrentLocation->createLayer(baseParent, index);

	// установка имени слоя
	newItem->setText(DATA_COLUMN, baseLayer->getName());

	// сохранение указателя на BaseLayer в поле DATA_COLUMN
	setBaseLayer(newItem, baseLayer);

	// формирование иконки предпросмотра
	QIcon icon = createLayerIcon(baseLayer);

	// установка иконки предпросмотра
	newItem->setIcon(DATA_COLUMN, icon);

	// добавление с родителем
	parent->insertChild(index, newItem);

	// добавление "галочки" видимости
	if (hasInvisibleParent(newItem))
	{	// есть невидимый родитель
		setVisibleState(newItem, BaseLayer::LAYER_PARTIALLY_VISIBLE);
	}
	else
	{	// нет невидимого родителя
		setVisibleState(newItem, BaseLayer::LAYER_VISIBLE);
	}

	// добавление "галочки" замочка
	if (hasLockedParent(newItem))
	{	// есть заблокированный родитель
		setLockState(newItem, BaseLayer::LAYER_PARTIALLY_UNLOCKED);
	}
	else
	{	// нет заблокированного родителя
		setLockState(newItem, BaseLayer::LAYER_UNLOCKED);
	}

	// сохранение иконки предпросмотра
	baseLayer->setThumbnail(icon);

	// установка текущим элементом нового добавленного
	setCurrentItem(newItem);
}

void LayersTreeWidget::createTreeItems(QTreeWidgetItem *item, BaseLayer *base)
{
	// 1. установка текста
	item->setText(DATA_COLUMN, base->getName());

	// 2. сохранение указателя на BaseLayer в поле DATA_COLUMN
	setBaseLayer(item, base);

	// формирование иконки предпросмотра
	QIcon icon;
	if (base->getThumbnail().isNull())
	{
		icon = isLayerGroup(item) ? mLayerGroupIcon : createLayerIcon(base);
		base->setThumbnail(icon);
	}
	else
	{
		icon = base->getThumbnail();
	}

	// установка иконки предпросмотра слоя
	item->setIcon(DATA_COLUMN, icon);

	// установка иконки видимости "глаз"
	setVisibleState(item, base->getVisibleState());

	// установка  иконки блокирования "замок"
	setLockState(item, base->getLockState());

	// установка флагов перетаскивания слою
	if (isLayer(item))
		item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);

	// разрешение переименования этого элемента
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	// установка развернутости
	item->setExpanded(base->isExpanded());

	foreach (BaseLayer *currentBase, base->getChildLayers())
	{
		QTreeWidgetItem *currentItem = new QTreeWidgetItem;

		// физическое создание элемента
		item->addChild(currentItem);

		createTreeItems(currentItem, currentBase);
	}

}

QString LayersTreeWidget::generateCopyName(const QString &name)
{
	// выделяем базовую часть имени
	QString baseName = name;
	int index = 0;
	QRegExp regexp;
	if ((regexp = QRegExp("(.*) копия (\\d+)")).exactMatch(name) && regexp.capturedTexts()[2].toInt() >= 2)
	{
		baseName = regexp.capturedTexts()[1];
		index = regexp.capturedTexts()[2].toInt();
	}
	else if ((regexp = QRegExp("(.*) копия")).exactMatch(name))
	{
		baseName = regexp.capturedTexts()[1];
		index = 1;
	}

	// подбираем индекс для нового имени
	QString newName = name;
	//for (int i = index; mRootLayer->findGameObjectByName(newName) != NULL; ++i)
	for (int i = index; mCurrentLocation->getRootLayer()->findLayerByName(newName) != NULL; ++i)
		newName = baseName + " копия" + (i > 1 ? " " + QString::number(i) : "");

	return newName;
}

QTreeWidgetItem *LayersTreeWidget::findItemByBaseLayer(BaseLayer *layer, QTreeWidgetItem *item) const
{
	if (item == NULL)
		item = invisibleRootItem();

	if (getBaseLayer(item) == layer)
		return item;

	for (int i = 0; i < item->childCount(); ++i)
	{
		QTreeWidgetItem *findItem = findItemByBaseLayer(layer, item->child(i));
		if (findItem != NULL)
			return findItem;
	}

	return NULL;
}

int LayersTreeWidget::getMaxDepthChilds(QTreeWidgetItem *item, int currentDepth) const
{
	int maxDepth = currentDepth;
	for (int i = 0; i < item->childCount(); ++i)
		maxDepth = qMax(maxDepth, getMaxDepthChilds(item->child(i), currentDepth + 1));
	return maxDepth;
}

int LayersTreeWidget::getLayerDepth(QTreeWidgetItem *item) const
{
	if (item == invisibleRootItem())
		return 0;

	int depth;
	for (depth = 0; item != NULL; item = item->parent(), ++depth);

	return depth;
}

bool LayersTreeWidget::hasInvisibleParent(QTreeWidgetItem *item) const
{
	for (QTreeWidgetItem *currentItem = item->parent(); currentItem != NULL; currentItem = currentItem->parent())
	{
		if ( getVisibleState(currentItem) == BaseLayer::LAYER_INVISIBLE)
			return true;
	}
	return false;
}

bool LayersTreeWidget::hasLockedParent(QTreeWidgetItem *item) const
{
	for (QTreeWidgetItem *currentItem = item->parent(); currentItem != NULL; currentItem = currentItem->parent())
	{
		// если элемент заблокирован
		if (getLockState(currentItem) == BaseLayer::LAYER_LOCKED)
			return true;
	}
	return false;
}

void LayersTreeWidget::changeChildrenPartiallyVisibleState(QTreeWidgetItem *item, bool state)
{
	// state == true - установить видимость у полувидимых
	// state == false - снять видимость у полувидимых

	for (int i = 0; i < item->childCount(); ++i)
	{
		// смена видимости текущего элемента
		if (state && getVisibleState(item->child(i)) == BaseLayer::LAYER_PARTIALLY_VISIBLE)
		{
			// 1/2 -> 1
			// установить видимость
			setVisibleState(item->child(i), BaseLayer::LAYER_VISIBLE);
			// продолжение обхода
			changeChildrenPartiallyVisibleState(item->child(i), state);
		}
		else if (!state && getVisibleState(item->child(i)) == BaseLayer::LAYER_VISIBLE)
		{
			// 1 -> 1/2
			// установить полувидимость
			setVisibleState(item->child(i), BaseLayer::LAYER_PARTIALLY_VISIBLE);
			// продолжение обхода
			changeChildrenPartiallyVisibleState(item->child(i), state);
		}
	}
}

void LayersTreeWidget::changeChildrenPartiallyLockState(QTreeWidgetItem *item, bool state)
{
	for (int i = 0; i < item->childCount(); ++i)
	{
		// смена блокировки текущего элемента
		if (state && getLockState(item->child(i)) == BaseLayer::LAYER_UNLOCKED)
		{
			// 0 -> 1/2
			// установить частичную блокировку
			setLockState(item->child(i), BaseLayer::LAYER_PARTIALLY_UNLOCKED);
			// продолжение обхода
			changeChildrenPartiallyLockState(item->child(i), state);
		}
		else if (!state && getLockState(item->child(i)) == BaseLayer::LAYER_PARTIALLY_UNLOCKED)
		{
			// 1/2 -> 0
			// снять блокировку
			setLockState(item->child(i), BaseLayer::LAYER_UNLOCKED);
			// продолжение обхода
			changeChildrenPartiallyLockState(item->child(i), state);
		}
	}
}

void LayersTreeWidget::setBaseLayer(QTreeWidgetItem *item, BaseLayer *baseLayer)
{
	QVariant v = qVariantFromValue(reinterpret_cast<quint64>(baseLayer));
	item->setData(DATA_COLUMN, Qt::UserRole, v);
}

BaseLayer *LayersTreeWidget::getBaseLayer(QTreeWidgetItem *item) const
{
	return reinterpret_cast<BaseLayer *>(item->data(DATA_COLUMN, Qt::UserRole).value<quint64>());
}

void LayersTreeWidget::setVisibleState(QTreeWidgetItem *item, BaseLayer::VisibleState state)
{
	// установка видимости для базового элемента BaseLayer
	getBaseLayer(item)->setVisibleState(state);

	// установка иконки видимости (глаз)
	item->setIcon(VISIBLE_COLUMN, mLayerVisibleIcons[state]);
}

BaseLayer::VisibleState LayersTreeWidget::getVisibleState(QTreeWidgetItem *item) const
{
	return getBaseLayer(item)->getVisibleState();
}

void LayersTreeWidget::setLockState(QTreeWidgetItem *item, BaseLayer::LockState state)
{
	// установка блокировки базового элемента BaseLayer
	getBaseLayer(item)->setLockState(state);

	// установка иконки блокировки слоя/группы (замок)
	item->setIcon(LOCK_COLUMN, mLayerLockedIcons[state]);
}

BaseLayer::LockState LayersTreeWidget::getLockState(QTreeWidgetItem *item) const
{
	return getBaseLayer(item)->getLockState();
}

bool LayersTreeWidget::isLayer(QTreeWidgetItem *item) const
{
	return (dynamic_cast<Layer *>(getBaseLayer(item)) != NULL);
}

bool LayersTreeWidget::isLayerGroup(QTreeWidgetItem *item) const
{
	return (dynamic_cast<LayerGroup *>(getBaseLayer(item)) != NULL);
}

QIcon LayersTreeWidget::createLayerIcon(BaseLayer *baseLayer)
{
	mPrimaryGLWidget->makeCurrent();

	mFrameBuffer->bind();

	// отчистка
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// устанавливаем систему координат с началом координат в левом верхнем углу
	glViewport(0, 0, WIDTH_THUMBNAIL, HEIGHT_THUMBNAIL);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, WIDTH_THUMBNAIL, HEIGHT_THUMBNAIL, 0.0);

	// настройка текстурирования
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// настройка смешивания
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// отрисовка рамки по краю
	Utils::drawRect(QRectF(0.5, 0.5, WIDTH_THUMBNAIL - 1.0, HEIGHT_THUMBNAIL - 1.0), QColor(0, 0, 0));

	// центровка с масштабированием
	QRectF rect = baseLayer->getBoundingRect();
	qreal scale = qMin((WIDTH_THUMBNAIL - 2) / rect.width(), (HEIGHT_THUMBNAIL - 2) / rect.height());
	glScaled(scale, scale, 1.0);
	// центровка
	glTranslated(-rect.left() + ((WIDTH_THUMBNAIL) / scale - rect.width()) / 2.0,
		-rect.top() + ((HEIGHT_THUMBNAIL) / scale - rect.height()) / 2.0, 0.0);

	baseLayer->draw(true);
	QImage image = mFrameBuffer->toImage();
	mFrameBuffer->release();

	return QIcon(QPixmap::fromImage(image));
}
