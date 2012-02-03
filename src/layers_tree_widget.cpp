#include "pch.h"
#include "layers_tree_widget.h"
#include "base_layer.h"
#include "location.h"
#include "layer.h"
#include "layer_group.h"

// FIXME: в одной строчке с { комментарии не пишут. ИСПРАВИТЬ ВЕЗДЕ!!!
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
}

LayersTreeWidget::~LayersTreeWidget()
{
	mPrimaryGLWidget->makeCurrent();

	Q_ASSERT(mFrameBuffer);
	delete mFrameBuffer;
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
		Q_ASSERT(findByBaseLayer(baseCurrent) != NULL);
		setCurrentItem(findByBaseLayer(baseCurrent));
	}
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
			qDebug() << "Error: Trees are not equal";
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
	{	// текущий элемент - корень
		// добавление в корень нулевым элементом
		int index = 0;
		insertNewLayerGroup(index, invisibleRootItem());
	}
	else if (isLayerGroup(currentItem()))
	{	// текущий элемент - группа
		// добавление внутрь группы 0-вым элементом
		insertNewLayerGroup(0, currentItem());
	}
	else if (isLayer(currentItem()))
	{	// текущий элемент - слой

		if (currentItem()->parent() == NULL)
		{	// родительский элемент - корень
			// добавление в корень перед текущим элементом
			int index = currentIndex().row();
			insertNewLayerGroup(index, invisibleRootItem());
		}
		else if (isLayerGroup(currentItem()->parent()))
		{	// родительский элемент - группа
			int index = currentIndex().row();
			insertNewLayerGroup(index, currentItem()->parent());
		}
	}
	else
	{
		qDebug() << "Error in LayersWindow::onAddLayerGroup";
	}

	emit locationChanged();
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

		// добавление внутрь группы 0-вым элементом
		insertNewLayer(0, currentItem());
	}
	else if (isLayer(currentItem()))
	{
		// текущий элемент - слой

		if (currentItem()->parent() == NULL)
		{	// родительский элемент - корень
			// добавление в корень перед текущим элементом
			int index = currentIndex().row();
			insertNewLayer(index, invisibleRootItem());
		}
		else if (isLayerGroup(currentItem()->parent()))
		{	// родительский элемент - группа
			int index = currentIndex().row();
			insertNewLayer(index, currentItem()->parent());
		}
		else
		{
			qDebug() << "Error in LayersWindow::onAddLayer";
		}
	}
	else
	{
		qDebug() << "Error in LayersWindow::onAddLayer";
	}

	emit locationChanged();
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

void LayersTreeWidget::onEditorWindowLayerChanged(BaseLayer *layer)
{
	QIcon icon = createLayerIcon(layer);

	QTreeWidgetItem *item = findByBaseLayer(layer);

	if (item)
		item->setIcon(DATA_COLUMN, icon);
}

void LayersTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
	// проверяем, что перетаскивается элемент из LayersTreeWidget
	if (event->source() != this)
		return;

	QTreeWidget::dragEnterEvent(event);
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

	// получение QTreeWidgetItem * и BaseLayer * перетаскиваемого элемента
	QTreeWidgetItem *itemSelected = *listItems.begin();
	BaseLayer *baseSelected = getBaseLayer(itemSelected);

	// получение родителя QTreeWidgetItem * перетаскиваемого элемента
	QTreeWidgetItem *itemSelectedParentBefore = itemSelected->parent() ? itemSelected->parent() : invisibleRootItem();
	// получение индекса перетаскиваемого элемента через QTreeWidgetItem *
	int indexSelectedBefore = itemSelectedParentBefore->indexOfChild(itemSelected);
	// получение BaseLayer * родителя перетаскиваемого элемента
	BaseLayer *baseSelectedParentBefore = getBaseLayer(itemSelectedParentBefore);

	QTreeWidget::dropEvent(event);

	// itemSelected - теперь новый добавленный элемент
	if (currentDropAction == Qt::CopyAction)
	{
		// поиск нового добавленного элемента QTreeWidgetItem в дереве
		QList<QTreeWidgetItem *> stackItem;
		stackItem.push_back(invisibleRootItem());

		while (!stackItem.empty())
		{
			QTreeWidgetItem *currentItem = stackItem.last();
			stackItem.pop_back();

			// BaseLayer * совпадает, но сам QTreeWidgetItem * не совпадает
			if (getBaseLayer(currentItem) == baseSelected && currentItem != itemSelected)
			{
				// переназначение QTreeWidgetItem * добавленного элемента
				itemSelected = currentItem;
				// обнуление BaseLayer у нового элемента
				setBaseLayer(itemSelected, NULL);

				//				//и BaseLayer *
//				//baseSelected = getBaseLayer(itemSelected);
				break;
			}

			for (int i = 0; i < currentItem->childCount(); ++i)
				stackItem.push_back(currentItem->child(i));
		}
	}

	// получение нового местоположения элемента
	QTreeWidgetItem *itemSelectedParentAfter = itemSelected->parent() ? itemSelected->parent() : invisibleRootItem();
	int indexSelectedAfter = itemSelectedParentAfter->indexOfChild(itemSelected);
	BaseLayer *baseSelectedParentAfter = getBaseLayer(itemSelectedParentAfter);


	// синхронизация дерева BaseLayer с деревом Item
	if (currentDropAction == Qt::MoveAction)
	{
		// отцепление от старого родителя
		baseSelectedParentBefore->removeChildLayer(indexSelectedBefore);

		// добавление
		baseSelectedParentAfter->insertChildLayer(indexSelectedAfter, baseSelected);
	}
	else if (currentDropAction == Qt::CopyAction)
	{
		// дублирование дерева BaseLayer с прикреплением к новому родителю
		BaseLayer *baseAdded = baseSelected->duplicate(baseSelectedParentAfter, indexSelectedAfter);

		// модификация имени нового слоя
		// oldName - сначала исходное имя, затем базовое
		QString oldName = itemSelected->text(DATA_COLUMN);
		// индекс копии
		int indexAfterCopy = 0;

		QStringList splitName = oldName.split(" ");
		int index = splitName.lastIndexOf("Copy");
		if (index == splitName.size() - 1)
		{
			// последнее вхождение
			indexAfterCopy = 1;
			splitName.removeLast();
			oldName = splitName.join(" ");
		}
		else if (index == splitName.size() - 2)
		{
			// предпоследнее вхождение
			bool isOk;
			indexAfterCopy = splitName[splitName.size() - 1].toInt(&isOk);
			if (isOk && indexAfterCopy >= 2)
			{
				// справа валидное unsigned int число
				splitName.removeLast();
				splitName.removeLast();
				oldName = splitName.join(" ");
			}
		}

		// формирование нового имени
		QString newName;
		do
		{
			if (indexAfterCopy == 0)
				newName = oldName + " Copy";
			else
				newName = oldName + " Copy " + QString::number(indexAfterCopy + 1);

			++indexAfterCopy;
		}
		while (mCurrentLocation->getRootLayer()->findLayerByName(newName));

		itemSelected->setText(DATA_COLUMN, newName);
		baseAdded->setName(newName);

		// ручное досоздание дерева
		createTreeItems(itemSelected, baseAdded);
	}

	emit locationChanged();

	// отладочное сравнение деревьев
	DEBUG_TREES();
}

void LayersTreeWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
		onDelete();

	QTreeWidget::keyPressEvent(event);
}

void LayersTreeWidget::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    // FIXME: Мммм.. А че мешало написать getBaseLayer так, чтобы он
    // возвращал NULL, если передано NULL?
	// if (currentLocation != NULL)
	//     currentLocation->setActiveLayer(getBaseLayer(current));
	// 2 строчки вместо 12

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
	if (isLayerGroup(item))
		icon = mLayerGroupIcon;
	else
		icon = base->getThumbnail().isNull() ? createLayerIcon(base) : base->getThumbnail();

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

QTreeWidgetItem *LayersTreeWidget::findByBaseLayer(BaseLayer *layer)
{
	// FIXME: я так понял, рекурсию ты ниасилил и штампуешь говнокод в промышленных масштабах :-P
	// рекурсивная реализация этой функции занимает раза в два меньше места
	// обход всего дерева
	QList<QTreeWidgetItem *> stack;
	stack.push_back(invisibleRootItem());
	while (!stack.empty())
	{
		QTreeWidgetItem *currentItem = stack.last();
		// удаление последнего элемента стека
		stack.pop_back();

		if (getBaseLayer(currentItem) == layer)

		{	// указатель совпадает
			return currentItem;
		}

		// добавлеине в стек всех потомков текущего элемента
		for (int i = 0; i < currentItem->childCount(); ++i)
			stack.push_back(currentItem->child(i));
			//stack += currentItem->child(i);
	}

	qDebug() << "Warning! findByBaseLayer return NULL";
	return NULL;

	// элемент не найден - возврат корневого элемента
	//return invisibleRootItem();
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

	QRectF rect = baseLayer->getBoundingRect();

	float scale = qMin(WIDTH_THUMBNAIL / rect.width(), HEIGHT_THUMBNAIL / rect.height());
	// центровка с масштабированием
	glScalef(scale, scale, 1.0f);
	glTranslatef(-rect.left() + (WIDTH_THUMBNAIL / scale - rect.width()) / 2.0f,
		-rect.top() + (HEIGHT_THUMBNAIL / scale - rect.height()) / 2.0f, 0.0f);

	baseLayer->draw();
	QImage image = mFrameBuffer->toImage();
	mFrameBuffer->release();

	return QIcon(QPixmap::fromImage(image));
}
