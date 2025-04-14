#ifndef SUBSYSTEMBLOCK_H
#define SUBSYSTEMBLOCK_H

#include "block.h"
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QGraphicsScene>

class SubsystemBlock : public Block {
    Q_OBJECT  // 꼭 추가해야 함
public:
    SubsystemBlock(const QString& name = "Subsystem", QGraphicsItem* parent = nullptr);
    ~SubsystemBlock();

    // 서브시스템 경로 설정 (외부 파일 또는 내장 모델)
    void setSubsystemPath(const QString& path);
    QString getSubsystemPath() const;

    // 내장 모델 설정 및 가져오기
    void setEmbeddedModel(const QJsonObject& model);
    QJsonObject getEmbeddedModel() const;

    // 서브시스템 정의가 외부 파일인지 내장 모델인지 여부
    bool isExternalFile() const;

    // 블록 오버라이드 메서드들
    void setupPorts() override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    double compute(const std::vector<double>& inputs) const override;

    // 서브시스템 내의 입출력 블록 정보 분석 후 포트 업데이트
    void updatePortsFromSubsystem();

    // 서브시스템 편집
    void editSubsystem();

    // 외부 파일에서 서브시스템 로드
    bool loadFromFile(const QString& filePath);

    // 내장 모델에서 서브시스템 초기화
    void initFromEmbeddedModel();

    // 서브시스템 저장
    bool saveToFile(const QString& filePath) const;

    // 반환 타입과 메서드 이름 수정
    QList<QPair<QString, int>> getInputPortInfos() const;
    QList<QPair<QString, int>> getOutputPortInfos() const;



private:
    QString m_subsystemPath;    // 외부 파일 경로 (비어있으면 내장 모델)
    QJsonObject m_embeddedModel;  // 내장된 서브시스템 모델 (JSON)

    // 포트 추가 정보 (이름, 인덱스)
    QList<QPair<QString, int>> m_inputPortInfos;
    QList<QPair<QString, int>> m_outputPortInfos;

    // 내부 시뮬레이션 상태 (계산을 위해 필요)
    mutable QMap<QString, double> m_internalValues;

    // 파일에서 모델 불러오기 헬퍼 함수 (private 메서드로 선언)
    QJsonObject loadModelFromFile(const QString& filePath);
    // 포트 그리기 도우미 메서드
    void drawPorts(QPainter* painter) override;
signals:
    // 서브시스템 편집 요청 신호
    void editSubsystemRequested(SubsystemBlock* subsystem);
};

#endif // SUBSYSTEMBLOCK_H
