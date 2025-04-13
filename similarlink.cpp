#include "similarlink.h"
//#include "./ui_similarlink.h"

Similarlink::Similarlink(QWidget *parent)
    : QMainWindow(parent),
    m_isModified(false),
    m_lastBlockPosition(0, 0),
    m_hasLastPosition(false)
    //, ui(new Ui::Similarlink)
{
    //ui->setupUi(this);

    setWindowTitle("Similarlink");
    resize(1200, 800);

    setupUI();

    m_engine = new SimulationEngine(m_scene, this);
    connect(m_engine, &SimulationEngine::started, this, &Similarlink::onSimulationStarted);
    connect(m_engine, &SimulationEngine::stopped, this, &Similarlink::onSimulationStopped);
    connect(m_engine, &SimulationEngine::simulationStepped, this, &Similarlink::onSimulationStepped);

    // 저장된 설정 복원 (선택 사항)
    readSettings();

    // 창 제목 초기화
    m_currentFilePath = "";
    updateWindowTitle();
    // 씬 변경 감지
    connect(m_scene, &QGraphicsScene::changed, this, &Similarlink::onSceneChanged);
}

Similarlink::~Similarlink()
{
    //delete ui;
}


QDialog* Similarlink::createBlockPropertyDialog(Block* block, QWidget* parent) {
    switch (block->getType()) {
    case Block::LOOKUP_TABLE_1D:
    case Block::LOOKUP_TABLE_2D:
        return new LookupTablePropertyDialog(block, parent);

    case Block::SWITCH:
        return new SwitchPropertyDialog(block, parent);

    case Block::AND:
    case Block::OR:
    case Block::NOT:
    case Block::XOR:
        return new LogicGatePropertyDialog(block, parent);

    case Block::SATURATION:
    case Block::RATE_LIMITER:
        return new LimiterPropertyDialog(block, parent);

    case Block::CLOCK:
    case Block::RAMP:
    case Block::STEP:
    case Block::SINE_WAVE:
        return new SignalGeneratorPropertyDialog(block, parent);
    default:
        return new BlockPropertyDialog(block, parent);
    }
}

// Similarlink 시뮬레이션 지속 시간을 가져오는 메서드 추가
double Similarlink::getSimulationDuration() const {
    return m_durationSpinBox ? m_durationSpinBox->value() : 10.0;
}

void Similarlink::showScopeDialog(ScopeBlock* scope) {
    ScopeDialog* dialog = new ScopeDialog(scope, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void Similarlink::setupStatusBar(){
    // 상태 표시줄에 진행 상태 위젯 추가
    QStatusBar* statusBar = this->statusBar();

    // 시간 표시 레이블
    m_timeLabel = new QLabel("Time: 0.000 s");
    m_timeLabel->setMinimumWidth(120);
    statusBar->addWidget(m_timeLabel);

    // 진행 상태 표시 바
    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(1000); // 0.1% 단위로 표시
    m_progressBar->setValue(0);
    m_progressBar->setFormat("%p%"); // 퍼센트 표시
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setVisible(false); // 초기에는 숨김
    statusBar->addWidget(m_progressBar);

    // 상태 메시지
    m_statusLabel = new QLabel("Ready");
    statusBar->addPermanentWidget(m_statusLabel);
}
void Similarlink::setupUI() {
    // Create central widget
    QWidget* centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    // Create main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // Create scene and view
    m_scene = new SimulationScene(this);
    m_scene->setSceneRect(QRectF(0, 0, 2000, 2000));

    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        m_deleteAction->setEnabled(!m_scene->selectedItems().isEmpty());
    });

    m_view = new SimulationView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setDragMode(QGraphicsView::RubberBandDrag); // 기본은 선택 드래그 모드

    // 씬의 마우스 이벤트를 필터링하여 드래그 모드 동적 변경
    m_view->viewport()->installEventFilter(this);

    // Connect double click signal
    connect(m_view, &QGraphicsView::rubberBandChanged, this, [this](const QRect&, const QPointF&, const QPointF&) {
        // This is a workaround for rubber band selection clearing current selection
        m_deleteAction->setEnabled(!m_scene->selectedItems().isEmpty());
    });

    // Handle double click on items
    m_view->viewport()->installEventFilter(this);

    // 키 이벤트를 받기 위해 뷰에 포커스 정책 설정
    m_view->setFocusPolicy(Qt::StrongFocus);

    // 키 이벤트를 처리하기 위한 이벤트 필터 설치
    m_view->installEventFilter(this);

    // Create dock widget for blocks library
    m_blocksDock  = new QDockWidget("Blocks Library", this);
    m_blocksDock ->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_blocksDock->setObjectName("BlockLibraryDock"); // 상태 저장을 위한 고유 이름

    QWidget* dockWidget = new QWidget();
    QVBoxLayout* dockLayout = new QVBoxLayout(dockWidget);

    // QListWidget 대신 커스텀 BlockListWidget 사용
    m_blockList = new BlockListWidget();
    /*m_blockList->addItem("Source");
    m_blockList->addItem("Clock");        // 추가: Clock 블록
    m_blockList->addItem("Ramp");         // 추가: Ramp 블록
    m_blockList->addItem("Step");         // 추가: Step 블록
    m_blockList->addItem("Sine Wave");    // 추가: Sine Wave 블록

    m_blockList->addItem("Gain");
    m_blockList->addItem("Sum");
    m_blockList->addItem("Product");
    m_blockList->addItem("Integrator");
    m_blockList->addItem("Derivative");
    m_blockList->addItem("Scope");
    m_blockList->addItem("In");     // 추가: In 블록
    m_blockList->addItem("Out");    // 추가: Out 블록
    m_blockList->addItem("Lookup Table 1D");  // 추가: 1D Lookup Table
    m_blockList->addItem("Lookup Table 2D");  // 추가: 2D Lookup Table
    // 수학 연산 블록
    m_blockList->addItem("Min");
    m_blockList->addItem("Max");
    m_blockList->addItem("Divide");

    // 제한 블록
    m_blockList->addItem("Saturation");
    m_blockList->addItem("Rate Limiter");

    // 로직 블록
    m_blockList->addItem("Switch");
    m_blockList->addItem("AND");
    m_blockList->addItem("OR");
    m_blockList->addItem("NOT");
    m_blockList->addItem("XOR");*/

    // 블록 리스트에서 블록 선택 시 신호 연결
    connect(m_blockList, &BlockListWidget::blockTypeSelected, this, [this](const QString& blockType) {
        //QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
        //createBlockAt(blockType, center);
        // 대각선 위치 적용을 위해 onAddBlock 메서드 호출
        onAddBlock();
        m_blockList->setCurrentItem(nullptr); // 추가: 현재 선택 항목 지우기
    });

    QPushButton* addButton = new QPushButton("Add Block");
    connect(addButton, &QPushButton::clicked, this, &Similarlink::onAddBlock);

    dockLayout->addWidget(m_blockList);
    dockLayout->addWidget(addButton);
    m_blocksDock->setWidget(dockWidget);

    addDockWidget(Qt::LeftDockWidgetArea, m_blocksDock);

    // Add the view to the main layout
    mainLayout->addWidget(m_view);

    // Create menu and toolbar
    setupMenus();
    setupToolbar();

    // Status bar
    setupStatusBar(); // 상태 표시줄 설정 추가
    statusBar()->showMessage("Ready");


    // QGraphicsView에 드롭 활성화
    m_view->setAcceptDrops(true);

    // 뷰 이벤트 핸들링을 위한 이벤트 필터 설치 (기존 코드에 추가)
    m_view->viewport()->installEventFilter(this);




}

void Similarlink::setupMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* newAction = fileMenu->addAction("&New");
    connect(m_newAction, &QAction::triggered, this, &Similarlink::onNewModel);


    m_saveAction = fileMenu->addAction("&Save");
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &Similarlink::onSaveModel);

    m_saveAsAction = fileMenu->addAction("Save &As...");
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, &Similarlink::onSaveModelAs);


    m_loadAction = fileMenu->addAction("&Open");
    m_loadAction->setShortcut(QKeySequence::Open);
    connect(m_loadAction, &QAction::triggered, this, &Similarlink::onLoadModel);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit menu
    QMenu* editMenu = menuBar()->addMenu("&Edit");

    m_deleteAction = editMenu->addAction("&Delete");
    m_deleteAction->setEnabled(false);
    connect(m_deleteAction, &QAction::triggered, this, &Similarlink::onDeleteSelectedItems);

    // Simulation menu
    QMenu* simMenu = menuBar()->addMenu("&Simulation");

    m_startAction = simMenu->addAction("&Start");
    connect(m_startAction, &QAction::triggered, this, &Similarlink::onStartSimulation);

    m_stopAction = simMenu->addAction("Sto&p");
    m_stopAction->setEnabled(false);
    connect(m_stopAction, &QAction::triggered, this, &Similarlink::onStopSimulation);

    // 코드 생성 메뉴 추가
    QMenu* codeMenu = menuBar()->addMenu("&Code");

    QAction* generateAction = codeMenu->addAction("&Generate C++ Code");
    connect(generateAction, &QAction::triggered, this, &Similarlink::onGenerateCppCode);

    // Python 코드 생성 액션 추가
    QAction* generatePythonAction = codeMenu->addAction("Generate &Python Code");
    connect(generatePythonAction, &QAction::triggered, this, &Similarlink::onGeneratePythonCode);

    // CAPL 코드 생성 액션 추가
    QAction* generateCaplAction = codeMenu->addAction("Generate C&APL Code");
    connect(generateCaplAction, &QAction::triggered, this, &Similarlink::onGenerateCaplCode);


    // View 메뉴 추가
    QMenu* viewMenu = menuBar()->addMenu("&View");

    // 블록 라이브러리 표시/숨김 액션 추가
    QAction* toggleBlocksDockAction = viewMenu->addAction("&Block Library");
    toggleBlocksDockAction->setCheckable(true);
    toggleBlocksDockAction->setChecked(m_blocksDock->isVisible());
    connect(toggleBlocksDockAction, &QAction::triggered, this, [this](bool checked) {
        m_blocksDock->setVisible(checked);
    });

    // 도크 위젯 가시성 변경될 때 액션 체크 상태 업데이트
    connect(m_blocksDock, &QDockWidget::visibilityChanged, this, [toggleBlocksDockAction](bool visible) {
        toggleBlocksDockAction->setChecked(visible);
    });

}

void Similarlink::setupToolbar() {
    QToolBar* toolbar = addToolBar("Main Toolbar");

    // New, Save, Load
    toolbar->addAction(m_newAction);
    toolbar->addAction(m_saveAction);
    toolbar->addAction(m_loadAction);
    toolbar->addSeparator();

    // Delete selected
    toolbar->addAction(m_deleteAction);
    toolbar->addSeparator();

    // 시뮬레이션 지속 시간 설정 위젯 추가
    QLabel* durationLabel = new QLabel("Duration (s):");
    toolbar->addWidget(durationLabel);

    m_durationSpinBox = new QDoubleSpinBox();
    m_durationSpinBox->setRange(0.1, 1000.0);
    m_durationSpinBox->setValue(10.0); // 기본값 10초
    m_durationSpinBox->setSingleStep(1.0);
    m_durationSpinBox->setDecimals(1);
    m_durationSpinBox->setToolTip("Set simulation duration in seconds");
    toolbar->addWidget(m_durationSpinBox);

    // 지속 시간 변경 시 엔진에 알림
    connect(m_durationSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) {
                m_engine->setDuration(value);
            });


    // 시간 간격 설정 추가
    toolbar->addSeparator();
    QLabel* timeStepLabel = new QLabel("Time Step (s):");
    toolbar->addWidget(timeStepLabel);

    m_timeStepSpinBox = new QDoubleSpinBox();
    m_timeStepSpinBox->setRange(0.001, 0.1);
    m_timeStepSpinBox->setValue(0.01); // 기본값 10ms
    m_timeStepSpinBox->setSingleStep(0.001);
    m_timeStepSpinBox->setDecimals(3);
    m_timeStepSpinBox->setToolTip("Set simulation time step in seconds (1ms-100ms)");
    toolbar->addWidget(m_timeStepSpinBox);

    // 연결 설정
    connect(m_timeStepSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) {
                m_engine->setTimeStep(value);
            });

    // 재생 배속 설정 추가
    toolbar->addSeparator();
    QLabel* speedLabel = new QLabel("Playback Speed:");
    toolbar->addWidget(speedLabel);

    m_playbackSpeedComboBox = new QComboBox();
    m_playbackSpeedComboBox->addItem("0.1x", 0.1);
    m_playbackSpeedComboBox->addItem("0.25x", 0.25);
    m_playbackSpeedComboBox->addItem("0.5x", 0.5);
    m_playbackSpeedComboBox->addItem("1.0x", 1.0);
    m_playbackSpeedComboBox->addItem("2.0x", 2.0);
    m_playbackSpeedComboBox->addItem("4.0x", 4.0);
    m_playbackSpeedComboBox->addItem("10.0x", 10.0);
    m_playbackSpeedComboBox->setCurrentIndex(3); // 기본값 1.0x
    m_playbackSpeedComboBox->setToolTip("Set simulation playback speed");
    toolbar->addWidget(m_playbackSpeedComboBox);

    // 연결 설정
    connect(m_playbackSpeedComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                double speed = m_playbackSpeedComboBox->itemData(index).toDouble();
                m_engine->setPlaybackSpeed(speed);
            });


    // Simulation control
    toolbar->addSeparator();
    toolbar->addAction(m_startAction);
    toolbar->addAction(m_stopAction);


}

bool Similarlink::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_view->viewport()) {
        // 드래그 엔터 이벤트 - 드롭 가능 여부 결정
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent* dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasText()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        // 드래그 이동 이벤트 - 드래그 시 시각적 피드백 (선택적)
        else if (event->type() == QEvent::DragMove) {
            QDragMoveEvent* dragEvent = static_cast<QDragMoveEvent*>(event);
            if (dragEvent->mimeData()->hasText()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        // 드롭 이벤트 - 실제 블록 생성
        else if (event->type() == QEvent::Drop) {
            QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
            if (dropEvent->mimeData()->hasText()) {
                QString blockType = dropEvent->mimeData()->text();
                QPointF scenePos = m_view->mapToScene(dropEvent->pos());

                // 블록 생성 및 배치
                createBlockAt(blockType, scenePos);

                dropEvent->acceptProposedAction();
                return true;
            }
        }


        // 연결 모드 시작 시 드래그 모드 변경
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
                QGraphicsItem* item = m_scene->itemAt(scenePos, m_view->transform());

                Block* block = dynamic_cast<Block*>(item);
                if (block) {
                    int portIndex = -1;
                    // 출력 포트에서 클릭하면 연결 모드로 간주
                    if (block->containsOutputPort(scenePos, portIndex)) {
                        m_view->setDragMode(QGraphicsView::NoDrag); // 드래그 모드 비활성화
                    }
                }
            }
        }
        // 마우스 버튼 릴리즈 시 원래 드래그 모드로 복원
        else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_scene->isConnectionMode()) {
                // 연결 모드가 끝난 후 기본 드래그 모드로 복원
                QTimer::singleShot(10, this, [this]() {
                    m_view->setDragMode(QGraphicsView::RubberBandDrag);
                });
            }
        }
        // 더블 클릭 이벤트 처리 (기존 코드)
        else if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
            QGraphicsItem* item = m_scene->itemAt(scenePos, m_view->transform());

            if (item) {
                onBlockDoubleClicked(item);
                return true;
            }
        }
    }
    // 키보드 이벤트 처리 추가
    if (watched == m_view && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Delete 키를 눌렀을 때 선택된 항목 삭제
        if (keyEvent->key() == Qt::Key_Delete) {
            if (!m_scene->selectedItems().isEmpty()) {
                onDeleteSelectedItems();
                return true; // 이벤트 처리 완료
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
#include <iostream>
void Similarlink::createBlockAt(const QString& typeName, const QPointF& scenePos) {
    /*Block* block = nullptr;

    if (typeName == "Source") {
        block = new SourceBlock();
    } else if (typeName == "Gain") {
        block = new GainBlock();
    } else if (typeName == "Sum") {
        block = new SumBlock();
    } else if (typeName == "Product") {
        block = new ProductBlock();
    } else if (typeName == "Integrator") {
        block = new IntegratorBlock();
    } else if (typeName == "Derivative") {
        block = new DerivativeBlock();
    } else if (typeName == "Scope") {
        block = new ScopeBlock();
    } else if (typeName == "In") {    // 추가: In 블록
        block = new InBlock();
    } else if (typeName == "Out") {   // 추가: Out 블록
        block = new OutBlock();
    } else if (typeName == "Lookup Table 1D") {  // 추가: 1D Lookup Table
        block = new LookupTable1DBlock();
    } else if (typeName == "Lookup Table 2D") {  // 추가: 2D Lookup Table
        block = new LookupTable2DBlock();
    } else if (typeName == "Min") {            // 추가: Min 블록
        block = new MinBlock();
    } else if (typeName == "Max") {            // 추가: Max 블록
        block = new MaxBlock();
    } else if (typeName == "Saturation") {     // 추가: Saturation 블록
        block = new SaturationBlock();
    } else if (typeName == "Rate Limiter") {   // 추가: Rate Limiter 블록
        block = new RateLimiterBlock();
    } else if (typeName == "Switch") {         // 추가: Switch 블록
        block = new SwitchBlock();
    } else if (typeName == "Divide") {         // 추가: Divide 블록
        block = new DivideBlock();
    } else if (typeName == "AND") {            // 추가: AND 블록
        block = new AndBlock();
    } else if (typeName == "OR") {             // 추가: OR 블록
        block = new OrBlock();
    } else if (typeName == "NOT") {            // 추가: NOT 블록
        block = new NotBlock();
    } else if (typeName == "XOR") {            // 추가: XOR 블록
        block = new XorBlock();
    } else if (typeName == "Clock") {          // 추가: Clock 블록
        block = new ClockBlock();
    } else if (typeName == "Ramp") {           // 추가: Ramp 블록
        block = new RampBlock();
    } else if (typeName == "Step") {           // 추가: Step 블록
        block = new StepBlock();
    } else if (typeName == "Sine Wave") {      // 추가: Sine Wave 블록
        block = new SineWaveBlock();
    }

    if (block) {
        block->setPos(scenePos);
        m_scene->addItem(block);
    }*/

    // 블록 팩토리를 통해 블록 생성
    Block* block = BlockFactory::instance()->createBlockByName(typeName);
    std::cout << block->getName().toStdString() << std::endl;
    if (block) {
        // 고유한 이름 생성 및 설정
        QString uniqueName = generateUniqueBlockName(block->getName());
        block->setName(uniqueName);

        block->setPos(scenePos);
        m_scene->addItem(block);
        setModified(true);

        // 위치 기록
        m_lastBlockPosition = scenePos;
        m_hasLastPosition = true;
    }
}

// 코드 생성 메서드 추가
void Similarlink::onGenerateCppCode() {
    CppCodeGenerator generator(m_scene, this);
    QString code = generator.generateCode();

    // 코드 표시 다이얼로그
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Generated C++ Code");
    dialog->resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QTextEdit* textEdit = new QTextEdit(dialog);
    textEdit->setFont(QFont("Courier New", 10));
    textEdit->setPlainText(code);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* copyButton = new QPushButton("Copy to Clipboard", dialog);
    QPushButton* saveButton = new QPushButton("Save to File", dialog);
    QPushButton* closeButton = new QPushButton("Close", dialog);

    connect(copyButton, &QPushButton::clicked, [code]() {
        QApplication::clipboard()->setText(code);
    });

    connect(saveButton, &QPushButton::clicked, [this, code]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save C++ Code", "", "C++ Source (*.cpp)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << code;
                file.close();
            }
        }
    });

    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    dialog->setLayout(layout);
    dialog->exec();
}

// 파이썬 코드 생성 메서드 추가
void Similarlink::onGeneratePythonCode() {
    PythonCodeGenerator generator(m_scene, this);
    QString code = generator.generateCode();

    // 코드 표시 다이얼로그
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Generated Python Code");
    dialog->resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QTextEdit* textEdit = new QTextEdit(dialog);
    textEdit->setFont(QFont("Courier New", 10));
    textEdit->setPlainText(code);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* copyButton = new QPushButton("Copy to Clipboard", dialog);
    QPushButton* saveButton = new QPushButton("Save to File", dialog);
    QPushButton* closeButton = new QPushButton("Close", dialog);

    connect(copyButton, &QPushButton::clicked, [code]() {
        QApplication::clipboard()->setText(code);
    });

    connect(saveButton, &QPushButton::clicked, [this, code]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Python Code", "", "Python Files (*.py)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << code;
                file.close();
            }
        }
    });

    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    dialog->setLayout(layout);
    dialog->exec();
}

// CAPL 코드 생성
void Similarlink::onGenerateCaplCode() {
    CaplCodeGenerator generator(m_scene, this);
    QString code = generator.generateCode();

    // 코드 표시 다이얼로그
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Generated CAPL Code");
    dialog->resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QTextEdit* textEdit = new QTextEdit(dialog);
    textEdit->setFont(QFont("Courier New", 10));
    textEdit->setPlainText(code);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* copyButton = new QPushButton("Copy to Clipboard", dialog);
    QPushButton* saveButton = new QPushButton("Save to File", dialog);
    QPushButton* closeButton = new QPushButton("Close", dialog);

    connect(copyButton, &QPushButton::clicked, [code]() {
        QApplication::clipboard()->setText(code);
    });

    connect(saveButton, &QPushButton::clicked, [this, code]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save CAPL Code", "", "CAPL Files (*.can)");
        if (!fileName.isEmpty()) {
            if (!fileName.endsWith(".can"))
                fileName += ".can";

            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << code;
                file.close();
            }
        }
    });

    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    dialog->setLayout(layout);
    dialog->exec();
}

// 창 닫기 이벤트 재정의
void Similarlink::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        // 윈도우 및 도크 위젯 상태 저장
        QSettings settings("YourCompany", "SimulinkClone");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}

void Similarlink::readSettings() {
    // 윈도우 및 도크 위젯 상태 복원
    QSettings settings("YourCompany", "SimulinkClone");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void Similarlink::onAddBlock() {
    QString blockType = m_blockList->selectedBlockType();
    if (!blockType.isEmpty()) {
        QPointF center;

        if (m_hasLastPosition) {
            // 마지막 생성된 블록 위치에서 대각선 아래로 이동
            center = m_lastBlockPosition + QPointF(20, 20);
        } else {
            // 첫 블록이면 중앙에 배치
            center = m_view->mapToScene(m_view->viewport()->rect().center());
        }

        createBlockAt(blockType, center);
    }
}

void Similarlink::onDeleteSelectedItems() {
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();

    // 연결과 블록을 별도로 수집
    QList<Connection*> connectionsToRemove;
    QList<Block*> blocksToRemove;
    QList<QGraphicsItem*> otherItemsToRemove;

    for (auto item : selectedItems) {
        if (Connection* connection = dynamic_cast<Connection*>(item)) {
            connectionsToRemove.append(connection);
        } else if (Block* block = dynamic_cast<Block*>(item)) {
            blocksToRemove.append(block);
        } else {
            otherItemsToRemove.append(item);
        }
    }

    // 먼저 선택된 연결 제거
    for (auto connection : connectionsToRemove) {
        m_scene->removeItem(connection);
        delete connection;
    }

    // 블록 제거 - 블록에 연관된 연결은 SimulationScene::removeItem에서 처리
    for (auto block : blocksToRemove) {
        m_scene->removeItem(block);
        delete block;
    }

    // 기타 항목 제거
    for (auto item : otherItemsToRemove) {
        m_scene->removeItem(item);
        delete item;
    }

    // 삭제 후 선택 항목이 없으면 Delete 버튼 비활성화
    m_deleteAction->setEnabled(!m_scene->selectedItems().isEmpty());
}

void Similarlink::onBlockDoubleClicked(QGraphicsItem* item) {
    Block* block = dynamic_cast<Block*>(item);
    if (block) {
        if (block->getType() == Block::SCOPE) {
            ScopeBlock* scope = dynamic_cast<ScopeBlock*>(block);
            showScopeDialog(scope);
        } else {
            // 블록 유형에 맞는 속성 다이얼로그 생성
            QDialog* dialog = createBlockPropertyDialog(block, this);
            dialog->exec();
            delete dialog;
        }
    }
}

void Similarlink::onStartSimulation() {
    m_engine->start();
}

void Similarlink::onStopSimulation() {
    m_engine->stop();
}

void Similarlink::onSimulationStarted() {
    m_startAction->setEnabled(false);
    m_stopAction->setEnabled(true);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    statusBar()->showMessage("Simulation running...");
}

void Similarlink::onSimulationStopped() {
    m_startAction->setEnabled(true);
    m_stopAction->setEnabled(false);
    m_progressBar->setVisible(false);
    statusBar()->showMessage("Simulation stopped");
}

void Similarlink::onSimulationStepped(double time, const std::map<Block*, double>& outputs) {
    // 시간 표시 업데이트
    m_timeLabel->setText(QString("Time: %1 s").arg(time, 0, 'f', 3));

    // 진행 상태 표시줄 업데이트
    double duration = m_durationSpinBox->value();
    int progress = static_cast<int>((time / duration) * 1000);
    m_progressBar->setValue(std::min(progress, 1000));

    statusBar()->showMessage(QString("Simulation time: %1").arg(time));

    // Update scope displays
    for (auto item : m_scene->items()) {
        ScopeBlock* scope = dynamic_cast<ScopeBlock*>(item);
        if (scope && outputs.find(scope) != outputs.end()) {
            // In a real application, you would update a chart or display
            // For this demonstration, we just update the status bar
            statusBar()->showMessage(
                QString("Simulation time: %1, Scope '%2' value: %3")
                    .arg(time).arg(scope->getName()).arg(outputs.at(scope)));
        }
    }
}

// 새 모델 생성
void Similarlink::onNewModel() {
    if (maybeSave()) {
        m_scene->clear();
        m_currentFilePath = "";
        setModified(false);

        // 위치 초기화
        m_hasLastPosition = false;

        updateWindowTitle();
    }
}

void Similarlink::onSaveModel() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Model", "", "Model Files (*.json)");
    if (fileName.isEmpty()) return;

    QJsonObject root;
    QJsonArray blocksArray;
    QJsonArray connectionsArray;

    // Save blocks
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block) {
            QJsonObject blockObj;
            blockObj["type"] = static_cast<int>(block->getType());
            blockObj["name"] = block->getName();
            blockObj["x"] = block->pos().x();
            blockObj["y"] = block->pos().y();

            QJsonObject propsObj;
            QMap<QString, QVariant> props = block->getProperties();
            for (auto it = props.begin(); it != props.end(); ++it) {
                propsObj[it.key()] = QJsonValue::fromVariant(it.value());
            }
            blockObj["properties"] = propsObj;

            blocksArray.append(blockObj);
        }
    }

    // Save connections
    for (auto connection : m_scene->getConnections()) {
        QJsonObject connObj;
        connObj["sourceBlockName"] = connection->getSourceBlock()->getName();
        connObj["sourcePortIndex"] = connection->getSourcePortIndex();
        connObj["destBlockName"] = connection->getDestBlock()->getName();
        connObj["destPortIndex"] = connection->getDestPortIndex();

        connectionsArray.append(connObj);
    }

    root["blocks"] = blocksArray;
    root["connections"] = connectionsArray;

    QJsonDocument doc(root);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        statusBar()->showMessage("Model saved");
    } else {
        QMessageBox::warning(this, "Error", "Could not save the model");
    }
}

// 다른 이름으로 저장
void Similarlink::onSaveModelAs() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Model", "", "Model Files (*.json)");
    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(".json"))
        fileName += ".json";

    if (saveModelToFile(fileName)) {
        m_currentFilePath = fileName;
        setModified(false);
        updateWindowTitle();
        statusBar()->showMessage("Model saved to " + fileName, 3000);
    }
}
// 파일에 모델 저장 (실제 저장 로직)
bool Similarlink::saveModelToFile(const QString& filePath) {
    QJsonObject root;
    QJsonArray blocksArray;
    QJsonArray connectionsArray;

    // Save blocks
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block) {
            QJsonObject blockObj;
            blockObj["type"] = static_cast<int>(block->getType());
            blockObj["name"] = block->getName();
            blockObj["x"] = block->pos().x();
            blockObj["y"] = block->pos().y();

            QJsonObject propsObj;
            QMap<QString, QVariant> props = block->getProperties();
            for (auto it = props.begin(); it != props.end(); ++it) {
                propsObj[it.key()] = QJsonValue::fromVariant(it.value());
            }
            blockObj["properties"] = propsObj;

            blocksArray.append(blockObj);
        }
    }

    // Save connections
    for (auto connection : m_scene->getConnections()) {
        QJsonObject connObj;
        connObj["sourceBlockName"] = connection->getSourceBlock()->getName();
        connObj["sourcePortIndex"] = connection->getSourcePortIndex();
        connObj["destBlockName"] = connection->getDestBlock()->getName();
        connObj["destPortIndex"] = connection->getDestPortIndex();

        connectionsArray.append(connObj);
    }

    root["blocks"] = blocksArray;
    root["connections"] = connectionsArray;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    } else {
        QMessageBox::warning(this, "Error", "Could not save the model to " + filePath);
        return false;
    }
}

// 모델 불러오기
void Similarlink::onLoadModel() {
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this, "Open Model", "", "Model Files (*.json)");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject root = doc.object();

            // Clear existing scene
            m_scene->clear();

            // 위치 초기화
            m_hasLastPosition = false;

            // Create blocks
            QMap<QString, Block*> blockMap;
            QJsonArray blocksArray = root["blocks"].toArray();
            for (const QJsonValue& value : blocksArray) {
                QJsonObject blockObj = value.toObject();
                Block::BlockType type = static_cast<Block::BlockType>(blockObj["type"].toInt());
                QString name = blockObj["name"].toString();
                qreal x = blockObj["x"].toDouble();
                qreal y = blockObj["y"].toDouble();

                // 블록 팩토리를 통해 블록 생성
                Block* block = BlockFactory::instance()->createBlock(type, name);

                if (block) {
                    QJsonObject propsObj = blockObj["properties"].toObject();
                    QMap<QString, QVariant> props;
                    for (auto it = propsObj.begin(); it != propsObj.end(); ++it) {
                        props[it.key()] = it.value().toVariant();
                    }
                    block->setProperties(props);

                    block->setPos(x, y);
                    m_scene->addItem(block);
                    blockMap[name] = block;
                }
            }

            // Create connections
            QJsonArray connectionsArray = root["connections"].toArray();
            for (const QJsonValue& value : connectionsArray) {
                QJsonObject connObj = value.toObject();
                QString sourceBlockName = connObj["sourceBlockName"].toString();
                int sourcePortIndex = connObj["sourcePortIndex"].toInt();
                QString destBlockName = connObj["destBlockName"].toString();
                int destPortIndex = connObj["destPortIndex"].toInt();

                if (blockMap.contains(sourceBlockName) && blockMap.contains(destBlockName)) {
                    Connection* connection = new Connection(
                        blockMap[sourceBlockName], sourcePortIndex,
                        blockMap[destBlockName], destPortIndex);
                    m_scene->addItem(connection);
                }
            }

            m_currentFilePath = fileName;
            setModified(false);
            updateWindowTitle();
            statusBar()->showMessage("Model loaded from " + fileName, 3000);
        } else {
            QMessageBox::warning(this, "Error", "Could not open the model from " + fileName);
        }
    }
}

// 파일 수정 상태 설정
void Similarlink::setModified(bool modified) {
    if (m_isModified != modified) {
        m_isModified = modified;
        updateWindowTitle();
    }
}

// 창 제목 업데이트
void Similarlink::updateWindowTitle() {
    QString title = "Qt Simulink Clone";

    if (m_currentFilePath.isEmpty()) {
        title = "Untitled - " + title;
    } else {
        QFileInfo fileInfo(m_currentFilePath);
        title = fileInfo.fileName() + " - " + title;
    }

    if (m_isModified) {
        title = "*" + title;
    }

    setWindowTitle(title);
}

// 씬 변경 감지
void Similarlink::onSceneChanged() {
    setModified(true);
}

// 저장 확인
bool Similarlink::maybeSave() {
    if (!m_isModified)
        return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(
        this, "Qt Simulink Clone",
        "The document has been modified.\nDo you want to save your changes?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return onSaveModel(), true;
    else if (ret == QMessageBox::Cancel)
        return false;

    return true;  // Discard
}


// 블록 이름 중복 체크 함수 구현
bool Similarlink::isBlockNameExists(const QString& name) const {
    // 씬의 모든 블록을 순회하며 이름 비교
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block && block->getName() == name) {
            return true;
        }
    }
    return false;
}

// 고유한 블록 이름 생성 함수 구현
QString Similarlink::generateUniqueBlockName(const QString& baseName) const {
    // 기본 이름이 중복되지 않으면 그대로 사용
    if (!isBlockNameExists(baseName)) {
        return baseName;
    }

    // 중복될 경우 숫자를 붙여 고유한 이름 생성
    int counter = 1;
    QString newName;
    do {
        newName = QString("%1_%2").arg(baseName).arg(counter);
        counter++;
    } while (isBlockNameExists(newName));

    return newName;
}
