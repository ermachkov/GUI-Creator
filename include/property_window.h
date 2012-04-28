#ifndef PROPERTY_WINDOW_H
#define PROPERTY_WINDOW_H

#include "ui_property_window.h"

class GameObject;

class PropertyWindow : public QDockWidget, private Ui::PropertyWindow
{
	Q_OBJECT

public:

	explicit PropertyWindow(QWidget *parent = 0);

	// сброс фокуса в окне свойств
	void clearChildWidgetFocus();

signals:

	// отправка сигнала при изменении позиции, размера, цвета, угла поворота
	void objectsChanged(const QPointF &rotationCenter);

	// Сигнал об изменении локализации выделенных объектов
	void localizationChanged();

public slots:

	// обработчик изменения выделения объектов
	void onEditorWindowSelectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter);

	// обработчик изменения свойств объекта
	void onEditorWindowObjectsChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter);

protected:

	virtual bool eventFilter(QObject *object, QEvent *event);

private slots:

	void on_mNameLineEdit_editingFinished();

	void on_mTextEditPushButton_clicked(bool checked);

	void on_mPositionXLineEdit_editingFinished();

	void on_mPositionYLineEdit_editingFinished();

	void on_mSizeWLineEdit_editingFinished();

	void on_mSizeHLineEdit_editingFinished();

	void on_mFlipXCheckBox_clicked(bool checked);

	void on_mFlipYCheckBox_clicked(bool checked);

	void onRotationAngleEditingFinished();

//	void on_mRotationAngleComboBox_currentIndexChanged(const QString &arg);

	void on_mRotationAngleComboBox_activated(const QString &arg);

	void on_mRotationCenterXLineEdit_editingFinished();

	void on_mRotationCenterYLineEdit_editingFinished();

	void on_mSpriteFileNameBrowsePushButton_clicked();

	void on_mSpriteOpacitySlider_valueChanged(int value);

//	void on_mLabelFileNameComboBox_currentIndexChanged(const QString &arg);

	void on_mLabelFileNameComboBox_activated(const QString &arg);

	void onFontSizeEditingFinished();

//	void on_mFontSizeComboBox_currentIndexChanged(const QString &arg);

	void on_mFontSizeComboBox_activated(const QString &arg);

	void onHorzAlignmentClicked(QAbstractButton *button);

	void onVertAlignmentClicked(QAbstractButton *button);

	void onLineSpacingEditingFinished();

//	void on_mLineSpacingComboBox_currentIndexChanged(const QString &arg);

	void on_mLineSpacingComboBox_activated(const QString &arg);

	void on_mLabelOpacitySlider_valueChanged(int value);

	void on_mLocalizationPushButton_clicked();

private:

	static const int PRECISION = 8;

	// Устанавливает видимость для виджетов со свойствами спрайтов
	void setSpriteWidgetsVisible(bool visible);

	// Устанавливает видимость для виджетов со свойствами надписей
	void setLabelWidgetsVisible(bool visible);

	// Устанавливает видимость для заданного диапазона строк в грид лейауте
	void setGridLayoutRowsVisible(QGridLayout *layout, int firstRow, int numRows, bool visible);

	// Устанавливает видимость для элемента лейаута
	void setLayoutItemVisible(QLayoutItem *item, bool visible);

	// Активирует/деактивирует элемент для заданного диапазона строк в грид лейауте
	void setGridLayoutRowsEnabled(QGridLayout *layout, int firstRow, int numRows, bool enable);

	// Активирует/деактивирует элемент лейаута
	void setLayoutItemEnabled(QLayoutItem *item, bool enable);

	// отображение значений параметров объектов в виджетах ГУИ
	void updateCommonWidgets();
	void updateSpriteWidgets();
	void updateLabelWidgets();

	// просчет текущего общего ограничевающего прямоугольника
	QRectF calculateCurrentBoundingRect();

	// просчет процентеого расположения центра вращения
	QPointF calculatePercentPosition(const QRectF &boundingRect, const QPointF &rotationCenter);

	// просчет расположения центра вращения по процентному
	QPointF calculatePosition(const QRectF &boundingRect, const QPointF &percentCenter);

	// возврат из опций текущей коренной директории
	QString getRootPath() const;

	QString getSpritesPath() const;

	QString getFontsPath() const;

	// загрузка и отображение списка доступных шрифтов в комбобоксе
	void scanFonts();

	QList<GameObject *> mSelectedObjects;           // текущие выделенные объекты
	QPointF             mRotationCenter;            // текущий центр вращения выделенных объектов

	QButtonGroup        *mHorzAlignmentButtonGroup; // группировка кнопок горизонтального выравнивания
	QButtonGroup        *mVertAlignmentButtonGroup; // группировка кнопок вертикального выравнивания
};

#endif // PROPERTY_WINDOW_H
