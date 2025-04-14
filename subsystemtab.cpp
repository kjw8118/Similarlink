#include "subsystemtab.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QLineEdit>

#include "blockfactory.h"

#include "similarlink.h"

SubsystemTab::SubsystemTab(SubsystemBlock* subsystemBlock, QWidget* parent)
    : QWidget(parent), m_subsystemBlock(subsystemBlock), m_isModified(false) {

    // 탭 이름 설정
    if (subsystemBlock) {
        m_tabName = subsystemBlock->getName();
    } else {
        m_tabName = "Subsystem";
    }

    setupUI();

    // 초기 모델 로드
    if (subsystemBlock) {
        loadModelFromSubsystem();
    }
}

SubsystemTab::~SubsystemTab() {
    // 필요한 정리 작업
}

QString SubsystemTab::getTabName() const {
    return m_tabName;
}

SubsystemBlock* SubsystemTab::getSubsystemBlock() const {
    return m_subsystemBlock;
}

bool SubsystemTab::hasUnsavedChanges() const {
    return m_isModified;
}

bool SubsystemTab::saveModel() {
    if (!m_subsystemBlock) {
        return false;
    }

    // 현재 편집 중인 모델을 서브시스템 블록에 적용
    applyChangesToSubsystem();

    // 외부 파일이면 파일로 저장
    if (m_subsystemBlock->isExternalFile()) {
        QString filePath = m_subsystemBlock->getSubsystemPath();
        if (!filePath.isEmpty()) {
            if (m_subsystemBlock->saveToFile(filePath)) {
                m_isModified = false;
                return true;
            }
        }
    } else {
        // 내장 모델이면 이미 적용되었으므로 성공
        m_isModified = false;
        return true;
    }

    return false;
}

void SubsystemTab::loadModelFromSubsystem() {
    if (!m_subsystemBlock) {
        return;
    }

    // 씬 클리어
    m_scene->clear();

    // 서브시스템 모델 가져오기
    QJsonObject model;
    if (m_subsystemBlock->isExternalFile()) {
        // 외부 파일에서 로드
        QString filePath = m_subsystemBlock->getSubsystemPath();
        if (!filePath.isEmpty()) {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                file.close();

                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (!doc.isNull() && doc.isObject()) {
                    model = doc.object();
                }
            }
        }
    } else {
        // 내장 모델 사용
        model = m_subsystemBlock->getEmbeddedModel();
    }

    // 모델이 비어있으면 기본 모델 생성
    if (model.isEmpty()) {
        model = QJsonObject{
            {"blocks", QJsonArray()},
            {"connections", QJsonArray()}
        };
    }

    // 블록 생성
    QMap<QString, Block*> blockMap;
    QJsonArray blocksArray = model["blocks"].toArray();

    for (const QJsonValue& value : blocksArray) {
        QJsonObject blockObj = value.toObject();
        Block::BlockType type = static_cast<Block::BlockType>(blockObj["type"].toInt());
        QString name = blockObj["name"].toString();
        qreal x = blockObj["x"].toDouble();
        qreal y = blockObj["y"].toDouble();

        // 블록 생성
        Block* block = BlockFactory::instance()->createBlock(type, name);
        if (block) {
            // 속성 설정
            QJsonObject propsObj = blockObj["properties"].toObject();
            QMap<QString, QVariant> props;
            for (auto it = propsObj.begin(); it != propsObj.end(); ++it) {
                props[it.key()] = it.value().toVariant();
            }
            block->setProperties(props);

            // 위치 설정 및 씬에 추가
            block->setPos(x, y);
            m_scene->addItem(block);
            blockMap[name] = block;
        }
    }

    // 연결 생성
    QJsonArray connectionsArray = model["connections"].toArray();
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

    m_isModified = false;
}

void SubsystemTab::applyChangesToSubsystem() {
    if (!m_subsystemBlock) {
        return;
    }

    // 현재 씬의 블록과 연결을 JSON으로 직렬화
    QJsonObject model;
    QJsonArray blocksArray;
    QJsonArray connectionsArray;

    // 블록 직렬화
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block) {
            QJsonObject blockObj;
            blockObj["type"] = static_cast<int>(block->getType());
            blockObj["name"] = block->getName();
            blockObj["x"] = block->pos().x();
            blockObj["y"] = block->pos().y();

            // 속성 직렬화
            QJsonObject propsObj;
            QMap<QString, QVariant> props = block->getProperties();
            for (auto it = props.begin(); it != props.end(); ++it) {
                propsObj[it.key()] = QJsonValue::fromVariant(it.value());
            }
            blockObj["properties"] = propsObj;

            blocksArray.append(blockObj);
        }
    }

    // 연결 직렬화
    for (auto connection : m_scene->getConnections()) {
        QJsonObject connObj;
        connObj["sourceBlockName"] = connection->getSourceBlock()->getName();
        connObj["sourcePortIndex"] = connection->getSourcePortIndex();
        connObj["destBlockName"] = connection->getDestBlock()->getName();
        connObj["destPortIndex"] = connection->getDestPortIndex();

        connectionsArray.append(connObj);
    }

    model["blocks"] = blocksArray;
    model["connections"] = connectionsArray;

    // 모델을 서브시스템 블록에 적용
    if (m_subsystemBlock->isExternalFile()) {
        // 외부 파일 모드면 파일 저장은 saveModel()에서 처리
        // 여기서는 내부 모델만 업데이트
        m_subsystemBlock->setEmbeddedModel(model);
    } else {
        // 내장 모델 모드면 내장 모델 업데이트
        m_subsystemBlock->setEmbeddedModel(model);
    }

    // 포트 업데이트
    m_subsystemBlock->updatePortsFromSubsystem();
}

void SubsystemTab::closeEvent(QCloseEvent* event) {
    if (m_isModified) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, "Save Changes",
            "Do you want to save changes to this subsystem?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (result == QMessageBox::Yes) {
            if (!saveModel()) {
                // 저장 실패
                QMessageBox::critical(this, "Error", "Failed to save the subsystem.");
                event->ignore();
                return;
            }
        } else if (result == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    // 닫기 요청 신호 발생
    emit closeRequested(this);
    event->accept();
}

void SubsystemTab::onSceneChanged() {
    if (!m_isModified) {
        m_isModified = true;
        emit contentsChanged();
    }
}

void SubsystemTab::onBlockDoubleClicked(QGraphicsItem* item) {
    Block* block = dynamic_cast<Block*>(item);
    if (block) {
        // 블록 속성 다이얼로그 열기
        // 메인윈도우의 createBlockPropertyDialog() 메서드를 사용하는 것이 좋음
        // 여기서는 간단히 처리
        if (block->getType() == Block::SUBSYSTEM) {
            // 서브시스템 블록 더블클릭 시 중첩된 서브시스템 편집
            SubsystemBlock* subsystemBlock = dynamic_cast<SubsystemBlock*>(block);
            if (subsystemBlock) {
                // 중첩 서브시스템 편집 요청 (메인윈도우에서 처리해야 함)
                // 여기서는 신호를 발생시킬 수 있음
                emit editNestedSubsystem(subsystemBlock);
            }
        } else {
            // 일반 블록 속성 편집
            QDialog* dialog = createBlockPropertyDialog(block, this);
            dialog->exec();
            delete dialog;
        }
    }
}

void SubsystemTab::onSaveClicked() {
    saveModel();
}
// 블록 추가 메서드 추가
void SubsystemTab::createBlockAt(const QString& typeName, const QPointF& scenePos) {
    // 블록 팩토리를 통해 블록 생성
    Block* block = BlockFactory::instance()->createBlockByName(typeName);
    if (block) {
        // 고유한 이름 생성 및 설정
        QString uniqueName = generateUniqueBlockName(block->getName());
        block->setName(uniqueName);

        block->setPos(scenePos);
        m_scene->addItem(block);
        m_isModified = true;
        emit contentsChanged();

        // 위치 기록
        m_lastBlockPosition = scenePos;
        m_hasLastPosition = true;
    }
}

// 고유한 블록 이름 생성 (Similarlink와 유사한 로직)
QString SubsystemTab::generateUniqueBlockName(const QString& baseName) const {
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

// 블록 이름 중복 확인
bool SubsystemTab::isBlockNameExists(const QString& name) const {
    // 씬의 모든 블록을 순회하며 이름 비교
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block && block->getName() == name) {
            return true;
        }
    }
    return false;
}
#include <iostream>
void SubsystemTab::setupUI() {
    // 메인 레이아웃
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 시뮬레이션 씬 생성 - 가장 먼저 초기화
    m_scene = new SimulationScene(this);
    m_scene->setSceneRect(QRectF(0, 0, 2000, 2000));
    connect(m_scene, &QGraphicsScene::changed, this, &SubsystemTab::onSceneChanged);

    // 뷰 생성
    m_view = new SimulationView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);

    // 블록 더블클릭 이벤트 처리
    m_view->viewport()->installEventFilter(this);

    // 뷰 드래그 앤 드롭 설정 추가
    m_view->setAcceptDrops(true);

    // 키보드 이벤트 포커스 정책 설정
    m_view->setFocusPolicy(Qt::StrongFocus);

    // 이벤트 필터 설치
    m_view->installEventFilter(this);

    // 이벤트 처리 연결
    connect(m_view, &QGraphicsView::rubberBandChanged, this, [this](const QRect&, const QPointF&, const QPointF&) {
        // 필요한 처리
    });

    // 이제 씬과 뷰가 모두 초기화되었으므로 툴바 설정
    setupToolbar();
    mainLayout->addWidget(m_toolbar);

    // 뷰 추가
    mainLayout->addWidget(m_view);

    setLayout(mainLayout);

    // 마지막 블록 위치 초기화
    m_hasLastPosition = false;

    // 선택 항목 변경 감지 연결 - 모든 객체가 생성된 후에 추가
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        if (m_deleteAction) {
            m_deleteAction->setEnabled(!m_scene->selectedItems().isEmpty());
        }
    });
}

void SubsystemTab::setupToolbar() {
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);

    // 저장 액션
    m_saveAction = new QAction(QIcon::fromTheme("document-save"), "Save", this);
    connect(m_saveAction, &QAction::triggered, this, &SubsystemTab::onSaveClicked);
    m_toolbar->addAction(m_saveAction);

    m_toolbar->addSeparator();

    // 삭제 액션 추가
    m_deleteAction = new QAction(QIcon::fromTheme("edit-delete"), "Delete", this);
    m_deleteAction->setEnabled(false);  // 초기에는 비활성화 상태로 시작
    connect(m_deleteAction, &QAction::triggered, this, &SubsystemTab::onDeleteSelectedItems);
    m_toolbar->addAction(m_deleteAction);

    m_toolbar->addSeparator();

    // 닫기 액션
    m_closeAction = new QAction(QIcon::fromTheme("window-close"), "Close", this);
    connect(m_closeAction, &QAction::triggered, this, [this]() {
        emit closeRequested(this);
    });
    m_toolbar->addAction(m_closeAction);

    // 여기서는 m_scene에 관련된 연결을 하지 않음
    // 선택 항목 변경 감지 연결은 setupUI() 메서드의 끝부분에서 처리
}

// 블록 팩토리를 통한 속성 다이얼로그 생성 (확장된 구현)
QDialog* SubsystemTab::createBlockPropertyDialog(Block* block, QWidget* parent) {
    if (!block) return nullptr;

    // 메인 윈도우에서 다이얼로그 생성 기능 재사용 (Similarlink 인스턴스 접근)
    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(parent->window());
    Similarlink* similarlink = qobject_cast<Similarlink*>(mainWindow);

    if (similarlink) {
        return similarlink->createBlockPropertyDialog(block, parent);
    } else {
        // 메인 윈도우 접근 불가 시 기본 다이얼로그 생성 (기존 코드)
        QDialog* dialog = new QDialog(parent);
        dialog->setWindowTitle("Block Properties: " + block->getName());

        QVBoxLayout* layout = new QVBoxLayout(dialog);

        // 블록 타입에 따른 속성 위젯 추가
        QMap<QString, QVariant> props = block->getProperties();
        for (auto it = props.begin(); it != props.end(); ++it) {
            QHBoxLayout* propLayout = new QHBoxLayout();
            QLabel* label = new QLabel(it.key() + ":", dialog);
            QLineEdit* edit = new QLineEdit(it.value().toString(), dialog);

            propLayout->addWidget(label);
            propLayout->addWidget(edit);
            layout->addLayout(propLayout);
        }

        QDialogButtonBox* buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            dialog);
        connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

        layout->addWidget(buttonBox);
        dialog->setLayout(layout);

        return dialog;
    }
}
// 탭 이름이 변경될 때 호출되는 메서드
void SubsystemTab::setTabName(const QString& name) {
    if (m_tabName != name) {
        m_tabName = name;
        // 여기서 필요한 추가 작업 수행 (예: 부모 탭 위젯에 이름 업데이트 요청)
    }
}

// 탭 데이터 갱신 메서드 (서브시스템 블록의 내용이 외부에서 변경된 경우)
void SubsystemTab::refreshFromSubsystemBlock() {
    if (!m_subsystemBlock) return;

    // 서브시스템 블록의 이름으로 탭 이름 동기화
    m_tabName = m_subsystemBlock->getName();

    // 모델 다시 로드
    loadModelFromSubsystem();

    // 수정 상태 초기화
    m_isModified = false;
}
// 이벤트 필터 구현 - 메인윈도우의 구현과 유사하게
bool SubsystemTab::eventFilter(QObject* watched, QEvent* event) {
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
        // 더블 클릭 이벤트 처리
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

    return QWidget::eventFilter(watched, event);
}

// 선택된 항목 삭제 메서드 구현
void SubsystemTab::onDeleteSelectedItems() {
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

    // 삭제 후 상태 업데이트
    m_isModified = true;
    emit contentsChanged();

    // 삭제 버튼 상태 업데이트
    m_deleteAction->setEnabled(!m_scene->selectedItems().isEmpty());
}

// 중첩 서브시스템 편집 처리
void SubsystemTab::onEditNestedSubsystem(SubsystemBlock* subsystem) {
    if (!subsystem) return;

    // 중첩 서브시스템 편집 요청을 상위로 전달
    emit editNestedSubsystem(subsystem);
}

// 현재 탭이 활성화될 때 호출되는 메서드 (추가 구현)
void SubsystemTab::activate() {
    // 탭 활성화 시 필요한 초기화 작업 수행
    // 현재 뷰에 포커스 설정
    if (m_view) {
        m_view->setFocus();
    }
}
