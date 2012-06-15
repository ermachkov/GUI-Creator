#include "pch.h"
#include "main_window.h"
#include "editor_window.h"
#include "font_browser.h"
#include "font_manager.h"
#include "history_window.h"
#include "layers_window.h"
#include "options.h"
#include "options_dialog.h"
#include "project.h"
#include "property_window.h"
#include "sprite_browser.h"
#include "texture_manager.h"
#include "utils.h"

MainWindow::MainWindow()
: mRenderEnabled(true), mUntitledIndex(1), mTabWidgetCurrentIndex(-1), mTranslationCounter(0)
{
	setupUi(this);

	// создаем невидимый OpenGL виджет для загрузки текстур в основном потоке
	QGLFormat format;
	format.setSwapInterval(0);
	mPrimaryGLWidget = new QGLWidget(format, this);
	mPrimaryGLWidget->setVisible(false);

	// создаем вспомогательный OpenGL виджет для загрузки текстур в фоновом потоке и расшариваем его с основным
	mSecondaryGLWidget = new QGLWidget(format, this, mPrimaryGLWidget);
	mSecondaryGLWidget->setVisible(false);

	// создаем синглетоны
	QSettings settings;
	new Options(settings);
	new Project();
	new FontManager(mPrimaryGLWidget);
	new TextureManager(mPrimaryGLWidget, mSecondaryGLWidget);

	// открываем проект
	QStringList arguments = QCoreApplication::arguments();
	if (arguments.size() > 1)
		if (!Project::getSingleton().open(arguments[1]))
			QMessageBox::critical(this, "", "Ошибка открытия файла проекта " + arguments[1]);

	// создаем браузер спрайтов
	mSpriteBrowser = new SpriteBrowser(this);
	addDockWidget(Qt::RightDockWidgetArea, mSpriteBrowser);

	// создаем браузер шрифтов
	mFontBrowser = new FontBrowser(this);
	addDockWidget(Qt::RightDockWidgetArea, mFontBrowser);

	// создаем окно свойств объекта
	mPropertyWindow = new PropertyWindow(this);
	addDockWidget(Qt::RightDockWidgetArea, mPropertyWindow);

	// создаем окно слоев
	mLayersWindow = new LayersWindow(mPrimaryGLWidget, this);
	addDockWidget(Qt::RightDockWidgetArea, mLayersWindow);

	// создаем окно истории
	mHistoryWindow = new HistoryWindow(this);
	addDockWidget(Qt::RightDockWidgetArea, mHistoryWindow);

	// наложение плавающих окон друг на друга
	tabifyDockWidget(mSpriteBrowser, mFontBrowser);
	tabifyDockWidget(mFontBrowser, mPropertyWindow);
	tabifyDockWidget(mLayersWindow, mHistoryWindow);

	// установка нулевого индекса для всех таббаров
	QList<QTabBar *> tabBars = findChildren<QTabBar *>();
	foreach (QTabBar *tabBar, tabBars)
		if (tabBar->count() != 0)
			tabBar->setCurrentIndex(0);

	// разворачивание окна редактора на весь экран
	setWindowState(Qt::WindowMaximized);

	// обновляем пункты главного меню
	updateMainMenuActions();

	// добавляем в меню Файл пункты для последних файлов
	for (int i = 0; i < 5; ++i)
	{
		QAction *action = new QAction(this);
		connect(action, SIGNAL(triggered()), this, SLOT(on_mOpenAction_triggered()));
		mRecentFilesActions.push_back(action);
	}

	mFileMenu->insertActions(mExitAction, mRecentFilesActions);
	mRecentFilesSeparator = mFileMenu->insertSeparator(mExitAction);
	updateRecentFilesActions("");

	// настраиваем акселераторы по Alt для пунктов меню плавающих окон
	mSpriteBrowser->toggleViewAction()->setText("Спр&айты");
	mFontBrowser->toggleViewAction()->setText("&Шрифты");
	mPropertyWindow->toggleViewAction()->setText("Сво&йства");
	mLayersWindow->toggleViewAction()->setText("С&лои");
	mHistoryWindow->toggleViewAction()->setText("&История");

	// добавляем в меню Вид пункты для плавающих окон
	QAction *zoomAction = mZoomMenu->menuAction();
	mViewMenu->insertAction(zoomAction, mSpriteBrowser->toggleViewAction());
	mViewMenu->insertAction(zoomAction, mFontBrowser->toggleViewAction());
	mViewMenu->insertAction(zoomAction, mPropertyWindow->toggleViewAction());
	mViewMenu->insertAction(zoomAction, mLayersWindow->toggleViewAction());
	mViewMenu->insertAction(zoomAction, mHistoryWindow->toggleViewAction());
	mViewMenu->insertSeparator(zoomAction);

	// добавляем в меню Язык все доступные языки
	QStringList languages = Project::getSingleton().getLanguages();
	QStringList languageNames = Project::getSingleton().getLanguageNames();
	QActionGroup *languagesActionGroup = new QActionGroup(this);
	QSignalMapper *languagesSignalMapper = new QSignalMapper(this);
	for (int i = 0; i < languages.size(); ++i)
	{
		// создаем пункт меню
		QAction *action = new QAction(languageNames[i], this);
		action->setCheckable(true);
		mLocalizationMenu->addAction(action);
		languagesActionGroup->addAction(action);

		// устанавливаем галочку для пункта меню с текущим языком
		if (languages[i] == Project::getSingleton().getCurrentLanguage())
			action->setChecked(true);

		// добавляем в маппер сигнал от пункта меню
		languagesSignalMapper->setMapping(action, languages[i]);
		connect(action, SIGNAL(triggered()), languagesSignalMapper, SLOT(map()));
	}

	// связываем сигнал об изменении текущего языка
	connect(languagesSignalMapper, SIGNAL(mapped(const QString &)), this, SLOT(onLanguageChanged(const QString &)));

	// создаем ватчер для слежения за файлами переводов
	mTranslationFilesWatcher = new QFileSystemWatcher(this);
	connect(mTranslationFilesWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(onTranslationFileChanged(const QString &)));

	// создаем текстовое поле для координат мыши
	mMousePosLabel = new QLabel(this);
	mStatusBar->addWidget(mMousePosLabel);

	// создаем выпадающий список масштабов
	mZoomComboBox = new QComboBox(this);
	mZoomComboBox->setEditable(true);
	mZoomComboBox->setInsertPolicy(QComboBox::NoInsert);
	mViewToolBar->insertWidget(mZoomInAction, mZoomComboBox);

	// устанавливаем валидатор для ввода только цифр и процента
	PercentIntValidator *validator = new PercentIntValidator(10, 3200, this);
	mZoomComboBox->setValidator(validator);

	// для группировки пунктов в меню масштаба
	QActionGroup *zoomActionGroup = new QActionGroup(this);

	// для перенаправления сигналов от нескольких пунктов меню масштаба на один слот
	QSignalMapper *zoomSignalMapper = new QSignalMapper(this);

	// создаем список масштабов
	const qreal ZOOM_LIST[] = {0.1, 0.25, 0.5, 0.75, 1.0, 2.0, 5.0, 10.0, 20.0, 32.0};
	for (size_t i = 0; i < sizeof(ZOOM_LIST) / sizeof(qreal); ++i)
		mZoomList.push_back(ZOOM_LIST[i]);

	// заполняем комбобокс масштабов и меню Вид-Масштаб
	foreach (qreal zoom, mZoomList)
	{
		// формирование строки процентов
		QString zoomStr = QString::number(qRound(zoom * 100.0)) + '%';

		// заполнение комбобокса значениями масштабов
		mZoomComboBox->addItem(zoomStr);

		// создание пункта меню
		QAction *action = new QAction(zoomStr, this);
		action->setCheckable(true);

		// назначение горячей клавиши
		if (zoom == 1.0)
			action->setShortcut(QString("Ctrl+1"));
		else if (zoom == 2.0)
			action->setShortcut(QString("Ctrl+2"));

		// добавление пункта в меню масштаба
		mZoomMenu->addAction(action);

		// группировка пунктов в меню масштаба
		zoomActionGroup->addAction(action);

		// установка активной галочки для меню
		if (zoom == 1.0)
			action->setChecked(true);

		// добавляем в маппер сигнал от пункта меню
		zoomSignalMapper->setMapping(action, zoomStr);
		connect(action, SIGNAL(triggered()), zoomSignalMapper, SLOT(map()));
	}

	// связываем сигналы изменения масштаба от пунктов меню и комбобокса
	connect(zoomSignalMapper, SIGNAL(mapped(const QString &)), this, SLOT(onZoomChanged(const QString &)));
	connect(mZoomComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(onZoomChanged(const QString &)));
	connect(mZoomComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onZoomEditingFinished()));

	// связываем сигналы об изменениях в окне слоев
	connect(mLayersWindow, SIGNAL(sceneChanged(const QString &)), this, SLOT(onSceneChanged(const QString &)));
	connect(mLayersWindow, SIGNAL(layerChanged()), this, SLOT(onLayerWindowLayerChanged()));

	// связываем сигналы об изменениях в окне свойств
	connect(mPropertyWindow, SIGNAL(sceneChanged(const QString &)), this, SLOT(onSceneChanged(const QString &)));
	connect(mPropertyWindow, SIGNAL(objectsChanged(const QPointF &)), this, SLOT(onPropertyWindowObjectsChanged(const QPointF &)));
	connect(mPropertyWindow, SIGNAL(allowedEditorActionsChanged()), this, SLOT(onPropertyWindowAllowedEditorActionsChanged()));
	connect(mPropertyWindow, SIGNAL(layerChanged(BaseLayer *)), mLayersWindow, SIGNAL(layerChanged(BaseLayer *)));

	// связываем сигнал об изменении текстуры
	connect(TextureManager::getSingletonPtr(), SIGNAL(textureChanged(const QString &, const QSharedPointer<Texture> &)),
		this, SLOT(onTextureChanged(const QString &, const QSharedPointer<Texture> &)));

	// связываем сигнал об изменении содержимого буфера обмена
	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(onClipboardDataChanged()));

	// загружаем состояние главного окна и панелей
	settings.beginGroup("Window");
	restoreGeometry(settings.value("Geometry").toByteArray());
	restoreState(settings.value("State").toByteArray());
	settings.endGroup();

	// создаем новый документ
	on_mNewAction_triggered();

	// запускаем таймер на ~60 Гц для рендеринга
	startTimer(16);
}

MainWindow::~MainWindow()
{
	// удаляем объекты редактора
	delete mSpriteBrowser;
	delete mFontBrowser;
	delete mPropertyWindow;
	delete mLayersWindow;
	delete mHistoryWindow;

	// удаляем синглетоны в последнюю очередь
	TextureManager::destroy();
	FontManager::destroy();
	Project::destroy();
	Options::destroy();
}

EditorWindow *MainWindow::getCurrentEditorWindow() const
{
	return getEditorWindow(mTabWidgetCurrentIndex);
}

EditorWindow *MainWindow::getEditorWindow(int index) const
{
	return static_cast<EditorWindow *>(mTabWidget->widget(index));
}

bool MainWindow::event(QEvent *event)
{
	// разрешаем/запрещаем отрисовку по таймеру при блокировке/разблокировке главного окна модальными диалогами
	if (event->type() == QEvent::WindowBlocked)
		mRenderEnabled = false;
	else if (event->type() == QEvent::WindowUnblocked)
		mRenderEnabled = true;

	return QMainWindow::event(event);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
	// перерисовываем текущую открытую вкладку
	if (mRenderEnabled && mTabWidget->currentWidget() != NULL)
		mTabWidget->currentWidget()->update();

	// обновляем состояния файлов переводов
	if (++mTranslationCounter >= 15)
	{
		mTranslationCounter = 0;
		for (TranslationFilesMap::iterator it = mTranslationFilesMap.begin(); it != mTranslationFilesMap.end(); ++it)
			if (it->mChanged)
			{
				// если с момента последнего изменения файла переводов прошло достаточно много времени, загружаем его
				if (it->mTimer.hasExpired(250))
				{
					it->mChanged = false;
					it->mEditorWindow->loadTranslationFile(it.key());
					if (!mTranslationFilesWatcher->files().contains(it.key()) && Utils::fileExists(it.key()))
						mTranslationFilesWatcher->addPath(it.key());
				}
			}
			else if (!mTranslationFilesWatcher->files().contains(it.key()))
			{
				// если файл переводов был удален, а потом восстановлен, загружаем его
				if (Utils::fileExists(it.key()))
				{
					it->mEditorWindow->loadTranslationFile(it.key());
					mTranslationFilesWatcher->addPath(it.key());
				}
			}
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// закрываем все открытые вкладки и сохраняем текущий открытый проект и настройки приложения перед выходом
	if (on_mCloseAllAction_triggered())
	{
		// сохраняем текущий открытый проект
		Project::getSingleton().save();

		// сохраняем настройки приложения
		QSettings settings;
		Options::getSingleton().save(settings);

		// сохраняем состояние главного окна и панелей
		settings.beginGroup("Window");
		settings.setValue("Geometry", saveGeometry());
		settings.setValue("State", saveState());
		settings.endGroup();
	}
	else
	{
		event->ignore();
	}
}

void MainWindow::on_mNewAction_triggered()
{
	// создаем новую вкладку с окном редактора и переключаемся на нее
	QString fileName = mUntitledIndex == 1 ? "untitled.lua" : QString("untitled_%1.lua").arg(mUntitledIndex);
	++mUntitledIndex;
	EditorWindow *editorWindow = createEditorWindow(fileName);
	mTabWidget->addTab(editorWindow, fileName);
	mTabWidget->setCurrentWidget(editorWindow);
	onClipboardDataChanged();
}

void MainWindow::on_mOpenAction_triggered()
{
	// показываем диалоговое окно открытия файла
	QString fileName = qobject_cast<QAction *>(sender())->data().toString();
	QString filter = "Файлы " + QCoreApplication::applicationName() + " (*.lua);;Все файлы (*)";
	if (fileName.isEmpty())
		fileName = QFileDialog::getOpenFileName(this, "Открыть", Options::getSingleton().getLastOpenedDirectory(), filter);
	if (!Utils::isFileNameValid(fileName, Project::getSingleton().getRootDirectory() + Project::getSingleton().getScenesDirectory(), this))
		return;

	// сохраняем каталог с выбранным файлом
	Options::getSingleton().setLastOpenedDirectory(QFileInfo(fileName).path());

	// проверяем, не открыт ли уже этот файл
	for (int i = 0; i < mTabWidget->count(); ++i)
		if (getEditorWindow(i)->getFileName() == fileName)
		{
			mTabWidget->setCurrentIndex(i);
			updateRecentFilesActions(fileName);
			return;
		}

	// создаем новую вкладку с окном редактора
	EditorWindow *editorWindow = createEditorWindow(fileName);

	// открываем файл
	if (!editorWindow->load(fileName))
	{
		delete editorWindow;
		on_mTabWidget_currentChanged(mTabWidget->currentIndex());
		QMessageBox::critical(this, "", "Ошибка открытия файла " + fileName);
		return;
	}

	// загружаем файл переводов
	QString translationFileName = getTranslationFileName(fileName);
	editorWindow->loadTranslationFile(translationFileName);

	// добавляем файл переводов на слежение
	mTranslationFilesMap.insert(translationFileName, TranslationFileInfo(editorWindow));
	if (!mTranslationFilesWatcher->files().contains(translationFileName) && Utils::fileExists(translationFileName))
		mTranslationFilesWatcher->addPath(translationFileName);

	// создаем новую вкладку и переключаемся на нее
	mTabWidget->addTab(editorWindow, QFileInfo(fileName).fileName());
	mTabWidget->setCurrentWidget(editorWindow);
	onClipboardDataChanged();

	// обновляем список последних файлов
	updateRecentFilesActions(fileName);

	// проверяем сцену на наличие отсутствующих файлов
	checkMissedFiles();
}

bool MainWindow::on_mSaveAction_triggered()
{
	// если файл не был ни разу сохранен, показываем диалоговое окно сохранения файла
	EditorWindow *editorWindow = getCurrentEditorWindow();
	if (editorWindow->isUntitled())
		return on_mSaveAsAction_triggered();

	// сохраняем файл
	if (!editorWindow->save(editorWindow->getFileName()))
	{
		QMessageBox::critical(this, "", "Ошибка сохранения файла " + editorWindow->getFileName());
		return false;
	}

	// обновляем звездочку в имени вкладки
	updateUndoRedoActions();
	return true;
}

bool MainWindow::on_mSaveAsAction_triggered()
{
	// показываем диалоговое окно сохранения файла
	EditorWindow *editorWindow = getCurrentEditorWindow();
	QString dir = editorWindow->isUntitled() ? Options::getSingleton().getLastOpenedDirectory() + editorWindow->getFileName() : editorWindow->getFileName();
	QString filter = "Файлы " + QCoreApplication::applicationName() + " (*.lua);;Все файлы (*)";
	QString fileName = QFileDialog::getSaveFileName(this, "Сохранить как", dir, filter);
	if (!Utils::isFileNameValid(fileName, Project::getSingleton().getRootDirectory() + Project::getSingleton().getScenesDirectory(), this))
		return false;

	// сохраняем каталог с выбранным файлом
	Options::getSingleton().setLastOpenedDirectory(QFileInfo(fileName).path());

	// сохраняем файл
	if (!editorWindow->save(fileName))
	{
		QMessageBox::critical(this, "", "Ошибка сохранения файла " + fileName);
		return false;
	}

	// обновляем список последних файлов и звездочку в имени вкладки
	updateRecentFilesActions(fileName);
	updateUndoRedoActions();
	return true;
}

bool MainWindow::on_mCloseAction_triggered()
{
	// закрываем текущую вкладку
	return on_mTabWidget_tabCloseRequested(mTabWidget->currentIndex());
}

bool MainWindow::on_mCloseAllAction_triggered()
{
	// закрываем все открытые вкладки от первой к последней
	while (mTabWidget->count() != 0)
	{
		mTabWidget->setCurrentIndex(0);
		if (!on_mTabWidget_tabCloseRequested(mTabWidget->currentIndex()))
			return false;
	}

	return true;
}

void MainWindow::on_mUndoAction_triggered()
{
	mPropertyWindow->clearChildWidgetFocus();
	getCurrentEditorWindow()->undo();
}

void MainWindow::on_mRedoAction_triggered()
{
	mPropertyWindow->clearChildWidgetFocus();
	getCurrentEditorWindow()->redo();
}

void MainWindow::on_mCutAction_triggered()
{
	mPropertyWindow->clearChildWidgetFocus();
	getCurrentEditorWindow()->cut();
}

void MainWindow::on_mCopyAction_triggered()
{
	getCurrentEditorWindow()->copy();
}

void MainWindow::on_mPasteAction_triggered()
{
	getCurrentEditorWindow()->paste();
}

void MainWindow::on_mDeleteAction_triggered()
{
	mPropertyWindow->clearChildWidgetFocus();
	getCurrentEditorWindow()->clear();
}

void MainWindow::on_mOptionsAction_triggered()
{
	// вызываем диалог настроек приложения
	OptionsDialog dialog(this);
	dialog.exec();

	// обновляем пункты главного меню
	updateMainMenuActions();
}

void MainWindow::on_mSelectAllAction_triggered()
{
	getCurrentEditorWindow()->selectAll();
}

void MainWindow::on_mZoomInAction_triggered()
{
	// ищем ближайший следующий масштаб в списке
	qreal zoom = getCurrentEditorWindow()->getZoom();
	for (int i = 0; i < mZoomList.size(); ++i)
		if (zoom < mZoomList[i])
		{
			zoom = mZoomList[i];
			break;
		}

	// устанавливаем новый масштаб
	QString zoomStr = QString::number(qRound(zoom * 100.0)) + '%';
	onZoomChanged(zoomStr);
}

void MainWindow::on_mZoomOutAction_triggered()
{
	// ищем ближайший предыдущий масштаб в списке
	qreal zoom = getCurrentEditorWindow()->getZoom();
	for (int i = mZoomList.size() - 1; i >= 0; --i)
		if (zoom > mZoomList[i])
		{
			zoom = mZoomList[i];
			break;
		}

	// устанавливаем новый масштаб
	QString zoomStr = QString::number(qRound(zoom * 100.0)) + '%';
	onZoomChanged(zoomStr);
}

void MainWindow::on_mShowGridAction_triggered(bool checked)
{
	Options::getSingleton().setShowGrid(checked);
}

void MainWindow::on_mSnapToGridAction_triggered(bool checked)
{
	Options::getSingleton().setSnapToGrid(checked);
}

void MainWindow::on_mShowGuidesAction_triggered(bool checked)
{
	Options::getSingleton().setShowGuides(checked);
}

void MainWindow::on_mSnapToGuidesAction_triggered(bool checked)
{
	Options::getSingleton().setSnapToGuides(checked);
}

void MainWindow::on_mEnableSmartGuidesAction_triggered(bool checked)
{
	Options::getSingleton().setEnableSmartGuides(checked);
}

void MainWindow::on_mBringToFrontAction_triggered()
{
	getCurrentEditorWindow()->bringToFront();
}

void MainWindow::on_mSendToBackAction_triggered()
{
	getCurrentEditorWindow()->sendToBack();
}

void MainWindow::on_mMoveUpAction_triggered()
{
	getCurrentEditorWindow()->moveUp();
}

void MainWindow::on_mMoveDownAction_triggered()
{
	getCurrentEditorWindow()->moveDown();
}

bool MainWindow::on_mTabWidget_tabCloseRequested(int index)
{
	// сброс фокуса в окне свойств
	mPropertyWindow->clearChildWidgetFocus();

	// запрашиваем подтверждение на сохранение, если файл был изменен
	EditorWindow *editorWindow = getEditorWindow(index);
	bool close = true;
	if (!editorWindow->isClean())
	{
		QString text = QString("Сохранить изменения в файле %1?").arg(QFileInfo(editorWindow->getFileName()).fileName());
		QMessageBox::StandardButton result = QMessageBox::warning(this, "", text, QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (result == QMessageBox::Save)
			close = on_mSaveAction_triggered();
		else if (result == QMessageBox::Cancel)
			close = false;
	}

	// удаляем вкладку вместе с окном редактора
	if (close)
	{
		// сбрасываем текущую сцену
		if (index == mTabWidget->currentIndex())
		{
			mLayersWindow->setCurrentScene(NULL, "");
			mPropertyWindow->onEditorWindowSelectionChanged(QList<GameObject *>(), QPointF());
		}

		// удаляем файл переводов из списка слежения
		if (!editorWindow->isUntitled())
		{
			QString translationFileName = getTranslationFileName(editorWindow->getFileName());
			mTranslationFilesMap.remove(translationFileName);
			if (mTranslationFilesWatcher->files().contains(translationFileName))
				mTranslationFilesWatcher->removePath(translationFileName);
		}

		// удаляем вкладку и окно редактора
		mTabWidget->removeTab(index);
		delete editorWindow;
	}

	return close;
}

void MainWindow::on_mTabWidget_currentChanged(int index)
{
	// сброс фокуса в окне свойств
	mPropertyWindow->clearChildWidgetFocus();
	mTabWidgetCurrentIndex = index;

	if (index != -1)
	{
		// вызываем обработчик изменения масштаба
		EditorWindow *editorWindow = getCurrentEditorWindow();
		QString zoomStr = QString::number(qRound(editorWindow->getZoom() * 100.0)) + '%';
		onZoomChanged(zoomStr);

		// обновляем пункты меню, связанные с редактированием выделенных объектов
		onEditorWindowSelectionChanged(editorWindow->getSelectedObjects(), editorWindow->getRotationCenter());

		// обновляем пункты меню, связанные с отменой/повтором операций, и звездочку в имени вкладки
		setWindowTitle("");
		updateUndoRedoActions();

		// устанавливаем текущую сцену
		mLayersWindow->setCurrentScene(editorWindow->getScene(), !editorWindow->isUntitled() ? editorWindow->getFileName() : "");
		mPropertyWindow->onEditorWindowSelectionChanged(editorWindow->getSelectedObjects(), editorWindow->getRotationCenter());
		mHistoryWindow->setUndoStack(editorWindow->getUndoStack());
	}
	else
	{
		// деактивируем нужные пункты меню
		mSaveAction->setEnabled(false);
		mSaveAsAction->setEnabled(false);
		mCloseAction->setEnabled(false);
		mCloseAllAction->setEnabled(false);
		mUndoAction->setEnabled(false);
		mRedoAction->setEnabled(false);
		mPasteAction->setEnabled(false);
		mSelectAllAction->setEnabled(false);
		mZoomMenu->setEnabled(false);
		mZoomInAction->setEnabled(false);
		mZoomOutAction->setEnabled(false);

		// деактивируем пункты меню, связанные с редактированием выделенных объектов
		onEditorWindowSelectionChanged(QList<GameObject *>(), QPointF());

		// деактивируем комбобокс со списком масштабов
		mZoomComboBox->setEnabled(false);

		// очищаем надпись с координатами мыши в строке состояния
		mMousePosLabel->clear();

		// сбрасываем заголовок окна
		setWindowModified(false);
		setWindowFilePath("");
		setWindowTitle(QCoreApplication::applicationName());

		// сбрасываем текущую сцену
		mLayersWindow->setCurrentScene(NULL, "");
		mPropertyWindow->onEditorWindowSelectionChanged(QList<GameObject *>(), QPointF());
		mHistoryWindow->setUndoStack(NULL);
	}
}

void MainWindow::onZoomChanged(const QString &zoomStr)
{
	// синхронизация с ComboBox
	mZoomComboBox->lineEdit()->setText(zoomStr);

	// снятие или установка галочки для пунктов масштабов в меню
	foreach (QAction *action, mZoomMenu->actions())
		action->setChecked(action->text() == zoomStr);

	// получаем числовое значение масштаба из строки
	Q_ASSERT(zoomStr.endsWith('%'));
	QString str = zoomStr.left(zoomStr.size() - 1);
	qreal zoom = str.toDouble() / 100.0;

	// установка масштаба
	EditorWindow *editorWindow = getCurrentEditorWindow();
	if (editorWindow != NULL)
		editorWindow->setZoom(zoom);
}

void MainWindow::onZoomEditingFinished()
{
	// дополняем значение масштаба символом процента
	QString zoomStr = mZoomComboBox->currentText();
	if (!zoomStr.endsWith('%'))
		zoomStr += '%';

	// вызываем обработчик изменения масштаба
	onZoomChanged(zoomStr);
}

void MainWindow::onLanguageChanged(const QString &language)
{
	// сброс фокуса в окне свойств
	mPropertyWindow->clearChildWidgetFocus();

	// устанавливаем текущий язык
	Project::getSingleton().setCurrentLanguage(language);
	for (int i = 0; i < mTabWidget->count(); ++i)
		getEditorWindow(i)->setCurrentLanguage(language);

	// обновляем окна слоев и свойств
	EditorWindow *editorWindow = getCurrentEditorWindow();
	if (editorWindow != NULL)
	{
		mLayersWindow->setCurrentScene(editorWindow->getScene(), !editorWindow->isUntitled() ? editorWindow->getFileName() : "");
		mPropertyWindow->onEditorWindowSelectionChanged(editorWindow->getSelectedObjects(), editorWindow->getRotationCenter());
	}
}

void MainWindow::onTranslationFileChanged(const QString &path)
{
	// помечаем файл переводов как измененный
	TranslationFilesMap::iterator it = mTranslationFilesMap.find(path);
	if (it != mTranslationFilesMap.end())
	{
		it->mChanged = true;
		it->mTimer.start();
	}
}

void MainWindow::onSceneChanged(const QString &commandName)
{
	getCurrentEditorWindow()->pushCommand(commandName);
	updateUndoRedoActions();
}

void MainWindow::onEditorWindowSelectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter)
{
	// разрешаем/запрещаем нужные пункты меню
	bool selected = !objects.empty();
	mCutAction->setEnabled(selected);
	mCopyAction->setEnabled(selected);
	mDeleteAction->setEnabled(selected);
	mBringToFrontAction->setEnabled(selected);
	mSendToBackAction->setEnabled(selected);
	mMoveUpAction->setEnabled(selected);
	mMoveDownAction->setEnabled(selected);
}

void MainWindow::onEditorWindowMouseMoved(const QPointF &pos)
{
	mMousePosLabel->setText(QString("Мышь: %1, %2").arg(qRound(pos.x())).arg(qRound(pos.y())));
}

void MainWindow::onEditorWindowUndoCommandChanged()
{
	// сбрасываем выделение в окне редактирования
	EditorWindow *editorWindow = getCurrentEditorWindow();
	editorWindow->deselectAll();

	// перезагружаем файл переводов
	if (!editorWindow->isUntitled())
		editorWindow->loadTranslationFile(getTranslationFileName(editorWindow->getFileName()));

	// обновляем окно слоев и главное меню
	mLayersWindow->setCurrentScene(editorWindow->getScene(), !editorWindow->isUntitled() ? editorWindow->getFileName() : "");
	updateUndoRedoActions();
}

void MainWindow::onLayerWindowLayerChanged()
{
	getCurrentEditorWindow()->deselectAll();
}

void MainWindow::onPropertyWindowObjectsChanged(const QPointF &rotationCenter)
{
	getCurrentEditorWindow()->updateSelection(rotationCenter);
}

void MainWindow::onPropertyWindowAllowedEditorActionsChanged()
{
	getCurrentEditorWindow()->updateAllowedEditorActions();
}

void MainWindow::onTextureChanged(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем текстуру во всех открытых вкладках
	for (int i = 0; i < mTabWidget->count(); ++i)
		getEditorWindow(i)->changeTexture(fileName, texture);

	// обновляем окно свойств
	EditorWindow *editorWindow = getCurrentEditorWindow();
	if (editorWindow != NULL)
		mPropertyWindow->onEditorWindowSelectionChanged(editorWindow->getSelectedObjects(), editorWindow->getRotationCenter());
}

void MainWindow::onClipboardDataChanged()
{
	// изменяем состояние пункта меню Правка-Вставить
	if (mTabWidget->count() != 0)
		mPasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat("application/x-gameobject"));
}

EditorWindow *MainWindow::createEditorWindow(const QString &fileName)
{
	// создаем новое окно редактора
	EditorWindow *editorWindow = new EditorWindow(this, mPrimaryGLWidget, fileName, mSpriteBrowser->getSpriteWidget(), mFontBrowser->getFontWidget());

	// связываем сигналы от окна редактирования с соответствующими слотами
	connect(editorWindow, SIGNAL(zoomChanged(const QString &)), this, SLOT(onZoomChanged(const QString &)));
	connect(editorWindow, SIGNAL(selectionChanged(const QList<GameObject *> &, const QPointF &)),
		this, SLOT(onEditorWindowSelectionChanged(const QList<GameObject *> &, const QPointF &)));
	connect(editorWindow, SIGNAL(sceneChanged(const QString &)), this, SLOT(onSceneChanged(const QString &)));
	connect(editorWindow, SIGNAL(mouseMoved(const QPointF &)), this, SLOT(onEditorWindowMouseMoved(const QPointF &)));
	connect(editorWindow, SIGNAL(undoCommandChanged()), this, SLOT(onEditorWindowUndoCommandChanged()));
	connect(editorWindow, SIGNAL(layerChanged(Scene *, BaseLayer *)), mLayersWindow, SIGNAL(layerChanged(Scene *, BaseLayer *)));

	// связывание сигналов изменения выделения объектов для отправки в ГУИ настроек
	connect(editorWindow, SIGNAL(selectionChanged(const QList<GameObject *> &, const QPointF &)),
		mPropertyWindow, SLOT(onEditorWindowSelectionChanged(const QList<GameObject *> &, const QPointF &)));
	connect(editorWindow, SIGNAL(objectsChanged(const QList<GameObject *> &, const QPointF &)),
		mPropertyWindow, SLOT(onEditorWindowObjectsChanged(const QList<GameObject *> &, const QPointF &)));

	// активируем нужные пункты меню
	mSaveAsAction->setEnabled(true);
	mCloseAction->setEnabled(true);
	mCloseAllAction->setEnabled(true);
	mSelectAllAction->setEnabled(true);
	mZoomMenu->setEnabled(true);
	mZoomInAction->setEnabled(true);
	mZoomOutAction->setEnabled(true);

	// активируем комбобокс со списком масштабов
	mZoomComboBox->setEnabled(true);

	// возвращаем указатель на созданное окно редактирования
	return editorWindow;
}

QString MainWindow::getTranslationFileName(const QString &fileName) const
{
	Project &project = Project::getSingleton();
	QString scenesDir = project.getRootDirectory() + project.getScenesDirectory();
	QString localizationDir = project.getRootDirectory() + project.getLocalizationDirectory();
	return localizationDir + fileName.mid(scenesDir.size());
}

void MainWindow::updateMainMenuActions()
{
	Options &options = Options::getSingleton();
	mShowGridAction->setChecked(options.isShowGrid());
	mSnapToGridAction->setChecked(options.isSnapToGrid());
	mShowGuidesAction->setChecked(options.isShowGuides());
	mSnapToGuidesAction->setChecked(options.isSnapToGuides());
	mEnableSmartGuidesAction->setChecked(options.isEnableSmartGuides());
}

void MainWindow::updateRecentFilesActions(const QString &fileName)
{
	// добавляем файл в список последних файлов
	QStringList recentFiles = Options::getSingleton().getRecentFiles();
	if (!fileName.isEmpty())
	{
		recentFiles.removeAll(fileName);
		recentFiles.push_front(fileName);
		recentFiles = recentFiles.mid(0, mRecentFilesActions.size());
		Options::getSingleton().setRecentFiles(recentFiles);
	}

	// настраиваем пункты меню
	int numRecentFiles = qMin(recentFiles.size(), mRecentFilesActions.size());
	for (int i = 0; i < numRecentFiles; ++i)
	{
		mRecentFilesActions[i]->setText(QString("&%1 %2").arg(i + 1).arg(recentFiles[i]));
		mRecentFilesActions[i]->setData(recentFiles[i]);
		mRecentFilesActions[i]->setVisible(true);
	}

	// скрываем остальные пункты меню
	for (int i = numRecentFiles; i < mRecentFilesActions.size(); ++i)
		mRecentFilesActions[i]->setVisible(false);

	// показываем разделитель, если есть последние файлы
	mRecentFilesSeparator->setVisible(numRecentFiles != 0);
}

void MainWindow::updateUndoRedoActions()
{
	// добавляем/убираем звездочку в имени вкладки
	EditorWindow *editorWindow = getCurrentEditorWindow();
	bool clean = editorWindow->isClean();
	mTabWidget->setTabText(mTabWidget->currentIndex(), QFileInfo(editorWindow->getFileName()).fileName() + (clean ? "" : "*"));

	// обновляем заголовок окна
	setWindowFilePath(editorWindow->getFileName());
	setWindowModified(!clean);

	// разрешаем/запрещаем пункты меню Файл-Сохранить, Правка-Отменить, Правка-Повторить
	mSaveAction->setEnabled(!clean);
	mUndoAction->setEnabled(editorWindow->canUndo());
	mRedoAction->setEnabled(editorWindow->canRedo());
}

void MainWindow::checkMissedFiles()
{
	// получаем список отсутствующих файлов
	QStringList missedFiles = getCurrentEditorWindow()->getMissedFiles();
	if (!missedFiles.empty())
	{
		// сортируем список и сокращаем его до 5 элементов максимум
		missedFiles.sort();
		if (missedFiles.size() > 5)
		{
			missedFiles = missedFiles.mid(0, 5);
			missedFiles.push_back("...");
		}

		// выдаем предупреждение об отсутствии файлов
		QMessageBox::warning(this, "", "Не найдены следующие файлы, возможно они были удалены или перемещены:\n" + missedFiles.join("\n"));
	}
}

MainWindow::PercentIntValidator::PercentIntValidator(int bottom, int top, MainWindow *parent)
: QIntValidator(bottom, top, parent), mParent(parent)
{
}

void MainWindow::PercentIntValidator::fixup(QString &input) const
{
	// откатываемся к прежнему значению масштаба
	EditorWindow *editorWindow = mParent->getCurrentEditorWindow();
	if (editorWindow != NULL)
		input = QString::number(qRound(editorWindow->getZoom() * 100.0)) + '%';
}

QValidator::State MainWindow::PercentIntValidator::validate(QString &input, int &pos) const
{
	// передаем родителю строку без процента на конце
	QString str = input.endsWith('%') ? input.left(input.size() - 1) : input;
	return QIntValidator::validate(str, pos);
}
