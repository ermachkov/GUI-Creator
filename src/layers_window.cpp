#include "pch.h"
#include "layers_window.h"

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

	connect(this, SIGNAL(layerChanged(BaseLayer *)), mLayersTreeWidget, SLOT(onEditorWindowLayerChanged(BaseLayer *)));

	// перенаправление сигналов из LayersTreeWidget
	connect(mLayersTreeWidget, SIGNAL(locationChanged()), this, SIGNAL(locationChanged()));
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
	}
	else
	{
		// отсутствует локация - отчистка
		mLayersTreeWidget->clear();
	}
}
