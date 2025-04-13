#ifndef BLOCKLISTWIDGET_H
#define BLOCKLISTWIDGET_H

#include <QTreeWidget>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QMouseEvent>

#include "blockfactory.h"

class BlockListWidget : public QTreeWidget {
    Q_OBJECT

public:
    BlockListWidget(QWidget* parent = nullptr);

    // 블록 카테고리 설정
    void setupBlockCategories();

    // 현재 선택된 블록 타입 이름 가져오기
    QString selectedBlockType() const;

protected:
    // 드래그 & 드롭 관련 이벤트 오버라이드
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

signals:
    void blockTypeSelected(const QString& blockType);

private:
    QPoint m_dragStartPosition;
};

#endif // BLOCKLISTWIDGET_H
