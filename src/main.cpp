#include "pch.h"
#include "main_window.h"

int main(int argc, char **argv)
{
	// создаем объект приложения
	QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
	QApplication app(argc, argv);

	// устанавливаем имена приложения и организации
	app.setApplicationName("gui-creator");
	app.setOrganizationName(app.applicationName());

	// устанавливаем кодировку UTF-8 по умолчанию
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

	// задаем использование ini-файлов для хранения настроек
	QSettings::setDefaultFormat(QSettings::IniFormat);

	// создаем главное окно
	MainWindow mainWindow;
	mainWindow.show();

	// запускаем главный цикл приложения
	return app.exec();
}
