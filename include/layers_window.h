#ifndef LAYERS_WINDOW_H
#define LAYERS_WINDOW_H

#include "ui_layers_window.h"

class Location;
class BaseLayer;

// Класс окна слоев
class LayersWindow : public QDockWidget, private Ui::LayersWindow
{
	Q_OBJECT

public:

	// Конструктор
	LayersWindow(QGLWidget *primaryGLWidget, QWidget *parent = NULL);

	// Устанавливает текущую локацию
	void setCurrentLocation(Location *location);

signals:

	// Сигнал об изменении локации
	void locationChanged();

	// Сигнал об изменении слоя
	void layerChanged();

	// Сигнал об изменении слоя в окне редактирования
	void layerChanged(BaseLayer *);
};

#endif // LAYERS_WINDOW_H
