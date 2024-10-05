#pragma once
#include <QTableView>
#include <QWidget>

class ReferenceTableView : public QTableView
{
	Q_OBJECT

		void mousePressEvent(QMouseEvent* event) override;

signals:
	void clickOnEmptySpace();

public:
	explicit ReferenceTableView(QWidget* parent = nullptr);
};
