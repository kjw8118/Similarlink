#ifndef SIMILARLINK_H
#define SIMILARLINK_H

#include <QMainWindow>
#include <QListWidget>
#include <QDockWidget>
#include <QObject>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QTextEdit>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QProgressBar>
#include <QTabWidget>


#include "blockdialog.h"
#include "block.h"
#include "blocklistwidget.h"
#include "simulation.h"

#include "codegenerator.h"

#include "blockfactory.h"

#include "subsystemtab.h"
#include "subsystemblock.h"
#include "subsystemdialog.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class Similarlink;
}
QT_END_NAMESPACE

class Similarlink : public QMainWindow
{
    Q_OBJECT

public:
    Similarlink(QWidget *parent = nullptr);
    ~Similarlink();

    QDialog* createBlockPropertyDialog(Block* block, QWidget* parent);

    // Similarlink 시뮬레이션 지속 시간을 가져오는 메서드 추가
    double getSimulationDuration() const;

    // MainWindow 클래스 시간 간격 getter
    double getSimulationTimeStep() const {
        return m_timeStepSpinBox ? m_timeStepSpinBox->value() : 0.01;
    }

    // 블록 이름 중복 체크 및 고유 이름 생성 함수 추가
    bool isBlockNameExists(const QString& name) const;
    QString generateUniqueBlockName(const QString& baseName) const;

private:
    //Ui::Similarlink *ui;
    QDockWidget* m_blocksDock;  // 블록 라이브러리 도크 위젯 저장용 변수
    void showScopeDialog(ScopeBlock* scope);

    void setupUI();

    void setupMenus();

    void setupToolbar();

    bool eventFilter(QObject* watched, QEvent* event) override;

    void createBlockAt(const QString& typeName, const QPointF& scenePos);

    void setupStatusBar();

    // 시뮬레이션 엔진 getter - 외부에서 필요할 경우 사용
    SimulationEngine* getSimulationEngine() const {
        return m_engine;
    }

    // 저장 관련 헬퍼 함수
    bool saveModelToFile(const QString& filePath);

    // 파일이 수정되었는지 확인하고 저장 여부 확인
    bool maybeSave();

    // 창 제목 업데이트
    void updateWindowTitle();

    // 파일 수정 상태 설정
    void setModified(bool modified);

    // 서브시스템 내의 블록 속성 다이얼로그 생성 헬퍼
    QDialog* createSubsystemBlockPropertyDialog(Block* block, SubsystemTab* parentTab);

private:
    SimulationScene* m_scene;
    SimulationView* m_view;
    BlockListWidget* m_blockList;
    SimulationEngine* m_engine;    
    QDoubleSpinBox* m_durationSpinBox;
    QDoubleSpinBox* m_timeStepSpinBox;
    QComboBox* m_playbackSpeedComboBox;

    // 파일 관련 변수
    QString m_currentFilePath;  // 현재 열린 파일 경로
    bool m_isModified;         // 파일 수정 여부

    // 마지막 블록 위치 저장 변수
    QPointF m_lastBlockPosition;
    bool m_hasLastPosition;

    // Actions
    QAction* m_newAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_loadAction;
    QAction* m_deleteAction;
    QAction* m_startAction;
    QAction* m_stopAction;

    QLabel* m_timeLabel;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;

    // 서브시스템 관련 UI 요소
    QTabWidget* m_tabWidget;  // 메인 탭 위젯
    QList<SubsystemTab*> m_subsystemTabs;  // 열린 서브시스템 탭 목록

public slots:
    // 파일 관련 메서드 추가
    void onNewModel();
    void onSaveModel();
    void onSaveModelAs();
    void onLoadModel();

    // 코드 생성 메서드 추가
    void onGenerateCppCode();
    void onGeneratePythonCode();
    void onGenerateCaplCode();


    // 4. 윈도우 상태 저장 및 복원 기능 추가 (선택 사항)
    void closeEvent(QCloseEvent* event);

    void readSettings();

private slots:
    void onAddBlock();

    void onDeleteSelectedItems();

    void onBlockDoubleClicked(QGraphicsItem* item);

    void onStartSimulation();
    void onStopSimulation();

    void onSimulationStarted();

    void onSimulationStopped();

    void onSimulationStepped(double time, const std::map<Block*, double>& outputs);


    // 씬 변경 감지
    void onSceneChanged();

    // 서브시스템 관련 슬롯 함수들
    void onEditSubsystem(SubsystemBlock* block);
    void onCloseSubsystemTab(SubsystemTab* tab);
    void onTabChanged(int index);
    void onCreateNewSubsystem();
    void onSubsystemContentsChanged();
};
#endif // SIMILARLINK_H
