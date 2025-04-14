#include "block.h"

#include <QMainWindow>

#include "simulation.h"

#include <iostream>

Block::Block(BlockType type, const QString& name, QGraphicsItem* parent)
    : QObject(), QGraphicsItem(parent), m_type(type), m_name(name), m_width(100), m_height(60) {
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);  // 이 플래그가 있는지 확인
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);  // 이 플래그 추가
    setAcceptHoverEvents(true); // 호버 이벤트 활성화
    setZValue(1);

    // 기본 하이라이트 설정
    m_highlightedInputPort = -1;
    m_highlightedOutputPort = -1;

    setupPorts();
}

QRectF Block::boundingRect() const {
    return QRectF(0, 0, m_width, m_height);
}

void Block::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // 블록 배경 및 테두리 그리기
    if (isSelected()) {
        painter->setBrush(QColor(240, 248, 255)); // 선택 시 연한 파란색 배경
        painter->setPen(QPen(Qt::blue, 2));
    } else {
        painter->setBrush(Qt::white);
        painter->setPen(QPen(Qt::black, 1));
    }
    painter->drawRect(boundingRect());

    // 블록 타입과 이름을 구분하는 구분선 그리기
    painter->setPen(Qt::lightGray);
    painter->drawLine(QPointF(2, m_height / 2), QPointF(m_width - 2, m_height / 2));

    // 블록 종류 이름 표시 (상단에)
    painter->setPen(isSelected() ? Qt::blue : Qt::black);
    QFont typeFont = painter->font();
    typeFont.setBold(true);
    painter->setFont(typeFont);

    QString typeName = getTypeDisplayName();  // 헬퍼 메서드 사용

    // 텍스트가 너무 길면 축약
    if (painter->fontMetrics().horizontalAdvance(typeName) > m_width - 10) {
        typeName = painter->fontMetrics().elidedText(typeName, Qt::ElideMiddle, m_width - 10);
    }

    painter->drawText(QRectF(5, 5, m_width - 10, m_height / 2 - 10), Qt::AlignCenter, typeName);

    // 블록 인스턴스 이름 표시 (하단에)
    QFont nameFont = painter->font();
    nameFont.setBold(false);
    nameFont.setItalic(true);
    nameFont.setPointSizeF(nameFont.pointSizeF() * 0.9); // 약간 더 작은 크기로
    painter->setFont(nameFont);

    // 이름이 너무 길면 축약
    QString displayName = m_name;
    if (painter->fontMetrics().horizontalAdvance(displayName) > m_width - 10) {
        displayName = painter->fontMetrics().elidedText(displayName, Qt::ElideMiddle, m_width - 10);
    }

    painter->drawText(QRectF(5, m_height / 2 + 5, m_width - 10, m_height / 2 - 10), Qt::AlignCenter, displayName);

    // 포트 그리기
    drawPorts(painter);
}

void Block::drawPorts(QPainter* painter) {
    // Draw input ports on the left
    for (int i = 0; i < m_inputPorts.size(); ++i) {
        QPointF pos = m_inputPorts[i];

        // 하이라이트된 포트인 경우 다른 색상으로 표시
        if (i == m_highlightedInputPort) {
            painter->setBrush(Qt::red); // 하이라이트 색상
            painter->setPen(QPen(Qt::red, 2));
            painter->drawEllipse(pos, 6, 6); // 약간 더 크게 그리기
        } else {
            painter->setBrush(Qt::black);
            painter->setPen(QPen(Qt::black, 1));
            painter->drawEllipse(pos, 5, 5);
        }
    }

    // Draw output ports on the right
    for (int i = 0; i < m_outputPorts.size(); ++i) {
        QPointF pos = m_outputPorts[i];

        // 하이라이트된 포트인 경우 다른 색상으로 표시
        if (i == m_highlightedOutputPort) {
            painter->setBrush(Qt::red); // 하이라이트 색상
            painter->setPen(QPen(Qt::red, 2));
            painter->drawEllipse(pos, 6, 6); // 약간 더 크게 그리기
        } else {
            painter->setBrush(Qt::black);
            painter->setPen(QPen(Qt::black, 1));
            painter->drawEllipse(pos, 5, 5);
        }
    }
}

void Block::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // Default: 1 input and 1 output port
    if (m_type != SOURCE) {
        m_inputPorts.push_back(QPointF(0, m_height / 2));
    }

    if (m_type != SCOPE) {
        m_outputPorts.push_back(QPointF(m_width, m_height / 2));
    }
}

QPointF Block::getInputPortPos(int index) const {
    if (index >= 0 && index < m_inputPorts.size()) {
        return mapToScene(m_inputPorts[index]);
    }
    return QPointF();
}

QPointF Block::getOutputPortPos(int index) const {
    if (index >= 0 && index < m_outputPorts.size()) {
        return mapToScene(m_outputPorts[index]);
    }
    return QPointF();
}

bool Block::containsInputPort(const QPointF& point, int& portIndex) const {
    for (int i = 0; i < m_inputPorts.size(); ++i) {
        if (QPointF(mapToScene(m_inputPorts[i]) - point).manhattanLength() < 10) {
            portIndex = i;
            return true;
        }
    }
    portIndex = -1;
    return false;
}

bool Block::containsOutputPort(const QPointF& point, int& portIndex) const {
    for (int i = 0; i < m_outputPorts.size(); ++i) {
        if (QPointF(mapToScene(m_outputPorts[i]) - point).manhattanLength() < 10) {
            portIndex = i;
            return true;
        }
    }
    portIndex = -1;
    return false;
}
// 호버 이벤트 처리
void Block::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    QPointF pos = event->pos();

    // 입력 포트 체크
    bool foundPort = false;
    for (int i = 0; i < m_inputPorts.size(); ++i) {
        if (QLineF(pos, m_inputPorts[i]).length() < 10) {
            // 입력 포트 하이라이트
            if (m_highlightedInputPort != i) {
                m_highlightedInputPort = i;
                m_highlightedOutputPort = -1; // 출력 포트 하이라이트 해제
                update(); // 다시 그리기

                // 커서 변경 및 툴팁 표시
                setCursor(Qt::CrossCursor);
                setToolTip(QString("Input port %1 - Drag connections here").arg(i+1));
            }
            foundPort = true;
            break;
        }
    }

    // 입력 포트에 없으면 출력 포트 체크
    if (!foundPort) {
        for (int i = 0; i < m_outputPorts.size(); ++i) {
            if (QLineF(pos, m_outputPorts[i]).length() < 10) {
                // 출력 포트 하이라이트
                if (m_highlightedOutputPort != i) {
                    m_highlightedOutputPort = i;
                    m_highlightedInputPort = -1; // 입력 포트 하이라이트 해제
                    update(); // 다시 그리기

                    // 커서 변경 및 툴팁 표시
                    setCursor(Qt::CrossCursor);
                    setToolTip(QString("Output port %1 - Drag from here to connect").arg(i+1));
                }
                foundPort = true;
                break;
            }
        }
    }

    // 포트 위에 없으면 하이라이트 해제
    if (!foundPort && (m_highlightedInputPort != -1 || m_highlightedOutputPort != -1)) {
        m_highlightedInputPort = -1;
        m_highlightedOutputPort = -1;
        update(); // 다시 그리기

        // 커서 및 툴팁 초기화
        unsetCursor();
        setToolTip("");
    }

    QGraphicsItem::hoverMoveEvent(event);
}

// 호버 나갈 때 하이라이트 해제
void Block::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    if (m_highlightedInputPort != -1 || m_highlightedOutputPort != -1) {
        m_highlightedInputPort = -1;
        m_highlightedOutputPort = -1;
        update(); // 다시 그리기

        // 커서 및 툴팁 초기화
        unsetCursor();
        setToolTip("");
    }

    QGraphicsItem::hoverLeaveEvent(event);
}
double Block::compute(const std::vector<double>& inputs) const {
    // Default implementation (should be overridden by derived classes)
    return 0.0;
}

Block::BlockType Block::getType() const { return m_type; }
QString Block::getName() const { return m_name; }
void Block::setName(const QString& name) { m_name = name; }

int Block::getInputPortCount() const { return m_inputPorts.size(); }
int Block::getOutputPortCount() const { return m_outputPorts.size(); }

// Properties specific to the block
void Block::setProperties(const QMap<QString, QVariant>& properties) {
    m_properties = properties;
}

QMap<QString, QVariant> Block::getProperties() const {
    return m_properties;
}

QVariant Block::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene()) {
        // 블록 위치가 변경될 때 신호 발생
        QGraphicsScene* s = scene();
        SimulationScene* simScene = qobject_cast<SimulationScene*>(s);
        if (simScene) {
            simScene->updateConnections();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}











SourceBlock::SourceBlock(const QString& name, QGraphicsItem* parent)
    : Block(SOURCE, name, parent) {
    setupPorts();
    m_properties["value"] = 1.0;
}

void SourceBlock::setupPorts()  {
    m_inputPorts.clear();
    m_outputPorts.clear();
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

double SourceBlock::compute(const std::vector<double>& inputs) const  {
    Q_UNUSED(inputs);
    std::cout << "source" << std::endl;
    return m_properties["value"].toDouble();
}



GainBlock::GainBlock(const QString& name, QGraphicsItem* parent)
    : Block(GAIN, name, parent) {
    setupPorts();
    m_properties["gain"] = 1.0;
}

double GainBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() > 0) {
        return inputs[0] * m_properties["gain"].toDouble();
    }
    return 0.0;
}



SumBlock::SumBlock(const QString& name, QGraphicsItem* parent)
    : Block(SUM, name, parent) {
    setupPorts();
}

void SumBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // Two input ports
    m_inputPorts.push_back(QPointF(0, m_height / 3));
    m_inputPorts.push_back(QPointF(0, 2 * m_height / 3));

    // One output port
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void SumBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // Draw a plus sign
    painter->drawText(boundingRect(), Qt::AlignCenter, "+");
}

double SumBlock::compute(const std::vector<double>& inputs) const {
    double sum = 0.0;
    for (const auto& input : inputs) {
        sum += input;
    }
    return sum;
}



ProductBlock::ProductBlock(const QString& name, QGraphicsItem* parent)
    : Block(PRODUCT, name, parent) {
    setupPorts();
}

void ProductBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // Two input ports
    m_inputPorts.push_back(QPointF(0, m_height / 3));
    m_inputPorts.push_back(QPointF(0, 2 * m_height / 3));

    // One output port
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void ProductBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // Draw a multiplication sign
    painter->drawText(boundingRect(), Qt::AlignCenter, "×");
}

double ProductBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 0.0;

    double product = 1.0;
    for (const auto& input : inputs) {
        product *= input;
    }
    return product;
}



IntegratorBlock::IntegratorBlock(const QString& name, QGraphicsItem* parent)
    : Block(INTEGRATOR, name, parent), m_accumulator(0.0) {
    setupPorts();
    m_properties["initial"] = 0.0;
}

double IntegratorBlock::compute(const std::vector<double>& inputs) const {
    // In a real implementation, we would need to track time and perform proper integration
    // This is a simplified version for demonstration
    if (inputs.size() > 0) {
        // Assume a fixed time step of 0.01 for simplicity
        const double dt = 0.01;
        m_accumulator += inputs[0] * dt;
        return m_accumulator;
    }
    return m_accumulator;
}

void IntegratorBlock::resetState() {
    m_accumulator = m_properties["initial"].toDouble();
}




DerivativeBlock::DerivativeBlock(const QString& name, QGraphicsItem* parent)
    : Block(DERIVATIVE, name, parent), m_lastInput(0.0) {
    setupPorts();
}

double DerivativeBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() > 0) {
        // Assume a fixed time step of 0.01 for simplicity
        const double dt = 0.01;
        double derivative = (inputs[0] - m_lastInput) / dt;
        m_lastInput = inputs[0];
        return derivative;
    }
    return 0.0;
}

void DerivativeBlock::resetState() {
    m_lastInput = 0.0;
}




ScopeBlock::ScopeBlock(const QString& name, QGraphicsItem* parent)
    : Block(SCOPE, name, parent) {
    setupPorts();
    m_width = 120;
    m_height = 80;
}

void ScopeBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // One input port, no output ports
    m_inputPorts.push_back(QPointF(0, m_height / 2));
}

void ScopeBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // Draw a simple waveform as an icon
    QPainterPath path;
    path.moveTo(20, m_height / 2);
    path.lineTo(30, m_height / 2 - 10);
    path.lineTo(40, m_height / 2 + 10);
    path.lineTo(50, m_height / 2 - 10);
    path.lineTo(60, m_height / 2 + 10);
    path.lineTo(70, m_height / 2 - 10);
    path.lineTo(80, m_height / 2);

    painter->setPen(QPen(Qt::blue, 1.5));
    painter->drawPath(path);
}

double ScopeBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() > 0) {
        // In a real implementation, we would store the values for visualization
        m_values.push_back(inputs[0]);
        if (m_values.size() > 1000) {
            m_values.erase(m_values.begin());
        }
        return inputs[0];
    }
    return 0.0;
}

const std::vector<double>& ScopeBlock::getValues() const {
    return m_values;
}




InBlock::InBlock(const QString& name, QGraphicsItem* parent)
    : Block(IN, name, parent) {
    setupPorts();
    m_properties["variable"] = "input"; // 기본 변수 이름
}

void InBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // In 블록은 출력 포트만 가짐
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void InBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // 특별한 시각적 표시: >
    painter->setPen(QPen(Qt::blue, 2));
    QPointF points[3] = {
        QPointF(m_width/4, m_height/4),
        QPointF(m_width*3/4, m_height/2),
        QPointF(m_width/4, m_height*3/4)
    };
    painter->drawPolyline(points, 3);
}

double InBlock::compute(const std::vector<double>& inputs) const {
    // 입력은 외부 값으로, 속성에서 실제 값을 가져옴
    return m_properties["value"].toDouble();
}




OutBlock::OutBlock(const QString& name, QGraphicsItem* parent)
    : Block(OUT, name, parent) {
    setupPorts();
    m_properties["variable"] = "output"; // 기본 변수 이름
}

void OutBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // Out 블록은 입력 포트만 가짐
    m_inputPorts.push_back(QPointF(0, m_height / 2));
}

void OutBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // 특별한 시각적 표시: <
    painter->setPen(QPen(Qt::blue, 2));
    QPointF points[3] = {
        QPointF(m_width*3/4, m_height/4),
        QPointF(m_width/4, m_height/2),
        QPointF(m_width*3/4, m_height*3/4)
    };
    painter->drawPolyline(points, 3);
}

double OutBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() > 0) {
        // Output 블록은 입력값을 그대로 반환
        return inputs[0];
    }
    return 0.0;
}




LookupTable1DBlock::LookupTable1DBlock(const QString& name, QGraphicsItem* parent)
    : Block(LOOKUP_TABLE_1D, name, parent) {
    // 블록 크기 설정
    m_width = 120;
    m_height = 80;

    setupPorts();

    // 기본 속성 설정
    m_properties["xData"] = "0,1,2,3,4,5"; // x 좌표값 (쉼표로 구분)
    m_properties["yData"] = "0,2,4,6,8,10"; // y 좌표값 (쉼표로 구분)
    m_properties["interpolation"] = "linear"; // 보간 방법 (linear, nearest)
    m_properties["extrapolation"] = "linear"; // 외삽 방법 (linear, nearest, constant)
}

void LookupTable1DBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 1개의 입력 포트 (x 값 입력)
    m_inputPorts.push_back(QPointF(0, m_height / 2));

    // 1개의 출력 포트 (y 값 출력)
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void LookupTable1DBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // 1D Lookup Table 아이콘 그리기
    painter->setPen(QPen(Qt::blue, 1.5));

    // 테이블 형태의 아이콘
    QRectF tableRect(20, 20, m_width - 40, m_height - 40);
    painter->drawRect(tableRect);

    // 그리드 라인
    painter->setPen(QPen(Qt::gray, 0.5));
    for (int i = 1; i < 3; i++) {
        double x = tableRect.left() + (tableRect.width() * i / 3);
        double y = tableRect.top() + (tableRect.height() * i / 2);

        painter->drawLine(QPointF(x, tableRect.top()), QPointF(x, tableRect.bottom()));
        painter->drawLine(QPointF(tableRect.left(), y), QPointF(tableRect.right(), y));
    }

    // 함수 그래프 (선형 증가 커브)
    painter->setPen(QPen(Qt::red, 1.5));
    QPainterPath path;
    path.moveTo(tableRect.left(), tableRect.bottom() - 5);
    path.lineTo(tableRect.right() - 10, tableRect.top() + 5);
    painter->drawPath(path);

    // 텍스트 추가
    painter->setPen(Qt::black);
    painter->setFont(QFont("Arial", 7));
    painter->drawText(QRectF(20, 5, m_width - 40, 15), Qt::AlignCenter, "1D Lookup");
}

double LookupTable1DBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 0.0;

    double x = inputs[0];

    // 테이블 데이터 파싱
    QStringList xDataStr = m_properties["xData"].toString().split(',');
    QStringList yDataStr = m_properties["yData"].toString().split(',');

    // 데이터 개수가 맞지 않으면 오류
    if (xDataStr.size() != yDataStr.size() || xDataStr.isEmpty()) {
        return 0.0;
    }

    // x, y 데이터 벡터로 변환
    std::vector<double> xData;
    std::vector<double> yData;

    for (int i = 0; i < xDataStr.size(); i++) {
        xData.push_back(xDataStr[i].toDouble());
        yData.push_back(yDataStr[i].toDouble());
    }

    // 값이 범위를 벗어나는 경우
    if (x <= xData.front()) {
        // 외삽 방법: constant
        if (m_properties["extrapolation"].toString() == "constant") {
            return yData.front();
        }
        // 외삽 방법: nearest
        else if (m_properties["extrapolation"].toString() == "nearest") {
            return yData.front();
        }
        // 외삽 방법: linear (기본값)
        else {
            if (xData.size() < 2) return yData.front();

            double slope = (yData[1] - yData[0]) / (xData[1] - xData[0]);
            return yData.front() + slope * (x - xData.front());
        }
    }
    else if (x >= xData.back()) {
        // 외삽 방법: constant
        if (m_properties["extrapolation"].toString() == "constant") {
            return yData.back();
        }
        // 외삽 방법: nearest
        else if (m_properties["extrapolation"].toString() == "nearest") {
            return yData.back();
        }
        // 외삽 방법: linear (기본값)
        else {
            if (xData.size() < 2) return yData.back();

            int last = xData.size() - 1;
            double slope = (yData[last] - yData[last-1]) / (xData[last] - xData[last-1]);
            return yData.back() + slope * (x - xData.back());
        }
    }

    // 테이블 내 값 찾기
    for (size_t i = 0; i < xData.size() - 1; i++) {
        if (x >= xData[i] && x <= xData[i+1]) {
            // 보간 방법: nearest
            if (m_properties["interpolation"].toString() == "nearest") {
                if (std::abs(x - xData[i]) < std::abs(x - xData[i+1])) {
                    return yData[i];
                } else {
                    return yData[i+1];
                }
            }
            // 보간 방법: linear (기본값)
            else {
                double t = (x - xData[i]) / (xData[i+1] - xData[i]);
                return yData[i] * (1 - t) + yData[i+1] * t;
            }
        }
    }

    // 여기까지 오면 뭔가 잘못된 것
    return 0.0;
}


LookupTable2DBlock::LookupTable2DBlock(const QString& name, QGraphicsItem* parent)
    : Block(LOOKUP_TABLE_2D, name, parent) {
    // 블록 크기 설정
    m_width = 120;
    m_height = 100;

    setupPorts();

    // 기본 속성 설정
    m_properties["xData"] = "0,1,2,3"; // x 좌표값 (쉼표로 구분)
    m_properties["yData"] = "0,1,2,3"; // y 좌표값 (쉼표로 구분)
    m_properties["zData"] = "0,1,2,3, 1,2,3,4, 2,3,4,5, 3,4,5,6"; // z 좌표값 (행 우선, 쉼표로 구분)
    m_properties["interpolation"] = "linear"; // 보간 방법 (linear, nearest)
    m_properties["extrapolation"] = "linear"; // 외삽 방법 (linear, nearest, constant)
}

void LookupTable2DBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 2개의 입력 포트 (x, y 값 입력)
    m_inputPorts.push_back(QPointF(0, m_height / 3));
    m_inputPorts.push_back(QPointF(0, 2 * m_height / 3));

    // 1개의 출력 포트 (z 값 출력)
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void LookupTable2DBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // 2D Lookup Table 아이콘 그리기
    painter->setPen(QPen(Qt::blue, 1.5));

    // 3D 표면 형태의 아이콘
    QRectF tableRect(20, 20, m_width - 40, m_height - 40);

    // 표면 그리기 (간단한 3D 육면체)
    QPainterPath path;

    // 상단면
    path.moveTo(tableRect.left() + 10, tableRect.top() + 10);
    path.lineTo(tableRect.right() - 10, tableRect.top() + 10);
    path.lineTo(tableRect.right(), tableRect.top());
    path.lineTo(tableRect.left() + 20, tableRect.top());
    path.closeSubpath();

    // 앞면
    path.moveTo(tableRect.left() + 10, tableRect.top() + 10);
    path.lineTo(tableRect.left() + 10, tableRect.bottom() - 10);
    path.lineTo(tableRect.right() - 10, tableRect.bottom() - 10);
    path.lineTo(tableRect.right() - 10, tableRect.top() + 10);

    // 측면
    path.moveTo(tableRect.right() - 10, tableRect.top() + 10);
    path.lineTo(tableRect.right(), tableRect.top());
    path.lineTo(tableRect.right(), tableRect.bottom() - 20);
    path.lineTo(tableRect.right() - 10, tableRect.bottom() - 10);

    painter->setBrush(QColor(200, 220, 255, 100));
    painter->drawPath(path);

    // 그리드 라인
    painter->setPen(QPen(Qt::gray, 0.5));
    painter->drawLine(tableRect.left() + 10, tableRect.top() + 10 + (tableRect.height() - 20) / 2,
                      tableRect.right() - 10, tableRect.top() + 10 + (tableRect.height() - 20) / 2);
    painter->drawLine(tableRect.left() + 10 + (tableRect.width() - 20) / 2, tableRect.top() + 10,
                      tableRect.left() + 10 + (tableRect.width() - 20) / 2, tableRect.bottom() - 10);

    // 텍스트 추가
    painter->setPen(Qt::black);
    painter->setFont(QFont("Arial", 7));
    painter->drawText(QRectF(20, 5, m_width - 40, 15), Qt::AlignCenter, "2D Lookup");

    // 포트 라벨링
    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height/3 - 10, 15, 20), Qt::AlignLeft, "x");
    painter->drawText(QRectF(5, 2*m_height/3 - 10, 15, 20), Qt::AlignLeft, "y");
    painter->drawText(QRectF(m_width - 20, m_height/2 - 10, 15, 20), Qt::AlignRight, "z");
}

double LookupTable2DBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() < 2) return 0.0;

    double x = inputs[0];
    double y = inputs[1];

    // 테이블 데이터 파싱
    QStringList xDataStr = m_properties["xData"].toString().split(',');
    QStringList yDataStr = m_properties["yData"].toString().split(',');
    QStringList zDataStr = m_properties["zData"].toString().split(',');

    // 데이터 개수 확인
    int xSize = xDataStr.size();
    int ySize = yDataStr.size();

    if (xSize == 0 || ySize == 0 || zDataStr.size() != xSize * ySize) {
        return 0.0;
    }

    // x, y, z 데이터 벡터로 변환
    std::vector<double> xData(xSize);
    std::vector<double> yData(ySize);
    std::vector<std::vector<double>> zData(ySize, std::vector<double>(xSize));

    for (int i = 0; i < xSize; i++) {
        xData[i] = xDataStr[i].toDouble();
    }

    for (int i = 0; i < ySize; i++) {
        yData[i] = yDataStr[i].toDouble();
    }

    // Z 데이터는 행 우선으로 저장 (y, x 순서)
    for (int i = 0; i < ySize; i++) {
        for (int j = 0; j < xSize; j++) {
            int index = i * xSize + j;
            if (index < zDataStr.size()) {
                zData[i][j] = zDataStr[index].toDouble();
            }
        }
    }

    // 보간 방법 선택
    QString interpolation = m_properties["interpolation"].toString();
    QString extrapolation = m_properties["extrapolation"].toString();

    // X, Y가 범위를 벗어나는지 확인
    bool xOutOfRange = (x < xData.front() || x > xData.back());
    bool yOutOfRange = (y < yData.front() || y > yData.back());

    // 범위를 벗어나면 외삽법 사용
    if (xOutOfRange || yOutOfRange) {
        // 외삽법: constant - 가장 가까운 경계값 반환
        if (extrapolation == "constant") {
            int xi = (x < xData.front()) ? 0 : (x > xData.back() ? xSize - 1 : 0);
            int yi = (y < yData.front()) ? 0 : (y > yData.back() ? ySize - 1 : 0);
            return zData[yi][xi];
        }
        // 외삽법: nearest - 가장 가까운 점 사용
        else if (extrapolation == "nearest") {
            int xi = 0;
            if (x < xData.front()) xi = 0;
            else if (x > xData.back()) xi = xSize - 1;
            else {
                for (int i = 0; i < xSize - 1; i++) {
                    if (x >= xData[i] && x <= xData[i+1]) {
                        xi = (x - xData[i] < xData[i+1] - x) ? i : i + 1;
                        break;
                    }
                }
            }

            int yi = 0;
            if (y < yData.front()) yi = 0;
            else if (y > yData.back()) yi = ySize - 1;
            else {
                for (int i = 0; i < ySize - 1; i++) {
                    if (y >= yData[i] && y <= yData[i+1]) {
                        yi = (y - yData[i] < yData[i+1] - y) ? i : i + 1;
                        break;
                    }
                }
            }

            return zData[yi][xi];
        }
        // 외삽법: linear - 선형 외삽 (기본)
        else {
            // x에 대한 외삽
            int xi = 0;
            int xi1 = 1;
            if (x < xData.front()) {
                xi = 0;
                xi1 = 1;
            } else if (x > xData.back()) {
                xi = xSize - 2;
                xi1 = xSize - 1;
            } else {
                for (int i = 0; i < xSize - 1; i++) {
                    if (x >= xData[i] && x <= xData[i+1]) {
                        xi = i;
                        xi1 = i + 1;
                        break;
                    }
                }
            }

            // y에 대한 외삽
            int yi = 0;
            int yi1 = 1;
            if (y < yData.front()) {
                yi = 0;
                yi1 = 1;
            } else if (y > yData.back()) {
                yi = ySize - 2;
                yi1 = ySize - 1;
            } else {
                for (int i = 0; i < ySize - 1; i++) {
                    if (y >= yData[i] && y <= yData[i+1]) {
                        yi = i;
                        yi1 = i + 1;
                        break;
                    }
                }
            }

            // 양선형 보간
            double x1 = xData[xi];
            double x2 = xData[xi1];
            double y1 = yData[yi];
            double y2 = yData[yi1];

            double q11 = zData[yi][xi];
            double q12 = zData[yi1][xi];
            double q21 = zData[yi][xi1];
            double q22 = zData[yi1][xi1];

            double dx = x2 - x1;
            double dy = y2 - y1;

            if (dx != 0 && dy != 0) {
                double tx = (x - x1) / dx;
                double ty = (y - y1) / dy;

                double r1 = q11 * (1 - tx) + q21 * tx;
                double r2 = q12 * (1 - tx) + q22 * tx;

                return r1 * (1 - ty) + r2 * ty;
            } else {
                return q11;
            }
        }
    }

    // 테이블 내 보간 (x, y가 범위 내에 있음)
    // x의 인덱스 찾기
    int xi = 0;
    for (int i = 0; i < xSize - 1; i++) {
        if (x >= xData[i] && x <= xData[i+1]) {
            xi = i;
            break;
        }
    }

    // y의 인덱스 찾기
    int yi = 0;
    for (int i = 0; i < ySize - 1; i++) {
        if (y >= yData[i] && y <= yData[i+1]) {
            yi = i;
            break;
        }
    }

    // 보간법: nearest
    if (interpolation == "nearest") {
        int nearestXi = (x - xData[xi] < xData[xi+1] - x) ? xi : xi + 1;
        int nearestYi = (y - yData[yi] < yData[yi+1] - y) ? yi : yi + 1;
        return zData[nearestYi][nearestXi];
    }
    // 보간법: linear (기본)
    else {
        // 양선형 보간 (Bilinear interpolation)
        double x1 = xData[xi];
        double x2 = xData[xi+1];
        double y1 = yData[yi];
        double y2 = yData[yi+1];

        double q11 = zData[yi][xi];
        double q12 = zData[yi+1][xi];
        double q21 = zData[yi][xi+1];
        double q22 = zData[yi+1][xi+1];

        double tx = (x - x1) / (x2 - x1);
        double ty = (y - y1) / (y2 - y1);

        double r1 = q11 * (1 - tx) + q21 * tx;
        double r2 = q12 * (1 - tx) + q22 * tx;

        return r1 * (1 - ty) + r2 * ty;
    }
}


MinBlock::MinBlock(const QString& name, QGraphicsItem* parent)
    : Block(MIN, name, parent) {
    setupPorts();
}

void MinBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 두 개의 입력 포트
    m_inputPorts.push_back(QPointF(0, m_height / 3));
    m_inputPorts.push_back(QPointF(0, 2 * m_height / 3));

    // 하나의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void MinBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)  {
    Block::paint(painter, option, widget);

    // Min 표시
    painter->setPen(QPen(Qt::blue, 1.5));
    painter->setFont(QFont("Arial", 10, QFont::Bold));
    painter->drawText(boundingRect(), Qt::AlignCenter, "MIN");

    // 수식 표시
    painter->setFont(QFont("Arial", 7));
    painter->drawText(QRectF(10, m_height - 20, m_width - 20, 15), Qt::AlignCenter, "y = min(u1, u2)");
}

double MinBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() < 2) return 0.0;

    return std::min(inputs[0], inputs[1]);
}



MaxBlock::MaxBlock(const QString& name, QGraphicsItem* parent)
    : Block(MAX, name, parent) {
    setupPorts();
}

void MaxBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 두 개의 입력 포트
    m_inputPorts.push_back(QPointF(0, m_height / 3));
    m_inputPorts.push_back(QPointF(0, 2 * m_height / 3));

    // 하나의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void MaxBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // Max 표시
    painter->setPen(QPen(Qt::red, 1.5));
    painter->setFont(QFont("Arial", 10, QFont::Bold));
    painter->drawText(boundingRect(), Qt::AlignCenter, "MAX");

    // 수식 표시
    painter->setFont(QFont("Arial", 7));
    painter->drawText(QRectF(10, m_height - 20, m_width - 20, 15), Qt::AlignCenter, "y = max(u1, u2)");
}

double MaxBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() < 2) return 0.0;

    return std::max(inputs[0], inputs[1]);
}



SaturationBlock::SaturationBlock(const QString& name, QGraphicsItem* parent)
    : Block(SATURATION, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["upperLimit"] = 1.0;  // 상한값
    m_properties["lowerLimit"] = -1.0; // 하한값
}

void SaturationBlock::setupPorts()  {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 한 개의 입력 포트
    m_inputPorts.push_back(QPointF(0, m_height / 2));

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void SaturationBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // Saturation 표시
    painter->setPen(QPen(Qt::darkGreen, 1.5));
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "Saturation");

    // 포화 함수 그래프 그리기
    painter->setPen(QPen(Qt::darkGreen, 1.5));

    double upperLimit = m_properties["upperLimit"].toDouble();
    double lowerLimit = m_properties["lowerLimit"].toDouble();

    // 그래프 영역
    QRectF graphRect(m_width * 0.2, m_height * 0.3, m_width * 0.6, m_height * 0.5);

    // 기준선 (x, y축)
    painter->setPen(QPen(Qt::gray, 0.5));
    double centerX = graphRect.center().x();
    double centerY = graphRect.center().y();
    painter->drawLine(QPointF(graphRect.left(), centerY), QPointF(graphRect.right(), centerY));
    painter->drawLine(QPointF(centerX, graphRect.top()), QPointF(centerX, graphRect.bottom()));

    // 포화 함수 그래프
    painter->setPen(QPen(Qt::darkGreen, 1.5));
    QPainterPath path;

    // 왼쪽 포화 영역
    path.moveTo(graphRect.left(), centerY + graphRect.height() * 0.25); // 하한값 높이
    path.lineTo(centerX - graphRect.width() * 0.2, centerY + graphRect.height() * 0.25);

    // 선형 영역
    path.lineTo(centerX + graphRect.width() * 0.2, centerY - graphRect.height() * 0.25);

    // 오른쪽 포화 영역
    path.lineTo(graphRect.right(), centerY - graphRect.height() * 0.25); // 상한값 높이

    painter->drawPath(path);

    // 제한값 표시
    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 20, m_width - 10, 15), Qt::AlignCenter,
                      QString("Limits: [%1, %2]").arg(lowerLimit).arg(upperLimit));
}

double SaturationBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 0.0;

    double input = inputs[0];
    double upperLimit = m_properties["upperLimit"].toDouble();
    double lowerLimit = m_properties["lowerLimit"].toDouble();

    if (input > upperLimit) return upperLimit;
    if (input < lowerLimit) return lowerLimit;
    return input;
}



RateLimiterBlock::RateLimiterBlock(const QString& name, QGraphicsItem* parent)
    : Block(RATE_LIMITER, name, parent), m_prevOutput(0.0), m_prevTime(0.0), m_initialized(false) {
    setupPorts();

    // 기본 속성 설정
    m_properties["risingLimit"] = 1.0;  // 상승률 제한
    m_properties["fallingLimit"] = -1.0; // 하강률 제한
    m_properties["sampleTime"] = 0.01;  // 샘플 시간
}

void RateLimiterBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 한 개의 입력 포트
    m_inputPorts.push_back(QPointF(0, m_height / 2));

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void RateLimiterBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // Rate Limiter 표시
    painter->setPen(QPen(Qt::darkMagenta, 1.5));
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "Rate Limiter");

    // 변화율 제한 그래프 그리기
    painter->setPen(QPen(Qt::darkMagenta, 1.5));

    // 그래프 영역
    QRectF graphRect(m_width * 0.2, m_height * 0.3, m_width * 0.6, m_height * 0.5);

    // 기준선 (x 축)
    painter->setPen(QPen(Qt::gray, 0.5));
    double centerY = graphRect.center().y();
    painter->drawLine(QPointF(graphRect.left(), centerY), QPointF(graphRect.right(), centerY));

    // 변화율 제한 그래프
    painter->setPen(QPen(Qt::darkMagenta, 1.5));
    QPainterPath path;

    // 계단 형태의 그래프로 변화율 제한 표현
    path.moveTo(graphRect.left(), centerY);

    // 첫 번째 단계 (느린 상승)
    path.lineTo(graphRect.left() + graphRect.width() * 0.15, centerY);
    path.lineTo(graphRect.left() + graphRect.width() * 0.15, centerY - graphRect.height() * 0.15);

    // 두 번째 단계 (빠른 상승 - 제한 없음)
    path.lineTo(graphRect.left() + graphRect.width() * 0.3, centerY - graphRect.height() * 0.15);
    path.lineTo(graphRect.left() + graphRect.width() * 0.3, centerY - graphRect.height() * 0.35);

    // 세 번째 단계 (일정)
    path.lineTo(graphRect.left() + graphRect.width() * 0.5, centerY - graphRect.height() * 0.35);

    // 네 번째 단계 (느린 하강)
    path.lineTo(graphRect.left() + graphRect.width() * 0.7, centerY - graphRect.height() * 0.35);
    path.lineTo(graphRect.left() + graphRect.width() * 0.7, centerY - graphRect.height() * 0.15);

    // 다섯 번째 단계 (느린 하강)
    path.lineTo(graphRect.left() + graphRect.width() * 0.85, centerY - graphRect.height() * 0.15);
    path.lineTo(graphRect.left() + graphRect.width() * 0.85, centerY);

    // 마지막 단계
    path.lineTo(graphRect.right(), centerY);

    painter->drawPath(path);

    // 제한값 표시
    double rising = m_properties["risingLimit"].toDouble();
    double falling = m_properties["fallingLimit"].toDouble();

    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 20, m_width - 10, 15), Qt::AlignCenter,
                      QString("Rise/Fall: [%1, %2]").arg(rising).arg(falling));
}

double RateLimiterBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 0.0;

    double input = inputs[0];
    double risingLimit = m_properties["risingLimit"].toDouble();
    double fallingLimit = m_properties["fallingLimit"].toDouble();
    double sampleTime = m_properties["sampleTime"].toDouble();

    // 첫 실행 시 초기화
    if (!m_initialized) {
        m_prevOutput = input;
        m_prevTime = 0.0;
        m_initialized = true;
        return input;
    }

    // 현재 시간 (샘플링 간격으로 증가)
    double currentTime = m_prevTime + sampleTime;

    // 입력 변화율 계산
    double rate = (input - m_prevOutput) / sampleTime;

    // 변화율 제한 적용
    if (rate > risingLimit) {
        rate = risingLimit;
    } else if (rate < fallingLimit) {
        rate = fallingLimit;
    }

    // 제한된 변화율로 출력 계산
    double output = m_prevOutput + rate * sampleTime;

    // 상태 업데이트
    m_prevOutput = output;
    m_prevTime = currentTime;

    return output;
}

void RateLimiterBlock::resetState() {
    m_initialized = false;
    m_prevOutput = 0.0;
    m_prevTime = 0.0;
}



SwitchBlock::SwitchBlock(const QString& name, QGraphicsItem* parent)
    : Block(SWITCH, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["threshold"] = 0.0;      // 임계값
    m_properties["operator"] = ">=";      // 비교 연산자 (>=, >, ==, !=, <, <=)
    m_properties["passFirstInput"] = true; // 조건 충족 시 첫 번째 입력 통과 여부
}

void SwitchBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 세 개의 입력 포트 (데이터1, 조건, 데이터2)
    m_inputPorts.push_back(QPointF(0, m_height / 4));        // 첫 번째 입력
    m_inputPorts.push_back(QPointF(0, m_height / 2));        // 조건 입력
    m_inputPorts.push_back(QPointF(0, 3 * m_height / 4));    // 두 번째 입력

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void SwitchBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)  {
    Block::paint(painter, option, widget);

    // Switch 표시
    painter->setPen(QPen(Qt::darkCyan, 1.5));
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "Switch");

    // 스위치 아이콘 그리기
    painter->setPen(QPen(Qt::darkCyan, 1.5));

    // 삼각형 스위치 기호
    QPointF triangle[3] = {
        QPointF(m_width * 0.3, m_height * 0.3),
        QPointF(m_width * 0.7, m_height * 0.5),
        QPointF(m_width * 0.3, m_height * 0.7)
    };

    // 삼각형 채우기
    painter->setBrush(QColor(200, 230, 250, 120));
    painter->drawPolygon(triangle, 3);

    // 입력 표시
    painter->setFont(QFont("Arial", 7));
    painter->setPen(Qt::black);
    painter->drawText(QRectF(5, m_height / 4 - 12, 30, 20), Qt::AlignLeft, "u1");
    painter->drawText(QRectF(5, m_height / 2 - 12, 30, 20), Qt::AlignLeft, "c");
    painter->drawText(QRectF(5, 3 * m_height / 4 - 12, 30, 20), Qt::AlignLeft, "u2");

    // 조건 표시
    QString op = m_properties["operator"].toString();
    QString passFirst = m_properties["passFirstInput"].toBool() ? "u1 : u2" : "u2 : u1";
    double threshold = m_properties["threshold"].toDouble();

    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 20, m_width - 10, 15), Qt::AlignCenter,
                      QString("if c %1 %2 then %3").arg(op).arg(threshold).arg(passFirst));
}

double SwitchBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() < 3) return 0.0;

    double input1 = inputs[0];
    double condition = inputs[1];
    double input2 = inputs[2];
    double threshold = m_properties["threshold"].toDouble();
    QString op = m_properties["operator"].toString();
    bool passFirst = m_properties["passFirstInput"].toBool();

    bool conditionMet = false;

    // 조건 평가
    if (op == ">=") conditionMet = (condition >= threshold);
    else if (op == ">") conditionMet = (condition > threshold);
    else if (op == "==") conditionMet = (std::abs(condition - threshold) < 1e-10);
    else if (op == "!=") conditionMet = (std::abs(condition - threshold) >= 1e-10);
    else if (op == "<") conditionMet = (condition < threshold);
    else if (op == "<=") conditionMet = (condition <= threshold);

    // 조건에 따라 출력 선택
    if (conditionMet) {
        return passFirst ? input1 : input2;
    } else {
        return passFirst ? input2 : input1;
    }
}



DivideBlock::DivideBlock(const QString& name, QGraphicsItem* parent)
    : Block(DIVIDE, name, parent) {
    setupPorts();
}

void DivideBlock::setupPorts()  {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 두 개의 입력 포트 (분자, 분모)
    m_inputPorts.push_back(QPointF(0, m_height / 3));
    m_inputPorts.push_back(QPointF(0, 2 * m_height / 3));

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void DivideBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // Divide 표시
    painter->setPen(QPen(Qt::darkRed, 1.5));
    painter->setFont(QFont("Arial", 10, QFont::Bold));
    painter->drawText(boundingRect(), Qt::AlignCenter, "/");

    // 입력 표시
    painter->setFont(QFont("Arial", 7));
    painter->setPen(Qt::black);
    painter->drawText(QRectF(5, m_height / 3 - 12, 30, 20), Qt::AlignLeft, "num");
    painter->drawText(QRectF(5, 2 * m_height / 3 - 12, 30, 20), Qt::AlignLeft, "den");

    // 수식 표시
    painter->setFont(QFont("Arial", 7));
    painter->drawText(QRectF(10, m_height - 20, m_width - 20, 15), Qt::AlignCenter, "y = num / den");
}

double DivideBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.size() < 2) return 0.0;

    double numerator = inputs[0];
    double denominator = inputs[1];

    // 0으로 나누기 방지
    if (std::abs(denominator) < 1e-10) {
        // 0으로 나누면 매우 큰 값 반환 (또는 오류 로깅이 가능하다면 오류 처리)
        return (numerator >= 0) ? 1e10 : -1e10;
    }

    return numerator / denominator;
}


AndBlock::AndBlock(const QString& name, QGraphicsItem* parent)
    : Block(AND, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["inputCount"] = 2;   // 입력 포트 개수
    m_properties["threshold"] = 0.5;  // 1로 판단할 임계값
}

void AndBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 속성에서 입력 포트 개수 가져오기
    int inputCount = m_properties["inputCount"].toInt();
    inputCount = std::max(2, std::min(inputCount, 5)); // 2~5개로 제한

    // 입력 포트 생성
    double step = m_height / (inputCount + 1);
    for (int i = 0; i < inputCount; i++) {
        m_inputPorts.push_back(QPointF(0, (i + 1) * step));
    }

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void AndBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // AND 게이트 기호 그리기
    painter->setPen(QPen(Qt::darkBlue, 1.5));
    painter->setBrush(QBrush(QColor(230, 230, 250, 120)));

    // AND 게이트 본체 - D 모양
    QPainterPath path;
    path.moveTo(m_width * 0.3, m_height * 0.2);
    path.lineTo(m_width * 0.3, m_height * 0.8);
    path.lineTo(m_width * 0.5, m_height * 0.8);

    // 반원 부분
    path.arcTo(QRectF(m_width * 0.3, m_height * 0.2, m_width * 0.4, m_height * 0.6),
               -90, 180);

    path.closeSubpath();
    painter->drawPath(path);

    // AND 표시
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "AND");

    // 임계값 표시
    double threshold = m_properties["threshold"].toDouble();
    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Threshold: %1").arg(threshold));
}

double AndBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 0.0;

    double threshold = m_properties["threshold"].toDouble();

    // 모든 입력이 임계값 이상이면 1, 아니면 0
    for (double input : inputs) {
        if (input < threshold) {
            return 0.0;
        }
    }

    return 1.0;
}



OrBlock::OrBlock(const QString& name, QGraphicsItem* parent)
    : Block(OR, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["inputCount"] = 2;   // 입력 포트 개수
    m_properties["threshold"] = 0.5;  // 1로 판단할 임계값
}

void OrBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 속성에서 입력 포트 개수 가져오기
    int inputCount = m_properties["inputCount"].toInt();
    inputCount = std::max(2, std::min(inputCount, 5)); // 2~5개로 제한

    // 입력 포트 생성
    double step = m_height / (inputCount + 1);
    for (int i = 0; i < inputCount; i++) {
        m_inputPorts.push_back(QPointF(0, (i + 1) * step));
    }

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void OrBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // OR 게이트 기호 그리기
    painter->setPen(QPen(Qt::darkRed, 1.5));
    painter->setBrush(QBrush(QColor(250, 230, 230, 120)));

    // OR 게이트 본체 - 곡선 D 모양
    QPainterPath path;
    path.moveTo(m_width * 0.3, m_height * 0.2);

    // 왼쪽 곡선
    path.cubicTo(
        QPointF(m_width * 0.2, m_height * 0.3),  // 제어점 1
        QPointF(m_width * 0.2, m_height * 0.7),  // 제어점 2
        QPointF(m_width * 0.3, m_height * 0.8)   // 끝점
        );

    // 아래쪽 직선
    path.lineTo(m_width * 0.5, m_height * 0.8);

    // 오른쪽 곡선
    path.arcTo(QRectF(m_width * 0.3, m_height * 0.2, m_width * 0.4, m_height * 0.6),
               -90, 180);

    path.closeSubpath();
    painter->drawPath(path);

    // OR 표시
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "OR");

    // 임계값 표시
    double threshold = m_properties["threshold"].toDouble();
    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Threshold: %1").arg(threshold));
}

double OrBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 0.0;

    double threshold = m_properties["threshold"].toDouble();

    // 하나라도 입력이 임계값 이상이면 1, 아니면 0
    for (double input : inputs) {
        if (input >= threshold) {
            return 1.0;
        }
    }

    return 0.0;
}



NotBlock::NotBlock(const QString& name, QGraphicsItem* parent)
    : Block(NOT, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["threshold"] = 0.5;  // 1로 판단할 임계값
}

void NotBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 한 개의 입력 포트
    m_inputPorts.push_back(QPointF(0, m_height / 2));

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void NotBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Block::paint(painter, option, widget);

    // NOT 게이트 기호 그리기
    painter->setPen(QPen(Qt::darkGray, 1.5));
    painter->setBrush(QBrush(QColor(240, 240, 240, 120)));

    // NOT 게이트 본체 - 삼각형
    QPointF triangle[3] = {
        QPointF(m_width * 0.3, m_height * 0.3),
        QPointF(m_width * 0.7, m_height * 0.5),
        QPointF(m_width * 0.3, m_height * 0.7)
    };
    painter->drawPolygon(triangle, 3);

    // 출력 원(버블)
    painter->setBrush(Qt::white);
    painter->drawEllipse(QPointF(m_width * 0.75, m_height * 0.5), m_width * 0.05, m_width * 0.05);

    // NOT 표시
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "NOT");

    // 임계값 표시
    double threshold = m_properties["threshold"].toDouble();
    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Threshold: %1").arg(threshold));
}

double NotBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 1.0; // 입력 없으면 1 (논리적으로는 false의 부정)

    double threshold = m_properties["threshold"].toDouble();

    // 입력이 임계값 미만이면 1, 이상이면 0
    return (inputs[0] < threshold) ? 1.0 : 0.0;
}



XorBlock::XorBlock(const QString& name, QGraphicsItem* parent)
    : Block(XOR, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["inputCount"] = 2;   // 입력 포트 개수
    m_properties["threshold"] = 0.5;  // 1로 판단할 임계값
}

void XorBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 속성에서 입력 포트 개수 가져오기
    int inputCount = m_properties["inputCount"].toInt();
    inputCount = std::max(2, std::min(inputCount, 5)); // 2~5개로 제한

    // 입력 포트 생성
    double step = m_height / (inputCount + 1);
    for (int i = 0; i < inputCount; i++) {
        m_inputPorts.push_back(QPointF(0, (i + 1) * step));
    }

    // 한 개의 출력 포트
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void XorBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget ) {
    Block::paint(painter, option, widget);

    // XOR 게이트 기호 그리기
    painter->setPen(QPen(Qt::darkGreen, 1.5));
    painter->setBrush(QBrush(QColor(230, 250, 230, 120)));

    // XOR 게이트 본체 - 곡선 D 모양 + 추가 곡선
    QPainterPath path;

    // 첫 번째 곡선 (왼쪽 가장자리)
    path.moveTo(m_width * 0.25, m_height * 0.2);
    path.cubicTo(
        QPointF(m_width * 0.15, m_height * 0.3),  // 제어점 1
        QPointF(m_width * 0.15, m_height * 0.7),  // 제어점 2
        QPointF(m_width * 0.25, m_height * 0.8)   // 끝점
        );

    // XOR의 특징적인 두 번째 곡선
    QPainterPath secondCurve;
    secondCurve.moveTo(m_width * 0.3, m_height * 0.2);
    secondCurve.cubicTo(
        QPointF(m_width * 0.2, m_height * 0.3),  // 제어점 1
        QPointF(m_width * 0.2, m_height * 0.7),  // 제어점 2
        QPointF(m_width * 0.3, m_height * 0.8)   // 끝점
        );
    painter->drawPath(secondCurve);

    // 나머지 OR 게이트처럼 그리기
    path.moveTo(m_width * 0.3, m_height * 0.2);
    // 아래쪽 직선
    path.lineTo(m_width * 0.5, m_height * 0.8);

    // 오른쪽 곡선
    path.arcTo(QRectF(m_width * 0.3, m_height * 0.2, m_width * 0.4, m_height * 0.6),
               -90, 180);

    path.closeSubpath();
    painter->drawPath(path);

    // XOR 표시
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "XOR");

    // 임계값 표시
    double threshold = m_properties["threshold"].toDouble();
    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Threshold: %1").arg(threshold));
}

double XorBlock::compute(const std::vector<double>& inputs) const {
    if (inputs.empty()) return 0.0;

    double threshold = m_properties["threshold"].toDouble();

    // 임계값 이상인 입력 개수 세기
    int trueCount = 0;
    for (double input : inputs) {
        if (input >= threshold) {
            trueCount++;
        }
    }

    // 홀수 개수의 입력이 1이면 1, 짝수 개수이면 0
    return (trueCount % 2 == 1) ? 1.0 : 0.0;
}



ClockBlock::ClockBlock(const QString& name, QGraphicsItem* parent)
    : Block(CLOCK, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["offset"] = 0.0;   // 오프셋 (초기 시간)
    m_properties["period"] = 1.0;   // 주기 (초)
}

void ClockBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 출력 포트만 있음
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void ClockBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )  {
    Block::paint(painter, option, widget);

    // Clock 블록 아이콘 그리기
    painter->setPen(QPen(Qt::blue, 1.5));
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "Clock");

    // 시계 아이콘 그리기
    QRectF clockRect(m_width/2 - 15, m_height/2 - 15, 30, 30);
    painter->drawEllipse(clockRect);

    // 시계 바늘 그리기
    QPointF center = clockRect.center();
    // 시침
    painter->drawLine(center, QPointF(center.x(), center.y() - 10));
    // 분침
    painter->drawLine(center, QPointF(center.x() + 8, center.y() + 5));

    // 주기 표시
    double period = m_properties["period"].toDouble();
    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Period: %1s").arg(period));
}

double ClockBlock::compute(const std::vector<double>& inputs) const  {
    Q_UNUSED(inputs);

    // 엔진에서 현재 시뮬레이션 시간 가져오기
    QObject* parent = scene()->parent();
    SimulationEngine* engine = nullptr;
    if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(parent)) {
        // MainWindow에서 엔진 참조 가져오기 시도
        // 실제 구현에서는 엔진 참조를 직접 얻을 수 있는 방법이 필요함
        // 여기서는 간단한 예시로 전역 변수나 싱글톤 패턴을 사용할 수 있음
        engine = dynamic_cast<SimulationEngine*>(mainWindow->findChild<SimulationEngine*>());
    }

    double time = 0.0;
    if (engine) {
        // 엔진에서 현재 시간 가져오기
        time = engine->getCurrentTime();
    }

    double offset = m_properties["offset"].toDouble();
    double period = m_properties["period"].toDouble();

    // 주기적으로 증가하는 값 반환 (0 ~ period)
    return std::fmod(time - offset, period);
}



RampBlock::RampBlock(const QString& name, QGraphicsItem* parent)
    : Block(RAMP, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["slope"] = 1.0;     // 기울기
    m_properties["startTime"] = 0.0;  // 시작 시간
    m_properties["initialOutput"] = 0.0; // 초기 출력값
}

void RampBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 출력 포트만 있음
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void RampBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )  {
    Block::paint(painter, option, widget);

    // Ramp 블록 아이콘 그리기
    painter->setPen(QPen(Qt::darkGreen, 1.5));
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "Ramp");

    // 램프 그래프 그리기
    QPointF start(m_width * 0.2, m_height * 0.8);
    QPointF end(m_width * 0.8, m_height * 0.3);

    painter->drawLine(start, end);

    // 속성 표시
    double slope = m_properties["slope"].toDouble();
    double startTime = m_properties["startTime"].toDouble();

    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Slope: %1").arg(slope));
}

double RampBlock::compute(const std::vector<double>& inputs) const  {
    Q_UNUSED(inputs);

    // 현재 시간 가져오기
    double time = 0.0;
    QObject* parent = scene()->parent();
    SimulationEngine* engine = nullptr;
    if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(parent)) {
        engine = dynamic_cast<SimulationEngine*>(mainWindow->findChild<SimulationEngine*>());
        if (engine) {
            time = engine->getCurrentTime();
        }
    }

    std::cout << time << std::endl;

    double slope = m_properties["slope"].toDouble();
    double startTime = m_properties["startTime"].toDouble();
    double initialOutput = m_properties["initialOutput"].toDouble();

    // 시작 시간 이전에는 초기값, 이후에는 기울기에 따라 증가
    if (time < startTime) {
        return initialOutput;
    } else {
        return initialOutput + slope * (time - startTime);
    }
}


StepBlock::StepBlock(const QString& name, QGraphicsItem* parent)
    : Block(STEP, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["stepTime"] = 1.0;     // 스텝 시간
    m_properties["initialValue"] = 0.0;  // 초기값
    m_properties["finalValue"] = 1.0;    // 최종값
}

void StepBlock::setupPorts()  {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 출력 포트만 있음
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void StepBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )  {
    Block::paint(painter, option, widget);

    // Step 블록 아이콘 그리기
    painter->setPen(QPen(Qt::darkRed, 1.5));
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "Step");

    // 스텝 그래프 그리기
    double initialY = m_height * 0.7;
    double finalY = m_height * 0.3;
    double stepX = m_width * 0.5;

    QPainterPath path;
    path.moveTo(m_width * 0.2, initialY);
    path.lineTo(stepX, initialY);
    path.lineTo(stepX, finalY);
    path.lineTo(m_width * 0.8, finalY);

    painter->drawPath(path);

    // 속성 표시
    double stepTime = m_properties["stepTime"].toDouble();
    double initialValue = m_properties["initialValue"].toDouble();
    double finalValue = m_properties["finalValue"].toDouble();

    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Step at: %1s").arg(stepTime));
}

double StepBlock::compute(const std::vector<double>& inputs) const  {
    Q_UNUSED(inputs);

    // 현재 시간 가져오기
    double time = 0.0;
    QObject* parent = scene()->parent();
    SimulationEngine* engine = nullptr;
    if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(parent)) {
        engine = dynamic_cast<SimulationEngine*>(mainWindow->findChild<SimulationEngine*>());
        if (engine) {
            time = engine->getCurrentTime();
        }
    }

    double stepTime = m_properties["stepTime"].toDouble();
    double initialValue = m_properties["initialValue"].toDouble();
    double finalValue = m_properties["finalValue"].toDouble();

    // 스텝 시간 이전에는 초기값, 이후에는 최종값
    return (time < stepTime) ? initialValue : finalValue;
}



SineWaveBlock::SineWaveBlock(const QString& name, QGraphicsItem* parent)
    : Block(SINE_WAVE, name, parent) {
    setupPorts();

    // 기본 속성 설정
    m_properties["amplitude"] = 1.0;    // 진폭
    m_properties["frequency"] = 1.0;    // 주파수 (Hz)
    m_properties["phase"] = 0.0;        // 위상 (라디안)
    m_properties["bias"] = 0.0;         // 바이어스 (오프셋)
}

void SineWaveBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 출력 포트만 있음
    m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void SineWaveBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )  {
    Block::paint(painter, option, widget);

    // Sine Wave 블록 아이콘 그리기
    painter->setPen(QPen(Qt::darkBlue, 1.5));
    painter->setFont(QFont("Arial", 9, QFont::Bold));
    painter->drawText(QRectF(10, 5, m_width - 20, 20), Qt::AlignCenter, "Sine Wave");

    // 사인파 그래프 그리기
    QPainterPath path;

    double centerY = m_height * 0.5;
    double amplitude = m_height * 0.25;
    double startX = m_width * 0.2;
    double endX = m_width * 0.8;
    double step = (endX - startX) / 20.0;

    path.moveTo(startX, centerY);

    for (double x = startX; x <= endX; x += step) {
        double t = (x - startX) / (endX - startX) * 2 * M_PI;
        double y = centerY - amplitude * std::sin(t);
        path.lineTo(x, y);
    }

    painter->drawPath(path);

    // 속성 표시
    double freq = m_properties["frequency"].toDouble();
    double amp = m_properties["amplitude"].toDouble();

    painter->setFont(QFont("Arial", 6));
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10), Qt::AlignRight,
                      QString("Freq: %1 Hz").arg(freq));
}

double SineWaveBlock::compute(const std::vector<double>& inputs) const  {
    Q_UNUSED(inputs);

    // 현재 시간 가져오기
    double time = 0.0;
    QObject* parent = scene()->parent();
    SimulationEngine* engine = nullptr;
    if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(parent)) {
        engine = dynamic_cast<SimulationEngine*>(mainWindow->findChild<SimulationEngine*>());
        if (engine) {
            time = engine->getCurrentTime();
        }
    }

    double amplitude = m_properties["amplitude"].toDouble();
    double frequency = m_properties["frequency"].toDouble();
    double phase = m_properties["phase"].toDouble();
    double bias = m_properties["bias"].toDouble();

    // 사인파 생성
    return amplitude * std::sin(2 * M_PI * frequency * time + phase) + bias;
}
