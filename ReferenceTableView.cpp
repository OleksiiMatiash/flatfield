#include "ReferenceTableView.h"
#include <QMouseEvent>

ReferenceTableView::ReferenceTableView(QWidget* parent) : QTableView(parent)
{
}

void ReferenceTableView::mousePressEvent(QMouseEvent* event)
{
	QTableView::mousePressEvent(event);

	QModelIndex item = indexAt(event->pos());

	if (!item.isValid())
	{
		clearSelection();
		emit clickOnEmptySpace();
	}
}