#include "pch.h"
#include "main_window.h"
#include "editor_window.h"
#include "font_browser.h"
#include "font_manager.h"
#include "layers_window.h"
#include "options.h"
#include "options_dialog.h"
#include "property_window.h"
#include "sprite_browser.h"
#include "texture_manager.h"
#include "utils.h"

MainWindow::MainWindow()
: mUntitledIndex(1)
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
	new FontManager(mPrimaryGLWidget);
	new TextureManager(mPrimaryGLWidget, mSecondaryGLWidget);

	// создаем браузер спрайтов
	mSpriteBrowser = new SpriteBrowser(this);
	addDockWidget(Qt::LeftDockWidgetArea, mSpriteBrowser);

	// создаем браузер шрифтов
	mFontBrowser = new FontBrowser(this);
	addDockWidget(Qt::LeftDockWidgetArea, mFontBrowser);

	// создаем окно свойств объекта
	mPropertyWindow = new PropertyWindow(this);
	addDockWidget(Qt::RightDockWidgetArea, mPropertyWindow);

	// создаем окно слоев
	mLayersWindow = new LayersWindow(mPrimaryGLWidget, this);
	addDockWidget(Qt::RightDockWidgetArea, mLayersWindow);

	// наложение плавающих окон друг на друга
	tabifyDockWidget(mSpriteBrowser, mFontBrowser);

	// установка нулевого индекса для всех таббаров
	QList<QTabBar *> tabBars = findChildren<QTabBar *>();
	foreach (QTabBar *tabBar, tabBars)
		if (tabBar->count() != 0)
			tabBar->setCurrentIndex(0);

	// разворачивание окна редактора на весь экран
	showMaximized();

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
	mSpriteBrowser->toggleViewAction()->setText("&Спрайты");
	mFontBrowser->toggleViewAction()->setText("&Шрифты");
	mPropertyWindow->toggleViewAction()->setText("С&войства");
	mLayersWindow->toggleViewAction()->setText("С&лои");

	// добавляем в меню Вид пункты для плавающих окон
	QAction *zoomAction = mZoomMenu->menuAction();
	mViewMenu->insertAction(zoomAction, mSpriteBrowser->toggleViewAction());
	mViewMenu->insertAction(zoomAction, mLayersWindow->toggleViewAction());
	mViewMenu->insertSeparator(zoomAction);

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
	QSignalMapper *zoomMapper = new QSignalMapper(this);

	// создаем список масштабов
	const qreal ZOOM_LIST[] = {0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0};
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
		zoomMapper->setMapping(action, zoomStr);
		connect(action, SIGNAL(triggered()), zoomMapper, SLOT(map()));
	}

	// связываем сигналы изменения масштаба от пунктов меню и комбобокса
	connect(zoomMapper, SIGNAL(mapped(const QString &)), this, SLOT(onZoomChanged(const QString &)));
	connect(mZoomComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(onZoomChanged(const QString &)));
	connect(mZoomComboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onZoomEditingFinished()));

	// связываем сигналы об изменениях в окне слоев
	connect(mLayersWindow, SIGNAL(locationChanged()), this, SLOT(onLayerWindowLocationChanged()));
	connect(mLayersWindow, SIGNAL(layerChanged()), this, SLOT(onLayerWindowLayerChanged()));

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
	// удаляем объекты редактора в нужном порядке
	delete mSpriteBrowser;
	delete mFontBrowser;
	delete mLayersWindow;

	// удаляем синглетоны в последнюю очередь
	TextureManager::destroy();
	FontManager::destroy();
	Options::destroy();
}

EditorWindow *MainWindow::getEditorWindow() const
{
	return static_cast<EditorWindow *>(mTabWidget->currentWidget());
}

EditorWindow *MainWindow::getEditorWindow(int index) const
{
	return static_cast<EditorWindow *>(mTabWidget->widget(index));
}

void MainWindow::timerEvent(QTimerEvent *event)
{
	// перерисовываем текущую открытую вкладку
	if (mTabWidget->currentWidget() != NULL)
		mTabWidget->currentWidget()->update();

	// обрабатываем все события в очереди
	QCoreApplication::processEvents();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// закрываем все открытые вкладки и сохраняем настройки приложения перед выходом
	if (on_mCloseAllAction_triggered())
	{
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
	QString fileName = mUntitledIndex == 1 ? "untitled.map" : QString("untitled_%1.map").arg(mUntitledIndex);
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
	if (fileName.isEmpty())
		fileName = QFileDialog::getOpenFileName(this, "Открыть", Options::getSingleton().getLastDirectory(), "Файлы карт (*.map);;Все файлы (*)");
	if (!checkFileName(fileName))
		return;

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

	// создаем новую вкладку и переключаемся на нее
	mTabWidget->addTab(editorWindow, QFileInfo(fileName).fileName());
	mTabWidget->setCurrentWidget(editorWindow);
	onClipboardDataChanged();

	// обновляем список последних файлов
	updateRecentFilesActions(fileName);

	// проверяем локацию на наличие отсутствующих файлов
	checkMissedFiles();
}

bool MainWindow::on_mSaveAction_triggered()
{
	// если файл не был ни разу сохранен, показываем диалоговое окно сохранения файла
	EditorWindow *editorWindow = getEditorWindow();
	if (!editorWindow->isSaved())
		return on_mSaveAsAction_triggered();

	// сохраняем файл
	if (!editorWindow->save(editorWindow->getFileName()))
	{
		QMessageBox::critical(this, "", "Ошибка сохранения файла " + editorWindow->getFileName());
		return false;
	}

	return true;
}

bool MainWindow::on_mSaveAsAction_triggered()
{
	// показываем диалоговое окно сохранения файла
	EditorWindow *editorWindow = getEditorWindow();
	QString dir = editorWindow->isSaved() ? editorWindow->getFileName() : Options::getSingleton().getLastDirectory() + editorWindow->getFileName();
	QString fileName = QFileDialog::getSaveFileName(this, "Сохранить как", dir, "Файлы карт (*.map);;Все файлы (*)");
	if (!checkFileName(fileName))
		return false;

	// сохраняем файл
	if (!editorWindow->save(fileName))
	{
		QMessageBox::critical(this, "", "Ошибка сохранения файла " + fileName);
		return false;
	}

	// обновляем список последних файлов
	updateRecentFilesActions(fileName);
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

void MainWindow::on_mCutAction_triggered()
{
	getEditorWindow()->cut();
}

void MainWindow::on_mCopyAction_triggered()
{
	getEditorWindow()->copy();
}

void MainWindow::on_mPasteAction_triggered()
{
	getEditorWindow()->paste();
}

void MainWindow::on_mDeleteAction_triggered()
{
	getEditorWindow()->clear();
}

void MainWindow::on_mOptionsAction_triggered()
{
	OptionsDialog dialog(this);
	dialog.exec();
}

void MainWindow::on_mSelectAllAction_triggered()
{
	getEditorWindow()->selectAll();
}

void MainWindow::on_mZoomInAction_triggered()
{
	// ищем ближайший следующий масштаб в списке
	qreal zoom = getEditorWindow()->getZoom();
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
	qreal zoom = getEditorWindow()->getZoom();
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

void MainWindow::on_mBringToFrontAction_triggered()
{
	getEditorWindow()->bringToFront();
}

void MainWindow::on_mSendToBackAction_triggered()
{
	getEditorWindow()->sendToBack();
}

void MainWindow::on_mMoveUpAction_triggered()
{
	getEditorWindow()->moveUp();
}

void MainWindow::on_mMoveDownAction_triggered()
{
	getEditorWindow()->moveDown();
}

bool MainWindow::on_mTabWidget_tabCloseRequested(int index)
{
	// запрашиваем подтверждение на сохранение, если файл был изменен
	EditorWindow *editorWindow = getEditorWindow(index);
	bool close = true;
	if (editorWindow->isChanged())
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
		mTabWidget->removeTab(index);
		delete editorWindow;
	}

	return close;
}

void MainWindow::on_mTabWidget_currentChanged(int index)
{
	if (index != -1)
	{
		// вызываем обработчик изменения масштаба
		EditorWindow *editorWindow = getEditorWindow(index);
		QString zoomStr = QString::number(qRound(editorWindow->getZoom() * 100.0)) + '%';
		onZoomChanged(zoomStr);

		// обновляем пункты меню, связанные с редактированием выделенных объектов
		onEditorWindowSelectionChanged(editorWindow->getSelectedObjects());

		// обновляем пункт меню Файл-Сохранить
		mSaveAction->setEnabled(editorWindow->isChanged());

		// обновляем заголовок окна
		setWindowTitle("");
		setWindowFilePath(editorWindow->getFileName());
		setWindowModified(editorWindow->isChanged());

		// устанавливаем текущую локацию
		mLayersWindow->setCurrentLocation(editorWindow->getLocation());
	}
	else
	{
		// деактивируем нужные пункты меню
		mSaveAction->setEnabled(false);
		mSaveAsAction->setEnabled(false);
		mCloseAction->setEnabled(false);
		mCloseAllAction->setEnabled(false);
		mPasteAction->setEnabled(false);
		mSelectAllAction->setEnabled(false);
		mZoomMenu->setEnabled(false);
		mZoomInAction->setEnabled(false);
		mZoomOutAction->setEnabled(false);

		// деактивируем пункты меню, связанные с редактированием выделенных объектов
		onEditorWindowSelectionChanged(QList<GameObject *>());

		// деактивируем комбобокс со списком масштабов
		mZoomComboBox->setEnabled(false);

		// очищаем надпись с координатами мыши в строке состояния
		mMousePosLabel->clear();

		// сбрасываем заголовок окна
		setWindowModified(false);
		setWindowFilePath("");
		setWindowTitle(QApplication::applicationName());

		// сбрасываем текущую локацию
		mLayersWindow->setCurrentLocation(NULL);
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
	EditorWindow *editorWindow = getEditorWindow();
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

void MainWindow::onEditorWindowSelectionChanged(const QList<GameObject *> &objects)
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

void MainWindow::onEditorWindowLocationChanged(bool changed)
{
	// добавляем/убираем звездочку в имени вкладки
	EditorWindow *editorWindow = getEditorWindow();
	mTabWidget->setTabText(mTabWidget->currentIndex(), QFileInfo(editorWindow->getFileName()).fileName() + (changed ? "*" : ""));

	// обновляем заголовок окна
	setWindowFilePath(editorWindow->getFileName());
	setWindowModified(changed);

	// разрешаем/запрещаем пункт меню Файл-Сохранить
	mSaveAction->setEnabled(changed);
}

void MainWindow::onEditorWindowMouseMoved(const QPointF &pos)
{
	mMousePosLabel->setText(QString("Мышь: %1, %2").arg(qFloor(pos.x())).arg(qFloor(pos.y())));
}

void MainWindow::onLayerWindowLocationChanged()
{
	getEditorWindow()->setChanged(true);
}

void MainWindow::onLayerWindowLayerChanged()
{
	getEditorWindow()->deselectAll();
}

void MainWindow::onTextureChanged(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем текстуру во всех открытых вкладках
	for (int i = 0; i < mTabWidget->count(); ++i)
		getEditorWindow(i)->changeTexture(fileName, texture);
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
	EditorWindow *editorWindow = new EditorWindow(this, mPrimaryGLWidget, fileName);

	// связываем сигналы от окна редактирования с соответствующими слотами
	connect(editorWindow, SIGNAL(zoomChanged(const QString &)), this, SLOT(onZoomChanged(const QString &)));
	connect(editorWindow, SIGNAL(selectionChanged(const QList<GameObject *> &)), this, SLOT(onEditorWindowSelectionChanged(const QList<GameObject *> &)));
	connect(editorWindow, SIGNAL(locationChanged(bool)), this, SLOT(onEditorWindowLocationChanged(bool)));
	connect(editorWindow, SIGNAL(mouseMoved(const QPointF &)), this, SLOT(onEditorWindowMouseMoved(const QPointF &)));
	connect(editorWindow, SIGNAL(layerChanged(Location *, BaseLayer *)), mLayersWindow, SIGNAL(layerChanged(Location *, BaseLayer *)));

	// связывание сигнала изменения выделения объекта(-ов) для отправки в ГУИ настроек
	connect(editorWindow, SIGNAL(selectionChanged(const QList<GameObject *> &)), mPropertyWindow, SLOT(onEditorWindowSelectionChanged(const QList<GameObject *> &)));

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

bool MainWindow::checkFileName(const QString &fileName)
{
	// проверяем, что имя не пустое
	if (fileName.isEmpty())
		return false;

	// сохраняем каталог с выбранным файлом
	Options::getSingleton().setLastDirectory(QFileInfo(fileName).path());

	// проверяем, что файл находится в каталоге данных
	QString dataDir = Options::getSingleton().getDataDirectory();
	if (!fileName.startsWith(dataDir))
	{
		QMessageBox::warning(this, "", "Неверный путь к файлу " + fileName + "\nСохраняйте все файлы только внутри папки проекта " + dataDir);
		return false;
	}

	// проверяем относительный путь к файлу на валидность
	QString relativePath = fileName.mid(dataDir.size());
	if (!Utils::isFileNameValid(relativePath))
	{
		QMessageBox::warning(this, "", "Неверный путь к файлу " + relativePath + "\nПереименуйте файлы и папки так, чтобы они "
			"начинались с буквы и состояли только из маленьких латинских букв, цифр и знаков подчеркивания");
		return false;
	}

	return true;
}

void MainWindow::checkMissedFiles()
{
	// получаем список отсутствующих файлов
	QStringList missedFiles = getEditorWindow()->getMissedFiles();
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
	EditorWindow *editorWindow = mParent->getEditorWindow();
	if (editorWindow != NULL)
		input = QString::number(qRound(editorWindow->getZoom() * 100.0)) + '%';
}

QValidator::State MainWindow::PercentIntValidator::validate(QString &input, int &pos) const
{
	// передаем родителю строку без процента на конце
	QString str = input.endsWith('%') ? input.left(input.size() - 1) : input;
	return QIntValidator::validate(str, pos);
}
