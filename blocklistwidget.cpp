#include "blocklistwidget.h"

#include <QApplication>

BlockListWidget::BlockListWidget(QWidget* parent) : QTreeWidget(parent) {
    setHeaderHidden(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    // 트리 아이템 설정
    setupBlockCategories();

    // 블록 항목 더블 클릭 시그널 연결
    connect(this, &QTreeWidget::itemDoubleClicked, this, &BlockListWidget::onItemDoubleClicked);
}

// 블록 카테고리 설정
void BlockListWidget::setupBlockCategories() {
    clear();

    // 블록 팩토리에서 모든 카테고리 가져오기
    QStringList categories = BlockFactory::instance()->getAllCategories();
    QMap<QString, QTreeWidgetItem*> categoryItems;

    // 카테고리별 트리 아이템 생성
    for (const QString& category : categories) {
        QTreeWidgetItem* categoryItem = new QTreeWidgetItem(this);
        categoryItem->setText(0, category);
        categoryItem->setFlags(Qt::ItemIsEnabled);
        categoryItems[category] = categoryItem;
    }

    // 각 블록 타입을 해당 카테고리에 추가
    for (const BlockInfo& blockInfo : BlockFactory::instance()->getAllBlockTypes()) {
        QString category = blockInfo.category();
        if (categoryItems.contains(category)) {
            QTreeWidgetItem* blockItem = new QTreeWidgetItem(categoryItems[category]);
            blockItem->setText(0, blockInfo.name());
            blockItem->setToolTip(0, blockInfo.description());
            blockItem->setData(0, Qt::UserRole, blockInfo.name());
            blockItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);

            // 아이콘이 있으면 설정
            if (!blockInfo.icon().isNull()) {
                blockItem->setIcon(0, blockInfo.icon());
            }
        }
    }

    // 모든 카테고리 펼치기
    expandAll();
}

// 현재 선택된 블록 타입 이름 가져오기
QString BlockListWidget::selectedBlockType() const {
    QTreeWidgetItem* item = currentItem();
    if (item && item->parent()) { // 부모가 있으면 블록 항목임
        return item->data(0, Qt::UserRole).toString();
    }
    return QString();
}

void BlockListWidget::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (item && item->parent()) { // 부모가 있으면 블록 항목임
        emit blockTypeSelected(item->data(0, Qt::UserRole).toString());
    }
}

// 마우스 버튼 누르기 이벤트
void BlockListWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    QTreeWidget::mousePressEvent(event);
}

// 마우스 이동 이벤트
void BlockListWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if ((event->pos() - m_dragStartPosition).manhattanLength()
        < QApplication::startDragDistance()) {
        return;
    }

    QTreeWidgetItem* item = itemAt(m_dragStartPosition);
    if (item && item->parent()) { // 부모가 있으면 블록 항목임
        startDrag(Qt::CopyAction);
    }
}

// 드래그 시작
void BlockListWidget::startDrag(Qt::DropActions supportedActions) {
    QTreeWidgetItem* item = currentItem();
    if (!item || !item->parent()) {
        return; // 카테고리 항목은 드래그하지 않음
    }

    // 드래그 할 블록 타입 가져오기
    QString blockType = item->data(0, Qt::UserRole).toString();

    // MimeData 생성 및 설정
    QMimeData* mimeData = new QMimeData();
    mimeData->setText(blockType);

    // 드래그 객체 생성
    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // 드래그 이미지 설정 (선택 사항)
    QPixmap pixmap(200, 30);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setPen(Qt::black);
    painter.drawText(QRect(0, 0, 200, 30), Qt::AlignCenter, item->text(0));
    painter.end();

    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

    // 드래그 시작
    drag->exec(supportedActions);
}
