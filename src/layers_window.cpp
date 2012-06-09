#include "pch.h"
#include "layers_window.h"
#include "scene.h"

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

	connect(this, SIGNAL(layerChanged(Scene *, BaseLayer *)), mLayersTreeWidget, SLOT(onEditorWindowLayerChanged(Scene *, BaseLayer *)));
	connect(this, SIGNAL(layerChanged(BaseLayer *)), mLayersTreeWidget, SLOT(onPropertyWindowLayerChanged(BaseLayer *)));

	// перенаправление сигналов из LayersTreeWidget
	connect(mLayersTreeWidget, SIGNAL(sceneChanged(const QString &)), this, SLOT(onLayersTreeWidgetSceneChanged(const QString &)));
	connect(mLayersTreeWidget, SIGNAL(layerChanged()), this, SIGNAL(layerChanged()));

	mLayersTreeWidget->setPrimaryGLWidget(primaryGLWidget);
}

void LayersWindow::setCurrentScene(Scene *scene)
{
	bool enabled = scene != NULL;

	// сцена есть - разблокировка GUI
	// сцены нет - блокировка GUI
	mLayersTreeWidget->setEnabled(enabled);
	mAddLayerGroupPushButton->setEnabled(enabled);
	mAddLayerPushButton->setEnabled(enabled);
	mDeletePushButton->setEnabled(enabled);

	if (enabled)
	{
		// перенаправление обработки в метод виджета
		mLayersTreeWidget->setCurrentScene(scene);

		// блокировка кнопки удаления если один слой или одна активная папка
		setDeleteButtonState();
	}
	else
	{
		// отсутствует сцена - очистка
		mLayersTreeWidget->clear();
	}
}

void LayersWindow::onLayersTreeWidgetSceneChanged(const QString &commandName)
{
	// блокировка кнопки удаления если один слой или одна активная папка
	setDeleteButtonState();

	emit sceneChanged(commandName);
}

void LayersWindow::setDeleteButtonState()
{
	Scene *scene = mLayersTreeWidget->getCurrentScene();

	bool enableDelete = scene->getRootLayer()->getNumChildLayers() != 1
		|| scene->getActiveLayer() != scene->getRootLayer()->getChildLayer(0);

	// активация/деактивация кнопки удаления
	mDeletePushButton->setEnabled(enableDelete);
}
