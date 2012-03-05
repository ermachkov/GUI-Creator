#ifndef PROPERTY_WINDOW_H
#define PROPERTY_WINDOW_H

#include "ui_property_window.h"

class GameObject;

class PropertyWindow : public QDockWidget, private Ui::PropertyWindow
{
	Q_OBJECT
	
public:

	explicit PropertyWindow(QWidget *parent = 0);
	
public slots:

	void onEditorWindowSelectionChanged(const QList<GameObject *> &objects);
};

#endif // PROPERTY_WINDOW_H
