#ifndef SUBSYSTEMTAB_H
#define SUBSYSTEMTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QJsonObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>

#include "simulation.h"
#include "subsystemblock.h"
#include "block.h"
#include "connection.h"

class SubsystemTab : public QWidget {
    Q_OBJECT

public:
    SubsystemTab(SubsystemBlock* subsystemBlock, QWidget* parent = nullptr);
    ~SubsystemTab();

    // 탭 이름 가져오기
    QString getTabName() const;

    // 서브시스템 블록 가져오기
    SubsystemBlock* getSubsystemBlock() const;

    // 편집 중인 내용이 있는지 확인
    bool hasUnsavedChanges() const;

    // 모델 저장
    bool saveModel();

    // 모델 초기화
    void loadModelFromSubsystem();

    // 편집 중인 모델을 서브시스템 블록에 적용
    void applyChangesToSubsystem();

    // 블록 추가 메서드 추가
    void createBlockAt(const QString& typeName, const QPointF& scenePos);

    // 고유한 블록 이름 생성 및 중복 확인
    QString generateUniqueBlockName(const QString& baseName) const;
    bool isBlockNameExists(const QString& name) const;

    // 뷰 접근자
    SimulationView* getView() const { return m_view; }

    // 마지막 블록 위치 관련 접근자
    bool hasLastPosition() const { return m_hasLastPosition; }
    QPointF getLastBlockPosition() const { return m_lastBlockPosition; }
    // 탭 활성화 시 호출
    void activate();
signals:
    // 탭 닫기 요청 신호
    void closeRequested(SubsystemTab* tab);

    // 변경사항 알림 신호
    void contentsChanged();
    void editNestedSubsystem(SubsystemBlock* subsystem);

protected:
    // 이벤트 처리
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

public slots:
    // 시뮬레이션 씬 변경 감지
    void onSceneChanged();

    // 블록 더블 클릭 처리
    void onBlockDoubleClicked(QGraphicsItem* item);

    // 저장 버튼 클릭 처리
    void onSaveClicked();

    // 선택된 항목 삭제 처리
    void onDeleteSelectedItems();

    // 중첩 서브시스템 편집 처리
    void onEditNestedSubsystem(SubsystemBlock* subsystem);

private:
    // UI 설정
    void setupUI();

    // 툴바 설정
    void setupToolbar();

    // 블록 속성 다이얼로그 생성
    QDialog* createBlockPropertyDialog(Block* block, QWidget* parent);
    // 고유한 블록 이름 생성 및 중복 확인

    // 탭 이름 변경
    void setTabName(const QString& name);



    // 서브시스템 블록에서 데이터 갱신
    void refreshFromSubsystemBlock();
private:
    SubsystemBlock* m_subsystemBlock;
    bool m_isModified;
    QString m_tabName;

    // UI 요소
    SimulationScene* m_scene;
    SimulationView* m_view;
    QToolBar* m_toolbar;

    // 액션들
    QAction* m_saveAction;
    QAction* m_closeAction;
    QAction* m_deleteAction;  // 삭제 액션 추가

    // 마지막 블록 위치 저장 변수
    QPointF m_lastBlockPosition;
    bool m_hasLastPosition;
};

#endif // SUBSYSTEMTAB_H
