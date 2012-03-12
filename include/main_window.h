#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "ui_main_window.h"

class EditorWindow;
class FontBrowser;
class GameObject;
class LayersWindow;
class PropertyWindow;
class SpriteBrowser;
class Texture;

// Класс главного окна приложения
class MainWindow : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

public:

	// Конструктор
	MainWindow();

	// Деструктор
	virtual ~MainWindow();

	// Возвращает текущее окно редактирования
	EditorWindow *getEditorWindow() const;

	// Возвращает окно редактирования с заданным индексом
	EditorWindow *getEditorWindow(int index) const;

protected:

	// Вызывается по срабатыванию таймера
	virtual void timerEvent(QTimerEvent *event);

	// Вызывается при попытке закрытия окна
	virtual void closeEvent(QCloseEvent *event);

private slots:

	// Обработчик пункта меню Файл-Создать
	void on_mNewAction_triggered();

	// Обработчик пункта меню Файл-Открыть
	void on_mOpenAction_triggered();

	// Обработчик пункта меню Файл-Сохранить
	bool on_mSaveAction_triggered();

	// Обработчик пункта меню Файл-Сохранить как
	bool on_mSaveAsAction_triggered();

	// Обработчик пункта меню Файл-Закрыть
	bool on_mCloseAction_triggered();

	// Обработчик пункта меню Файл-Закрыть все
	bool on_mCloseAllAction_triggered();

	// Обработчик пункта меню Правка-Вырезать
	void on_mCutAction_triggered();

	// Обработчик пункта меню Правка-Копировать
	void on_mCopyAction_triggered();

	// Обработчик пункта меню Правка-Вставить
	void on_mPasteAction_triggered();

	// Обработчик пункта меню Правка-Удалить
	void on_mDeleteAction_triggered();

	// Обработчик пункта меню Правка-Настройки
	void on_mOptionsAction_triggered();

	// Обработчик пункта меню Правка-Выделить все
	void on_mSelectAllAction_triggered();

	// Обработчик пункта меню Вид-Увеличить
	void on_mZoomInAction_triggered();

	// Обработчик пункта меню Вид-Уменьшить
	void on_mZoomOutAction_triggered();

	// Обработчик пункта меню Объект-На передний план
	void on_mBringToFrontAction_triggered();

	// Обработчик пункта меню Объект-На задний план
	void on_mSendToBackAction_triggered();

	// Обработчик пункта меню Объект-Переместить вверх
	void on_mMoveUpAction_triggered();

	// Обработчик пункта меню Объект-Переместить вниз
	void on_mMoveDownAction_triggered();

	// Обработчик закрытия вкладки
	bool on_mTabWidget_tabCloseRequested(int index);

	// Обработчик смены вкладки
	void on_mTabWidget_currentChanged(int index);

	// Обработчик изменения масштаба
	void onZoomChanged(const QString &zoomStr);

	// Редактирование масштаба вручную закончено в ComboBox
	void onZoomEditingFinished();

	// Обработчик изменения выделения в окне редактирования
	void onEditorWindowSelectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter);

	// Обработчик изменения локации в окне редактирования
	void onEditorWindowLocationChanged(bool changed);

	// Обработчик изменения положения курсора мышки на OpenGL окне
	void onEditorWindowMouseMoved(const QPointF &pos);

	// Обработчик изменения локации в окне слоёв
	void onLayerWindowLocationChanged();

	// Обработчик изменения слоя в окне слоёв
	void onLayerWindowLayerChanged();

	// Обработчик сигнала изменения текстуры от текстурного менеджера
	void onTextureChanged(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Обработчик изменения содержимого буфера обмена
	void onClipboardDataChanged();

private:

	// Валидатор формата int с процентом на конце
	class PercentIntValidator : public QIntValidator
	{
	public:

		// Конструктор
		PercentIntValidator(int bottom, int top, MainWindow *parent);

		// Вызывается для отката неверного значения
		virtual void fixup(QString &input) const;

		// Вызывается для проверки введенного значения
		virtual QValidator::State validate(QString &input, int &pos) const;

	private:

		MainWindow *mParent;
	};

	// Создает новое окно редактирования
	EditorWindow *createEditorWindow(const QString &fileName);

	// Обновляет пункты меню с последними файлами
	void updateRecentFilesActions(const QString &fileName);

	// Проверяет имя файла на валидность
	bool checkFileName(const QString &fileName);

	// Проверяет текущую локацию на наличие отсутствующих файлов
	void checkMissedFiles();

	SpriteBrowser       *mSpriteBrowser;        // Браузер спрайтов
	FontBrowser         *mFontBrowser;          // Браузер шрифтов
	PropertyWindow      *mPropertyWindow;       // Окно свойств объекта
	LayersWindow        *mLayersWindow;         // Окно слоев
	int                 mUntitledIndex;         // Текущий номер для новых файлов

	QGLWidget           *mPrimaryGLWidget;      // OpenGL виджет для загрузки текстур в главном потоке
	QGLWidget           *mSecondaryGLWidget;    // OpenGL виджет для загрузки текстур в фоновом потоке
	QLabel              *mMousePosLabel;        // Текстовое поле для координат мыши в строке статуса
	QComboBox           *mZoomComboBox;         // Выпадающий список масштабов
	QList<qreal>        mZoomList;              // Список масштабов
	QList<QAction *>    mRecentFilesActions;    // Список пунктов меню с последними файлами
	QAction             *mRecentFilesSeparator; // Разделитель для пунктов меню с последними файлами
};

#endif // MAIN_WINDOW_H
