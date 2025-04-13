#ifndef SIMULATION_H
#define SIMULATION_H


#include <QTimer>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>

#include "block.h"
#include "connection.h"



class SimulationScene : public QGraphicsScene {
    Q_OBJECT
public:
    SimulationScene(QObject* parent = nullptr)
        : QGraphicsScene(parent), m_connectionMode(false), m_tempConnection(nullptr),
        m_sourceBlock(nullptr), m_sourcePortIndex(-1) {
    }

    // 연결 모드 상태 확인 메서드 추가
    bool isConnectionMode() const {
        return m_connectionMode;
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
            Block* block = dynamic_cast<Block*>(item);

            if (block) {
                int portIndex = -1;
                if (block->containsOutputPort(event->scenePos(), portIndex)) {
                    m_connectionMode = true;
                    m_sourceBlock = block;
                    m_sourcePortIndex = portIndex;

                    // 임시 연결선을 생성 - 이제 직선 대신 곡선 사용
                    QPainterPath path;
                    QPointF startPos = block->getOutputPortPos(portIndex);
                    path.moveTo(startPos);

                    // 베지어 곡선을 위한 제어점 계산
                    QPointF endPos = event->scenePos();
                    qreal dx = endPos.x() - startPos.x();
                    qreal ctrl1X = startPos.x() + dx * 0.4;
                    qreal ctrl2X = startPos.x() + dx * 0.6;

                    path.cubicTo(
                        QPointF(ctrl1X, startPos.y()),
                        QPointF(ctrl2X, endPos.y()),
                        endPos
                        );

                    m_tempConnection = new QGraphicsPathItem(path);
                    m_tempConnection->setPen(QPen(Qt::darkGray, 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
                    addItem(m_tempConnection);

                    // 이벤트를 소비했음을 표시 (중요: 이벤트 전파 중단)
                    event->accept();
                    return;
                }
            }
        }

        // 연결 모드가 아닌 경우에만 기본 이벤트 처리
        QGraphicsScene::mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_connectionMode && m_tempConnection && m_sourceBlock) {
            // 베지어 곡선을 위한 임시 연결 업데이트
            QPointF startPos = m_sourceBlock->getOutputPortPos(m_sourcePortIndex);
            QPointF endPos = event->scenePos();

            QPainterPath path;
            path.moveTo(startPos);

            // 베지어 곡선을 위한 제어점 계산
            qreal dx = endPos.x() - startPos.x();
            qreal ctrl1X = startPos.x() + dx * 0.4;
            qreal ctrl2X = startPos.x() + dx * 0.6;

            path.cubicTo(
                QPointF(ctrl1X, startPos.y()),
                QPointF(ctrl2X, endPos.y()),
                endPos
                );

            m_tempConnection->setPath(path);

            // 현재 위치 아래 있는 입력 포트 찾기
            Block* destBlock = nullptr;
            int portIndex = -1;

            foreach (QGraphicsItem* item, items(event->scenePos())) {
                Block* block = dynamic_cast<Block*>(item);
                if (block && block != m_sourceBlock) {
                    if (block->containsInputPort(event->scenePos(), portIndex)) {
                        destBlock = block;
                        break;
                    }
                }
            }

            // 연결 가능한 포트 위에 있을 때 시각적 피드백 변경
            if (destBlock) {
                m_tempConnection->setPen(QPen(Qt::green, 3, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
            } else {
                m_tempConnection->setPen(QPen(Qt::darkGray, 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
            }

            // 이벤트를 소비했음을 표시 (중요: 이벤트 전파 중단)
            event->accept();
            return;
        }

        QGraphicsScene::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        if (m_connectionMode && m_tempConnection && m_sourceBlock) {
            QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
            Block* destBlock = dynamic_cast<Block*>(item);

            if (destBlock && destBlock != m_sourceBlock) {
                int portIndex = -1;
                if (destBlock->containsInputPort(event->scenePos(), portIndex)) {
                    // 연결 생성
                    Connection* connection = new Connection(
                        m_sourceBlock, m_sourcePortIndex, destBlock, portIndex);
                    addItem(connection);
                    m_connections.push_back(connection);

                    emit connectionCreated(connection);
                }
            }

            // 임시 연결선 제거
            removeItem(m_tempConnection);
            delete m_tempConnection;
            m_tempConnection = nullptr;

            // 연결 모드 종료
            m_connectionMode = false;
            m_sourceBlock = nullptr;
            m_sourcePortIndex = -1;

            // 이벤트를 소비했음을 표시 (중요: 이벤트 전파 중단)
            event->accept();
            return;
        }

        QGraphicsScene::mouseReleaseEvent(event);
    }

    void updateConnections() {
        for (auto connection : m_connections) {
            connection->updatePosition();
        }
    }

    std::vector<Connection*> getConnections() const {
        return m_connections;
    }

    // 항목 제거 시 연결도 제거
    void removeItem(QGraphicsItem* item) {
        // 블록이 제거되는 경우, 관련된 모든 연결 제거
        Block* block = dynamic_cast<Block*>(item);
        if (block) {
            auto it = m_connections.begin();
            while (it != m_connections.end()) {
                Connection* conn = *it;
                if (conn->getSourceBlock() == block || conn->getDestBlock() == block) {
                    // 먼저 연결을 씬에서 제거
                    QGraphicsScene::removeItem(conn);
                    // 그런 다음 메모리에서 제거
                    delete conn;
                    // 벡터에서 제거
                    it = m_connections.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Connection이 제거되는 경우, 목록에서도 제거
        Connection* connection = dynamic_cast<Connection*>(item);
        if (connection) {
            auto it = std::find(m_connections.begin(), m_connections.end(), connection);
            if (it != m_connections.end()) {
                m_connections.erase(it);
            }
        }

        // 기본 삭제 동작 수행
        QGraphicsScene::removeItem(item);
    }

    // 현재 씬을 지울 때 모든 연결 목록도 지움
    void clear() {
        m_connections.clear();
        QGraphicsScene::clear();
    }

signals:
    void connectionCreated(Connection* connection);
private:
    // 연결 가능한 포트들을 시각적으로 하이라이트
    void highlightCompatiblePorts() {
        if (!m_sourceBlock) return;

        // 모든 블록을 순회하면서 입력 포트 찾기
        foreach (QGraphicsItem* item, items()) {
            Block* block = dynamic_cast<Block*>(item);
            if (block && block != m_sourceBlock) {
                // 블록의 모든 입력 포트에 대해 시각적 힌트 제공
                for (int i = 0; i < block->getInputPortCount(); ++i) {
                    // 여기서는 실제 블록 클래스에 포트 하이라이트 기능이 추가되어 있다고 가정
                    // 실제 구현에서는 이 부분을 Block 클래스의 메서드로 구현해야 함
                }
            }
        }
    }

    // 포트 하이라이트 제거
    void clearPortHighlights() {
        // 모든 블록을 순회하면서 하이라이트 제거
        foreach (QGraphicsItem* item, items()) {
            Block* block = dynamic_cast<Block*>(item);
            if (block) {
                // 여기서는 실제 블록 클래스에 포트 하이라이트 제거 기능이 추가되어 있다고 가정
                // 실제 구현에서는 이 부분을 Block 클래스의 메서드로 구현해야 함
            }
        }
    }
private:
    bool m_connectionMode;
    QGraphicsPathItem* m_tempConnection;  // 라인 대신 패스 아이템 사용
    Block* m_sourceBlock;
    int m_sourcePortIndex;
    std::vector<Connection*> m_connections;
};



class SimulationEngine : public QObject {
    Q_OBJECT

public:
    SimulationEngine(SimulationScene* scene, QObject* parent = nullptr)
        : QObject(parent), m_scene(scene), m_running(false), m_time(0.0), m_duration(10.0),
        m_timeStep(0.01), m_playbackSpeed(1.0)   {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &SimulationEngine::step);
    }
    // 현재 시뮬레이션 시간 getter
    double getCurrentTime() const {
        return m_time;
    }
    // 새로운 시뮬레이션 지속 시간 setter/getter 메서드
    void setDuration(double duration) {
        m_duration = std::max(0.1, duration); // 최소 0.1초
    }

    double getDuration() const {
        return m_duration;
    }
    // 시간 간격 getter/setter
    void setTimeStep(double timeStep) {
        m_timeStep = std::max(0.001, std::min(0.1, timeStep)); // 1ms ~ 100ms 제한

        // 타이머 간격 조정 (배속 반영)
        if (m_running) {
            int timerInterval = static_cast<int>(m_timeStep * 1000 / m_playbackSpeed);
            m_timer->setInterval(timerInterval);
        }
    }

    double getTimeStep() const {
        return m_timeStep;
    }
    // 재생 배속 getter/setter
    void setPlaybackSpeed(double speed) {
        m_playbackSpeed = std::max(0.1, std::min(10.0, speed)); // 0.1x ~ 10x 제한

        // 타이머 간격 조정
        if (m_running) {
            int timerInterval = static_cast<int>(m_timeStep * 1000 / m_playbackSpeed);
            m_timer->setInterval(timerInterval);
        }
    }

    double getPlaybackSpeed() const {
        return m_playbackSpeed;
    }

    void start() {
        if (!m_running) {
            m_running = true;
            m_time = 0.0;
            resetBlocks();

            // 타이머 간격 계산 (배속 반영)
            int timerInterval = static_cast<int>(m_timeStep * 1000 / m_playbackSpeed);
            m_timer->setInterval(timerInterval);
            m_timer->start(); // 10ms interval (100 Hz)
            emit started();
        }
    }

    void stop() {
        if (m_running) {
            m_running = false;
            m_timer->stop();
            emit stopped();
        }
    }


    void step() {
        if (!m_running) return;

        // 설정된 지속 시간에 도달하면 시뮬레이션 중지
        if (m_time >= m_duration) {
            stop();
            return;
        }
        // Calculate all block outputs
        std::map<Block*, double> blockOutputs;

        // Find blocks with no inputs (sources and signal generators) and compute their values first
        for (auto item : m_scene->items()) {
            Block* block = dynamic_cast<Block*>(item);
            if (!block) continue;

            // 소스 블록 처리
            if (block->getType() == Block::SOURCE) {
                SourceBlock* sourceBlock = dynamic_cast<SourceBlock*>(block);
                blockOutputs[block] = sourceBlock->compute({});
            }
            // 시그널 제너레이터 계열 블록 처리
            else if (block->getType() == Block::CLOCK ||
                     block->getType() == Block::RAMP ||
                     block->getType() == Block::STEP ||
                     block->getType() == Block::SINE_WAVE) {
                // 시그널 제너레이터는 현재 시뮬레이션 시간을 기반으로 출력값 계산
                blockOutputs[block] = computeSignalGeneratorOutput(block, m_time);
            }
        }

        // Process connections in order (simple approach - might not work for all cases)
        bool progress = true;
        while (progress) {
            progress = false;

            for (auto connection : m_scene->getConnections()) {
                Block* sourceBlock = connection->getSourceBlock();
                Block* destBlock = connection->getDestBlock();

                // If source block output is already calculated
                if (blockOutputs.find(sourceBlock) != blockOutputs.end()) {
                    // Collect all inputs to destination block
                    std::vector<double> inputs;
                    bool allInputsReady = true;

                    // Find all connections to this destination block
                    for (auto conn : m_scene->getConnections()) {
                        if (conn->getDestBlock() == destBlock) {
                            Block* srcBlock = conn->getSourceBlock();
                            if (blockOutputs.find(srcBlock) == blockOutputs.end()) {
                                allInputsReady = false;
                                break;
                            }
                            inputs.push_back(blockOutputs[srcBlock]);
                        }
                    }

                    // If all inputs are ready, compute the output
                    if (allInputsReady && blockOutputs.find(destBlock) == blockOutputs.end()) {
                        blockOutputs[destBlock] = destBlock->compute(inputs);
                        progress = true;
                    }
                }
            }
        }

        // 시간 업데이트 (시간 간격 사용)
        m_time += m_timeStep;

        // Emit results
        emit simulationStepped(m_time, blockOutputs);
    }

    void resetBlocks() {
        // Reset state of blocks that maintain state
        for (auto item : m_scene->items()) {
            Block* block = dynamic_cast<Block*>(item);
            if (block) {
                if (block->getType() == Block::INTEGRATOR) {
                    dynamic_cast<IntegratorBlock*>(block)->resetState();
                } else if (block->getType() == Block::DERIVATIVE) {
                    dynamic_cast<DerivativeBlock*>(block)->resetState();
                } else if (block->getType() == Block::RATE_LIMITER) {
                    dynamic_cast<RateLimiterBlock*>(block)->resetState();
                }
            }
        }
    }

private:
    // 시그널 제너레이터 블록의 출력값 계산 함수
    double computeSignalGeneratorOutput(Block* block, double time) {
        QMap<QString, QVariant> props = block->getProperties();

        switch (block->getType()) {
        case Block::CLOCK: {
            double period = props["period"].toDouble();
            double offset = props["offset"].toDouble();

            if (period <= 0) period = 1.0;  // 기본값 제공

            // 시간 오프셋 적용
            double adjustedTime = time - offset;
            if (adjustedTime < 0) return 0.0;

            // 정규화된 톱니파 (0~1) 반환
            return std::fmod(adjustedTime, period) / period;
        }

        case Block::RAMP: {
            double slope = props["slope"].toDouble();
            double startTime = props["startTime"].toDouble();
            double initialOutput = props["initialOutput"].toDouble();

            if (time < startTime) {
                return initialOutput;
            } else {
                return initialOutput + slope * (time - startTime);
            }
        }

        case Block::STEP: {
            double stepTime = props["stepTime"].toDouble();
            double initialValue = props["initialValue"].toDouble();
            double finalValue = props["finalValue"].toDouble();

            return (time < stepTime) ? initialValue : finalValue;
        }

        case Block::SINE_WAVE: {
            double amplitude = props["amplitude"].toDouble();
            double frequency = props["frequency"].toDouble();
            double phase = props["phase"].toDouble();
            double bias = props["bias"].toDouble();

            return amplitude * std::sin(2 * M_PI * frequency * time + phase) + bias;
        }

        default:
            return 0.0;
        }
    }

signals:
    void started();
    void stopped();
    void simulationStepped(double time, const std::map<Block*, double>& outputs);

private:
    SimulationScene* m_scene;
    QTimer* m_timer;
    bool m_running;
    double m_time;
    double m_duration; // 추가: 시뮬레이션 총 지속 시간
    double m_timeStep;  // 추가: 시뮬레이션 시간 간격
    double m_playbackSpeed; // 추가: 재생 배속
};




class SimulationView : public QGraphicsView {
public:
    SimulationView(QGraphicsScene* scene = nullptr, QWidget* parent = nullptr)
        : QGraphicsView(scene, parent), m_isPanning(false), m_lastMousePos() {
        // 안티알리아싱 활성화
        setRenderHint(QPainter::Antialiasing);
        // 뷰포트 업데이트 모드 설정
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        // 기본 드래그 모드 설정
        setDragMode(QGraphicsView::RubberBandDrag);

        // 마우스 트래킹 활성화 (마우스 이동 이벤트 수신)
        setMouseTracking(true);

        // 휠 이벤트 처리를 위한 설정
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    }

protected:
    // 마우스 버튼을 누를 때
    void mousePressEvent(QMouseEvent* event) override {
        // 마우스 중간 버튼(휠 버튼)을 누르면 패닝 모드 시작
        if (event->button() == Qt::MiddleButton) {
            m_isPanning = true;
            m_lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor); // 손 모양 커서로 변경
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
    }

    // 마우스를 움직일 때
    void mouseMoveEvent(QMouseEvent* event) override {
        // 패닝 모드에서 마우스를 움직이면 뷰 이동
        if (m_isPanning) {
            QPoint delta = event->pos() - m_lastMousePos;
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
            m_lastMousePos = event->pos();
            event->accept();
            return;
        }
        QGraphicsView::mouseMoveEvent(event);
    }

    // 마우스 버튼을 놓을 때
    void mouseReleaseEvent(QMouseEvent* event) override {
        // 마우스 중간 버튼을 놓으면 패닝 모드 종료
        if (event->button() == Qt::MiddleButton) {
            m_isPanning = false;
            setCursor(Qt::ArrowCursor); // 기본 커서로 복원
            event->accept();
            return;
        }
        QGraphicsView::mouseReleaseEvent(event);
    }

    // 휠 이벤트 처리 - 확대/축소 기능 (선택적 추가)
    void wheelEvent(QWheelEvent* event) override {
        // 확대/축소 스케일 팩터
        const double scaleFactor = 1.15;

        // 휠을 위로 굴리면 확대, 아래로 굴리면 축소
        if (event->angleDelta().y() > 0) {
            // 확대
            scale(scaleFactor, scaleFactor);
        } else {
            // 축소
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }

        event->accept();
    }

private:
    bool m_isPanning;
    QPoint m_lastMousePos;
};


#endif // SIMULATION_H
