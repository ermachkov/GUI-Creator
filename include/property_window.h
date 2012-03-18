#ifndef PROPERTY_WINDOW_H
#define PROPERTY_WINDOW_H

#include "ui_property_window.h"

class GameObject;

class PropertyWindow : public QDockWidget, private Ui::PropertyWindow
{
	Q_OBJECT

public:

	explicit PropertyWindow(QWidget *parent = 0);

signals:

	// отправка сигнала при изменении позиции, размера, цвета, угла поворота
	void objectsChanged(const QPointF &rotationCenter);

public slots:

	// обработчик изменения выделения объектов
	void onEditorWindowSelectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter);

	// обработчик изменеия свойств объекта
	void onEditorWindowObjectsChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter);

protected:

	bool eventFilter(QObject *watched, QEvent *event);

private slots:

	void on_mNameLineEdit_editingFinished();

	void on_mNameEditPushButton_clicked(bool checked);

	void on_mPositionXLineEdit_editingFinished();

	void on_mPositionYLineEdit_editingFinished();

	void on_mSizeWLineEdit_editingFinished();

	void on_mSizeHLineEdit_editingFinished();

	void on_mFlipXCheckBox_clicked(bool checked);

	void on_mFlipYCheckBox_clicked(bool checked);

	void onRotationAngleEditingFinished();

	void on_mRotationCenterXLineEdit_editingFinished();

	void on_mRotationCenterYLineEdit_editingFinished();

	void on_mFileNameLineEdit_editingFinished();

	void on_mFileNameBrowsePushButton_clicked();

	void on_mSpriteOpacityHorizontalSlider_sliderMoved(int position);

	void on_mSpriteOpacityLineEdit_editingFinished();

	void on_mFileNameComboBox_currentIndexChanged(const QString &arg1);

	void onFontSizeEditingFinished();

	void onAlignmentClicked(QAbstractButton *button);

	void onVertAlignmentClicked(QAbstractButton *button);

	void onLineSpacingEditingFinished();

	void on_mLabelOpacityHorizontalSlider_sliderMoved(int position);

	void on_mLabelOpacityLineEdit_editingFinished();

private:

	const int PRECISION;

	// отображение значений параметров объектов в виджетах ГУИ
	void updateCommonWidgets(const QList<GameObject *> &objects, const QPointF &rotationCenter);
	void updateSpriteWidgets(const QList<GameObject *> &objects);
	void updateLabelWidgets(const QList<GameObject *> &objects);

	// просчет текущего общего ограничевающего прямоугольника
	QRectF calculateCurrentBoundingRect();

	// просчет процентеого расположения центра вращения
	QPointF calculatePercentPosition(const QRectF &boundingRect, const QPointF &rotationCenter);

	// просчет расположения центра вращения по процентному
	QPointF calculatePosition(const QRectF &boundingRect, const QPointF &percentCenter);

	QList<GameObject *> mCurrentSelectedObjects;    // текущие выделенные объекты
	QPointF             mRotationCenter;            // текущий центр вращения выделенных объектов

	QButtonGroup        *mAlignmentButtonGroup;     // группировка кнопок горизонтального выравнивания
	QButtonGroup        *mVertAlignmentButtonGroup; // группировка кнопок вертикального выравнивания
};

#endif // PROPERTY_WINDOW_H
