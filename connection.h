#ifndef CONNECTION_H
#define CONNECTION_H

#include <QGraphicsLineItem>
#include <QPen>
#include <QGraphicsPathItem>

#include "block.h"

class Connection : public QGraphicsPathItem  {
    //Q_OBJECT
public:
    Connection(Block* sourceBlock, int sourcePortIndex, Block* destBlock, int destPortIndex, QGraphicsItem* parent = nullptr)
        : QGraphicsPathItem(parent),
        m_sourceBlock(sourceBlock), m_sourcePortIndex(sourcePortIndex),
        m_destBlock(destBlock), m_destPortIndex(destPortIndex),
        m_isDragging(false), m_draggedEnd(None) {

        // 선택 가능하도록 설정
        setFlag(QGraphicsItem::ItemIsSelectable);
        setZValue(0);

        // 드래그하면 커서 변경
        setCursor(Qt::PointingHandCursor);

        // 시각적 스타일 설정
        setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        updatePath();
    }
    enum DraggedEnd {
        None,
        Source,
        Destination
    };
    QRectF boundingRect() const override {
        // 기본 boundingRect에 여유 추가
        return QGraphicsPathItem::boundingRect().adjusted(-5, -5, 5, 5);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override {
        // 선택 시 하이라이트 표시
        if (isSelected()) {
            painter->setPen(QPen(Qt::blue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        } else {
            painter->setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        }

        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path());
    }

    void updatePosition() {
        updatePath();
        update();
    }
    void updatePath() {
        if (!m_sourceBlock || !m_destBlock) return;

        QPointF sourcePos = m_sourceBlock->getOutputPortPos(m_sourcePortIndex);
        QPointF destPos = m_destBlock->getInputPortPos(m_destPortIndex);

        // 베지어 곡선 생성
        QPainterPath path;
        path.moveTo(sourcePos);

        // 제어점: 소스와 목적지 사이의 거리에 비례하여 설정
        qreal dx = destPos.x() - sourcePos.x();
        qreal ctrl1X = sourcePos.x() + dx * 0.4;
        qreal ctrl2X = sourcePos.x() + dx * 0.6;

        path.cubicTo(
            QPointF(ctrl1X, sourcePos.y()),
            QPointF(ctrl2X, destPos.y()),
            destPos
            );

        setPath(path);
    }
    /*void updatePosition() {
        if (m_sourceBlock && m_destBlock) {
            QPointF sourcePos = m_sourceBlock->getOutputPortPos(m_sourcePortIndex);
            QPointF destPos = m_destBlock->getInputPortPos(m_destPortIndex);

            setLine(QLineF(sourcePos, destPos));
        }
    }*/

    Block* getSourceBlock() const { return m_sourceBlock; }
    int getSourcePortIndex() const { return m_sourcePortIndex; }
    Block* getDestBlock() const { return m_destBlock; }
    int getDestPortIndex() const { return m_destPortIndex; }

    // 소스 블록과 포트 설정
    void setSource(Block* block, int portIndex) {
        m_sourceBlock = block;
        m_sourcePortIndex = portIndex;
        updatePath();
    }

    // 대상 블록과 포트 설정
    void setDestination(Block* block, int portIndex) {
        m_destBlock = block;
        m_destPortIndex = portIndex;
        updatePath();
    }

private:
    Block* m_sourceBlock;
    int m_sourcePortIndex;
    Block* m_destBlock;
    int m_destPortIndex;

    bool m_isDragging;
    DraggedEnd m_draggedEnd;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_isDragging = true;

            // 클릭한 위치가 소스에 가까운지 대상에 가까운지 확인
            QPointF sourcePos = m_sourceBlock->getOutputPortPos(m_sourcePortIndex);
            QPointF destPos = m_destBlock->getInputPortPos(m_destPortIndex);

            qreal sourceDistance = QLineF(event->pos(), sourcePos).length();
            qreal destDistance = QLineF(event->pos(), destPos).length();

            if (sourceDistance < destDistance) {
                m_draggedEnd = Source;
            } else {
                m_draggedEnd = Destination;
            }

            // 커서 변경
            setCursor(Qt::ClosedHandCursor);
        }

        // 항상 선택되도록
        setSelected(true);
        QGraphicsPathItem::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_isDragging) {
            QPointF pos = event->scenePos();

            // 씬에서 블록과 포트 찾기
            Block* targetBlock = nullptr;
            int targetPortIndex = -1;
            bool foundTarget = false;

            foreach (QGraphicsItem* item, scene()->items(pos)) {
                Block* block = dynamic_cast<Block*>(item);
                if (block) {
                    // 드래그 중인 엔드에 따라 입력 또는 출력 포트 확인
                    if (m_draggedEnd == Source) {
                        // 소스 끝을 드래그 중이면 새 소스 블록의 출력 포트를 찾음
                        if (block->containsOutputPort(pos, targetPortIndex)) {
                            targetBlock = block;
                            foundTarget = true;
                            break;
                        }
                    } else {
                        // 대상 끝을 드래그 중이면 새 대상 블록의 입력 포트를 찾음
                        if (block->containsInputPort(pos, targetPortIndex)) {
                            targetBlock = block;
                            foundTarget = true;
                            break;
                        }
                    }
                }
            }

            // 임시로 연결 라인 업데이트
            if (m_draggedEnd == Source) {
                if (foundTarget) {
                    // 새 소스 블록과 포트에 연결
                    setSource(targetBlock, targetPortIndex);
                } else {
                    // 마우스 위치까지 임시 경로 그리기
                    QPainterPath path;
                    path.moveTo(pos);

                    QPointF destPos = m_destBlock->getInputPortPos(m_destPortIndex);
                    qreal dx = destPos.x() - pos.x();
                    qreal ctrl1X = pos.x() + dx * 0.4;
                    qreal ctrl2X = pos.x() + dx * 0.6;

                    path.cubicTo(
                        QPointF(ctrl1X, pos.y()),
                        QPointF(ctrl2X, destPos.y()),
                        destPos
                        );

                    setPath(path);
                }
            } else {
                if (foundTarget) {
                    // 새 대상 블록과 포트에 연결
                    setDestination(targetBlock, targetPortIndex);
                } else {
                    // 마우스 위치까지 임시 경로 그리기
                    QPainterPath path;
                    QPointF sourcePos = m_sourceBlock->getOutputPortPos(m_sourcePortIndex);
                    path.moveTo(sourcePos);

                    qreal dx = pos.x() - sourcePos.x();
                    qreal ctrl1X = sourcePos.x() + dx * 0.4;
                    qreal ctrl2X = sourcePos.x() + dx * 0.6;

                    path.cubicTo(
                        QPointF(ctrl1X, sourcePos.y()),
                        QPointF(ctrl2X, pos.y()),
                        pos
                        );

                    setPath(path);
                }
            }
        }

        QGraphicsPathItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_isDragging) {
            m_isDragging = false;
            m_draggedEnd = None;

            // 커서 복원
            setCursor(Qt::PointingHandCursor);

            // 연결이 유효한지 확인
            if (!m_sourceBlock || !m_destBlock) {
                // 유효하지 않은 연결은 삭제
                scene()->removeItem(this);
                delete this;
                return;
            }

            // 최종 경로 업데이트
            updatePath();
        }

        QGraphicsPathItem::mouseReleaseEvent(event);
    }
};
#endif // CONNECTION_H
