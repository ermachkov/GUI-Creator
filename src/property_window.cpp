#include "property_window.h"
#include "game_object.h"
#include "label.h"
#include "sprite.h"
#include <typeinfo>

PropertyWindow::PropertyWindow(QWidget *parent)
: QDockWidget(parent)
{
	setupUi(this);

	//this->scrollArea->adjustSize();

	//setSizeConstraint(QLayout::SetMinimumSize);
	//dockWidgetContents->setSizeConstraint(QLayout::SetMinimumSize);
	//layout->setSizeConstraint(QLayout::SetMinimumSize);
	//scrollArea->adjustSize();

	//stackedWidget->adjustSize();

	QSize s;

	page->adjustSize();
	s = page->size();
	page->setMinimumSize(s);

	page_2->adjustSize();
	s = page_2->size();
	page_2->setMinimumSize(s);

	stackedWidget->setCurrentIndex(1);
	stackedWidget->setCurrentIndex(0);
	stackedWidget->setCurrentIndex(1);

	//scrollAreaWidgetContents->adjustSize();
}

void PropertyWindow::onEditorWindowSelectionChanged(const QList<GameObject *> &objects)
{
	//qDebug() << objects;

	// нет выделенных объектов
	if (objects.empty())
	{
		// FIXME:
		// ...

		return;
	}

	foreach (GameObject *object, objects)
		qDebug() << typeid(object).name();

	// определение идентичности типа выделенных объектов
	QStringList types;
	foreach (GameObject *object, objects)
		types.push_back(typeid(object).name());
	if (types.count(types.front()) != types.size())
	{
		;
	}
}
