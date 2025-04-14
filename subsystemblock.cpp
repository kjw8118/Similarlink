#include "subsystemblock.h"
#include <QPainter>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QGraphicsItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QList>
#include "blockfactory.h"

// 서브시스템 블록 생성자
SubsystemBlock::SubsystemBlock(const QString& name, QGraphicsItem* parent)
    : Block(Block::SUBSYSTEM, name, parent) {
    // 서브시스템 블록의 크기는 일반 블록보다 약간 크게 설정
    m_width = 120;
    m_height = 80;

    // 초기 포트 설정
    setupPorts();

    // 속성 초기화
    m_properties["embedded"] = false;  // 기본적으로 외부 파일 모드
    m_properties["filePath"] = "";     // 외부 파일 경로

    // 기본 빈 모델 생성
    m_embeddedModel = QJsonObject{
        {"blocks", QJsonArray()},
        {"connections", QJsonArray()}
    };
}

SubsystemBlock::~SubsystemBlock() {
    // 필요한 정리 작업 수행
}

void SubsystemBlock::setSubsystemPath(const QString& path) {
    m_subsystemPath = path;
    m_properties["filePath"] = path;
    m_properties["embedded"] = path.isEmpty();

    // 외부 파일이면 로드
    if (!path.isEmpty()) {
        loadFromFile(path);
    }
}

QString SubsystemBlock::getSubsystemPath() const {
    return m_subsystemPath;
}

void SubsystemBlock::setEmbeddedModel(const QJsonObject& model) {
    m_embeddedModel = model;
    m_properties["embedded"] = true;
    m_subsystemPath = "";

    // 내장 모델에서 초기화
    initFromEmbeddedModel();
}

QJsonObject SubsystemBlock::getEmbeddedModel() const {
    return m_embeddedModel;
}

bool SubsystemBlock::isExternalFile() const {
    return !m_subsystemPath.isEmpty();
}

void SubsystemBlock::setupPorts() {
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 기본 입력/출력 포트 생성 (최소 1개씩)
    //m_inputPorts.push_back(QPointF(0, m_height / 2));
    //m_outputPorts.push_back(QPointF(m_width, m_height / 2));
}

void SubsystemBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    // 기본 블록 배경 그리기 (검은색 대신 밝은 파란색으로 변경)
    painter->setBrush(QColor(230, 240, 255));  // 연한 파란색 배경
    painter->setPen(QPen(Qt::black, 1));
    painter->drawRoundedRect(0, 0, m_width, m_height, 5, 5);

    // 서브시스템임을 나타내는 추가 시각적 표시
    painter->setPen(QPen(Qt::darkBlue, 1, Qt::DashLine));
    painter->drawRoundedRect(5, 5, m_width - 10, m_height - 10, 3, 3);

    // 타이틀 영역 그리기
    QLinearGradient titleGradient(0, 0, 0, 20);
    titleGradient.setColorAt(0, QColor(120, 140, 220));
    titleGradient.setColorAt(1, QColor(100, 120, 200));
    painter->setBrush(titleGradient);
    painter->setPen(Qt::NoPen);
    painter->drawRect(0, 0, m_width, 20);

    // 서브시스템 이름 그리기
    painter->setPen(Qt::white);
    QFont titleFont = painter->font();
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->drawText(QRectF(5, 0, m_width - 10, 20),
                      Qt::AlignCenter, getName());

    // 내부 블록 미리보기 그리기
    QJsonObject model = m_embeddedModel;
    QJsonArray blocksArray = model["blocks"].toArray();

    // 미니맵 영역 계산
    QRectF minimapRect(10, 25, m_width - 20, m_height - 40);

    // 미니맵 배경
    painter->setBrush(QColor(245, 245, 255));
    painter->setPen(QPen(Qt::lightGray, 0.5));
    painter->drawRect(minimapRect);

    if (blocksArray.size() > 0) {
        // 내부 블록의 위치를 기반으로 미니맵 축척 계산
        qreal minX = std::numeric_limits<qreal>::max();
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxX = std::numeric_limits<qreal>::min();
        qreal maxY = std::numeric_limits<qreal>::min();

        for (const QJsonValue& value : blocksArray) {
            QJsonObject blockObj = value.toObject();
            qreal x = blockObj["x"].toDouble();
            qreal y = blockObj["y"].toDouble();

            minX = qMin(minX, x);
            minY = qMin(minY, y);
            maxX = qMax(maxX, x + 80);  // 블록 폭 고려
            maxY = qMax(maxY, y + 40);  // 블록 높이 고려
        }

        // 축척 및 오프셋 계산
        qreal scaleX = minimapRect.width() / (maxX - minX + 20);
        qreal scaleY = minimapRect.height() / (maxY - minY + 20);
        qreal scale = qMin(scaleX, scaleY);

        // 미니맵 내에서의 오프셋 (중앙 정렬)
        qreal offsetX = minimapRect.x() + (minimapRect.width() - (maxX - minX) * scale) / 2;
        qreal offsetY = minimapRect.y() + (minimapRect.height() - (maxY - minY) * scale) / 2;

        // 축척된 좌표에 따라 미니 블록 그리기
        for (const QJsonValue& value : blocksArray) {
            QJsonObject blockObj = value.toObject();
            qreal x = blockObj["x"].toDouble();
            qreal y = blockObj["y"].toDouble();
            int blockType = blockObj["type"].toInt();

            // 블록 타입에 따른 색상 설정
            QColor blockColor;
            switch (blockType) {
            case Block::IN:
                blockColor = QColor(100, 220, 100);  // 입력 블록은 녹색
                break;
            case Block::OUT:
                blockColor = QColor(220, 100, 100);  // 출력 블록은 빨간색
                break;
            case Block::SUBSYSTEM:
                blockColor = QColor(130, 160, 220);  // 중첩 서브시스템은 파란색
                break;
            default:
                blockColor = QColor(220, 220, 150);  // 기본 블록은 노란색
            }

            // 미니 블록 그리기
            qreal blockX = offsetX + (x - minX) * scale;
            qreal blockY = offsetY + (y - minY) * scale;
            qreal blockWidth = 25 * scale;  // 미니 블록 폭
            qreal blockHeight = 15 * scale; // 미니 블록 높이

            painter->setBrush(blockColor);
            painter->setPen(QPen(Qt::darkGray, 0.5));
            painter->drawRect(blockX, blockY, blockWidth, blockHeight);
        }

        // 연결 그리기
        QJsonArray connectionsArray = model["connections"].toArray();
        painter->setPen(QPen(Qt::darkGray, 0.7, Qt::SolidLine));

        for (const QJsonValue& value : connectionsArray) {
            QJsonObject connObj = value.toObject();
            QString sourceBlockName = connObj["sourceBlockName"].toString();
            QString destBlockName = connObj["destBlockName"].toString();

            // 연결된 블록 찾기
            QPointF sourcePos, destPos;
            bool foundSource = false, foundDest = false;

            for (const QJsonValue& blockValue : blocksArray) {
                QJsonObject blockObj = blockValue.toObject();
                QString blockName = blockObj["name"].toString();

                if (blockName == sourceBlockName) {
                    qreal x = blockObj["x"].toDouble();
                    qreal y = blockObj["y"].toDouble();
                    sourcePos = QPointF(
                        offsetX + (x - minX + 25) * scale,  // 블록 오른쪽 중앙
                        offsetY + (y - minY + 7.5) * scale
                        );
                    foundSource = true;
                }

                if (blockName == destBlockName) {
                    qreal x = blockObj["x"].toDouble();
                    qreal y = blockObj["y"].toDouble();
                    destPos = QPointF(
                        offsetX + (x - minX) * scale,  // 블록 왼쪽 중앙
                        offsetY + (y - minY + 7.5) * scale
                        );
                    foundDest = true;
                }

                if (foundSource && foundDest) break;
            }

            if (foundSource && foundDest) {
                painter->drawLine(sourcePos, destPos);
            }
        }
    } else {
        // 블록이 없는 경우 메시지 표시
        painter->setPen(Qt::gray);
        painter->drawText(minimapRect, Qt::AlignCenter, "Empty Subsystem");
    }

    // 내장 모델인지 외부 파일인지 표시
    QFont infoFont = painter->font();
    infoFont.setPointSizeF(infoFont.pointSizeF() * 0.7);
    painter->setFont(infoFont);
    painter->setPen(Qt::darkGray);

    QString pathInfo = isExternalFile() ?
                           QFileInfo(m_subsystemPath).fileName() :
                           "Embedded";

    // 이름 아래에 작은 글씨로 표시
    painter->drawText(QRectF(5, m_height - 15, m_width - 10, 10),
                      Qt::AlignRight, pathInfo);

    // 포트 그리기
    drawPorts(painter);
}
// 포트 그리기 함수 (기존 paint 메서드에 포함된 코드를 별도 함수로 분리)
void SubsystemBlock::drawPorts(QPainter* painter) {
    painter->setBrush(QColor(220, 220, 220));
    painter->setPen(QPen(Qt::black, 1));

    // 입력 포트
    for (const QPointF& port : m_inputPorts) {
        painter->drawEllipse(port.x() - 4, port.y() - 4, 8, 8);
    }

    // 출력 포트
    for (const QPointF& port : m_outputPorts) {
        painter->drawEllipse(port.x() - 4, port.y() - 4, 8, 8);
    }

    // 포트 레이블 그리기 (필요한 경우)
    painter->setPen(Qt::black);
    QFont portFont = painter->font();
    portFont.setPointSizeF(portFont.pointSizeF() * 0.8);
    painter->setFont(portFont);

    for (int i = 0; i < m_inputPortInfos.size(); ++i) {
        if (i < m_inputPorts.size()) {
            QPointF port = m_inputPorts[i];
            painter->drawText(QRectF(port.x() + 5, port.y() - 7, 50, 14),
                              Qt::AlignLeft | Qt::AlignVCenter,
                              m_inputPortInfos[i].first);
        }
    }

    for (int i = 0; i < m_outputPortInfos.size(); ++i) {
        if (i < m_outputPorts.size()) {
            QPointF port = m_outputPorts[i];
            painter->drawText(QRectF(port.x() - 55, port.y() - 7, 50, 14),
                              Qt::AlignRight | Qt::AlignVCenter,
                              m_outputPortInfos[i].first);
        }
    }
}
double SubsystemBlock::compute(const std::vector<double>& inputs) const {
    // 서브시스템 계산 로직
    // 이 함수는 상위 모델에서 호출됨

    // 기본적으로 첫 번째 출력 포트의 값을 반환
    // 복잡한 시뮬레이션 계산은 실제 구현에서 처리해야 함

    // 여기서는 간단히 마지막 입력을 출력으로 전달
    if (!inputs.empty() && !m_outputPorts.empty()) {
        return inputs.back();
    }

    return 0.0;  // 기본값
}

void SubsystemBlock::updatePortsFromSubsystem() {
    // 부모 클래스의 포트 위치 초기화
    m_inputPorts.clear();
    m_outputPorts.clear();

    // 내부 포트 정보 초기화
    m_inputPortInfos.clear();
    m_outputPortInfos.clear();

    // 서브시스템 모델 분석
    QJsonObject model = isExternalFile() ?
                            loadModelFromFile(m_subsystemPath) :
                            m_embeddedModel;

    QJsonArray blocksArray = model["blocks"].toArray();

    // In/Out 블록 찾기
    QList<QPair<QString, QPointF>> inBlocks;  // 이름, 위치
    QList<QPair<QString, QPointF>> outBlocks; // 이름, 위치

    for (const QJsonValue& value : blocksArray) {
        QJsonObject blockObj = value.toObject();
        int blockType = blockObj["type"].toInt();

        // In 블록 찾기
        if (blockType == Block::IN) {
            QString blockName = blockObj["name"].toString();
            double x = blockObj["x"].toDouble();
            double y = blockObj["y"].toDouble();

            // 변수 이름 가져오기
            QJsonObject propsObj = blockObj["properties"].toObject();
            QString varName = propsObj["variable"].toString();
            if (varName.isEmpty()) {
                varName = blockName; // 변수 이름이 없으면 블록 이름 사용
            }

            inBlocks.append(qMakePair(varName, QPointF(x, y)));
        }
        // Out 블록 찾기
        else if (blockType == Block::OUT) {
            QString blockName = blockObj["name"].toString();
            double x = blockObj["x"].toDouble();
            double y = blockObj["y"].toDouble();

            // 변수 이름 가져오기
            QJsonObject propsObj = blockObj["properties"].toObject();
            QString varName = propsObj["variable"].toString();
            if (varName.isEmpty()) {
                varName = blockName; // 변수 이름이 없으면 블록 이름 사용
            }

            outBlocks.append(qMakePair(varName, QPointF(x, y)));
        }
    }

    // In 블록에 따라 입력 포트 생성
    m_inputPorts.clear();

    // In 블록 위치에 따라 정렬 (위에서 아래로)
    std::sort(inBlocks.begin(), inBlocks.end(),
              [](const QPair<QString, QPointF>& a, const QPair<QString, QPointF>& b) {
                  return a.second.y() < b.second.y();
              });

    // 포트 생성 및 정보 저장 (분리)
    for (int i = 0; i < inBlocks.size(); ++i) {
        double portY = (i + 1) * m_height / (inBlocks.size() + 1);
        m_inputPorts.push_back(QPointF(0, portY));  // 위치
        m_inputPortInfos.append(qMakePair(inBlocks[i].first, i));  // 정보
    }

    // Out 블록에 따라 출력 포트 생성
    m_outputPorts.clear();

    // Out 블록 위치에 따라 정렬 (위에서 아래로)
    std::sort(outBlocks.begin(), outBlocks.end(),
              [](const QPair<QString, QPointF>& a, const QPair<QString, QPointF>& b) {
                  return a.second.y() < b.second.y();
              });

    // 출력 포트 생성 및 정보 저장
    for (int i = 0; i < outBlocks.size(); ++i) {
        double portY = (i + 1) * m_height / (outBlocks.size() + 1);
        m_outputPorts.push_back(QPointF(m_width, portY));
        m_outputPortInfos.append(qMakePair(outBlocks[i].first, i));
    }

    // 포트가 없는 경우 기본 포트를 추가하지 않음
    // 포트는 In/Out 블록이 있을 때만 생성
}

void SubsystemBlock::editSubsystem() {
    // 서브시스템 편집 요청
    // 이 함수는 SubsystemDialog를 열거나 새 탭을 열도록 신호를 발생시킴
    // Similarlink 클래스에서 처리할 예정

    // 이 함수는 블록이 더블클릭되었을 때 호출됨
    emit editSubsystemRequested(this);
}

bool SubsystemBlock::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open subsystem file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in subsystem file:" << filePath;
        return false;
    }

    m_subsystemPath = filePath;
    m_properties["filePath"] = filePath;
    m_properties["embedded"] = false;

    // 서브시스템 모델 설정
    QJsonObject model = doc.object();
    m_embeddedModel = model;  // 캐시로 저장

    // 모델에서 포트 정보 업데이트
    updatePortsFromSubsystem();

    return true;
}

void SubsystemBlock::initFromEmbeddedModel() {
    if (m_embeddedModel.isEmpty()) {
        // 기본 빈 모델 생성
        m_embeddedModel = QJsonObject{
            {"blocks", QJsonArray()},
            {"connections", QJsonArray()}
        };
    }

    m_properties["embedded"] = true;
    m_subsystemPath = "";

    // 모델에서 포트 정보 업데이트
    updatePortsFromSubsystem();
}

bool SubsystemBlock::saveToFile(const QString& filePath) const {
    // 내장 모델을 파일로 저장
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(m_embeddedModel);
    file.write(doc.toJson());
    file.close();

    return true;
}

QList<QPair<QString, int>> SubsystemBlock::getInputPortInfos() const {
    return m_inputPortInfos;
}

QList<QPair<QString, int>> SubsystemBlock::getOutputPortInfos() const {
    return m_outputPortInfos;
}

// 파일에서 모델 불러오기 헬퍼 함수
QJsonObject SubsystemBlock::loadModelFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file:" << filePath;
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in file:" << filePath;
        return QJsonObject();
    }

    return doc.object();
}
