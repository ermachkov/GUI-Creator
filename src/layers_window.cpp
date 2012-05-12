#include "pch.h"
#include "layers_window.h"
#include "location.h"

LayersWindow::LayersWindow(QGLWidget *primaryGLWidget, QWidget *parent)
: QDockWidget(parent)
{
	// создание и настройка дочерних элементов окна
	setupUi(this);

	// настройка порядка вывода столбцов в окне слоев
	mLayersTreeWidget->header()->moveSection(LayersTreeWidget::DATA_COLUMN, LayersTreeWidget::LOCK_COLUMN);
	mLayersTreeWidget->header()->setStretchLastSection(true);

	// перенаправление сигналов в LayersTreeWidget
	connect(mAddLayerGroupPushButton, SIGNAL(clicked()), mLayersTreeWidget, SLOT(onAddLayerGroup()));
	connect(mAddLayerPushButton, SIGNAL(clicked()), mLayersTreeWidget, SLOT(onAddLayer()));
	connect(mDeletePushButton, SIGNAL(clicked()), mLayersTreeWidget, SLOT(onDelete()));

	connect(this, SIGNAL(layerChanged(Location *, BaseLayer *)), mLayersTreeWidget, SLOT(onEditorWindowLayerChanged(Location *, BaseLayer *)));

	// перенаправление сигналов из LayersTreeWidget
	connect(mLayersTreeWidget, SIGNAL(locationChanged(const QString &)), this, SLOT(onLayersTreeWidgetLocationChanged(const QString &)));
	connect(mLayersTreeWidget, SIGNAL(layerChanged()), this, SIGNAL(layerChanged()));

	mLayersTreeWidget->setPrimaryGLWidget(primaryGLWidget);
}

void LayersWindow::setCurrentLocation(Location *location)
{
	bool enabled = location != NULL;

	// локации есть - разблокировка GUI
	// локации нет - блокировка GUI
	mLayersTreeWidget->setEnabled(enabled);
	mAddLayerGroupPushButton->setEnabled(enabled);
	mAddLayerPushButton->setEnabled(enabled);
	mDeletePushButton->setEnabled(enabled);

	if (enabled)
	{
		// перенаправление обработки в метод виджета
		mLayersTreeWidget->setCurrentLocation(location);

		// блокировка кнопки удаления если один слой или одна активная папка
		setDeleteButtonState();
	}
	else
	{
		// отсутствует локация - очистка
		mLayersTreeWidget->clear();
	}
}

void LayersWindow::onLayersTreeWidgetLocationChanged(const QString &commandName)
{
	// блокировка кнопки удаления если один слой или одна активная папка
	setDeleteButtonState();

	emit locationChanged(commandName);
}

void LayersWindow::setDeleteButtonState()
{
	Location *location = mLayersTreeWidget->getCurrentLocation();

	bool enableDelete = location->getRootLayer()->getNumChildLayers() != 1
		|| location->getActiveLayer() != location->getRootLayer()->getChildLayer(0);

	// активация/деактивация кнопки удаления
	mDeletePushButton->setEnabled(enableDelete);
}
