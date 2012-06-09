#ifndef LAYERS_WINDOW_H
#define LAYERS_WINDOW_H

#include "ui_layers_window.h"

class BaseLayer;
class Scene;

// Класс окна слоев
class LayersWindow : public QDockWidget, private Ui::LayersWindow
{
	Q_OBJECT

public:

	// Конструктор
	LayersWindow(QGLWidget *primaryGLWidget, QWidget *parent = NULL);

	// Устанавливает текущую сцену
	void setCurrentScene(Scene *scene);

signals:

	// Сигнал об изменении сцены
	void sceneChanged(const QString &commandName);

	// Сигнал об изменении слоя в окне слоев
	void layerChanged();

	// Сигнал об изменении слоя в окне редактирования
	void layerChanged(Scene *scene, BaseLayer *layer);

	// Сигнал об изменении слоя в окне свойств
	void layerChanged(BaseLayer *layer);

private slots:

	void onLayersTreeWidgetSceneChanged(const QString &commandName);

private:

	void setDeleteButtonState();
};

#endif // LAYERS_WINDOW_H
