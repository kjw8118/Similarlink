#include "subsystemdialog.h"

SubsystemDialog::SubsystemDialog(SubsystemBlock* block, QWidget* parent)
    : QDialog(parent), m_block(block) {

    setWindowTitle("Subsystem Properties");
    resize(500, 300);

    // 메인 레이아웃
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 서브시스템 이름
    QHBoxLayout* nameLayout = new QHBoxLayout();
    QLabel* nameLabel = new QLabel("Name:", this);
    QLineEdit* nameEdit = new QLineEdit(block ? block->getName() : "Subsystem", this);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    mainLayout->addLayout(nameLayout);

    // 구분선
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // 서브시스템 모드 선택
    QLabel* modeLabel = new QLabel("Subsystem Definition:", this);
    mainLayout->addWidget(modeLabel);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("External File", false);
    m_modeCombo->addItem("Embedded Model", true);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SubsystemDialog::onModeChanged);
    mainLayout->addWidget(m_modeCombo);

    // 외부 파일 모드 UI
    m_externalLayout = new QVBoxLayout();
    QHBoxLayout* pathLayout = new QHBoxLayout();
    QLabel* pathLabel = new QLabel("File Path:", this);
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setReadOnly(true);
    m_browseButton = new QPushButton("Browse...", this);
    connect(m_browseButton, &QPushButton::clicked, this, &SubsystemDialog::onBrowseClicked);

    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(m_browseButton);
    m_externalLayout->addLayout(pathLayout);
    mainLayout->addLayout(m_externalLayout);

    // 내장 모델 모드 UI
    m_embeddedLayout = new QVBoxLayout();
    QLabel* embeddedLabel = new QLabel("The subsystem model will be embedded within this file.", this);
    m_createNewButton = new QPushButton("Create New Model", this);
    m_editContentButton = new QPushButton("Edit Subsystem Content", this);

    connect(m_createNewButton, &QPushButton::clicked, this, &SubsystemDialog::onCreateNewClicked);
    connect(m_editContentButton, &QPushButton::clicked, this, &SubsystemDialog::onEditContentClicked);

    m_embeddedLayout->addWidget(embeddedLabel);
    m_embeddedLayout->addWidget(m_createNewButton);
    m_embeddedLayout->addWidget(m_editContentButton);
    mainLayout->addLayout(m_embeddedLayout);

    // 버튼 박스
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::clicked, this, &SubsystemDialog::onButtonClicked);
    mainLayout->addWidget(m_buttonBox);

    // 초기 상태 설정
    if (block) {
        bool isEmbedded = !block->isExternalFile();
        m_modeCombo->setCurrentIndex(isEmbedded ? 1 : 0);
        m_pathEdit->setText(block->getSubsystemPath());
    } else {
        m_modeCombo->setCurrentIndex(0); // 기본값: 외부 파일
    }

    // 초기 UI 상태 업데이트
    onModeChanged(m_modeCombo->currentIndex());
}

void SubsystemDialog::setSubsystemBlock(SubsystemBlock* block) {
    m_block = block;

    if (block) {
        bool isEmbedded = !block->isExternalFile();
        m_modeCombo->setCurrentIndex(isEmbedded ? 1 : 0);
        m_pathEdit->setText(block->getSubsystemPath());
    }
}

void SubsystemDialog::setEmbeddedMode(bool embedded) {
    m_modeCombo->setCurrentIndex(embedded ? 1 : 0);
}

void SubsystemDialog::onModeChanged(int index) {
    bool isEmbedded = m_modeCombo->currentData().toBool();

    // UI 요소 표시/숨김 설정
    for (int i = 0; i < m_externalLayout->count(); ++i) {
        QLayoutItem* item = m_externalLayout->itemAt(i);
        if (item->widget()) {
            item->widget()->setVisible(!isEmbedded);
        } else if (item->layout()) {
            for (int j = 0; j < item->layout()->count(); ++j) {
                QLayoutItem* subItem = item->layout()->itemAt(j);
                if (subItem->widget()) {
                    subItem->widget()->setVisible(!isEmbedded);
                }
            }
        }
    }

    for (int i = 0; i < m_embeddedLayout->count(); ++i) {
        QLayoutItem* item = m_embeddedLayout->itemAt(i);
        if (item->widget()) {
            item->widget()->setVisible(isEmbedded);
        }
    }

    // 편집 버튼 상태 업데이트
    m_editContentButton->setEnabled(m_block && (isEmbedded || !m_pathEdit->text().isEmpty()));
}

void SubsystemDialog::onBrowseClicked() {
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "Select Subsystem File",
                                                    "",
                                                    "Model Files (*.json)");
    if (!filePath.isEmpty()) {
        m_pathEdit->setText(filePath);

        // 편집 버튼 활성화
        m_editContentButton->setEnabled(true);
    }
}

void SubsystemDialog::onButtonClicked(QAbstractButton* button) {
    QDialogButtonBox::StandardButton stdButton = m_buttonBox->standardButton(button);

    if (stdButton == QDialogButtonBox::Ok) {
        applySettings();
        accept();
    } else if (stdButton == QDialogButtonBox::Cancel) {
        reject();
    }
}

void SubsystemDialog::onCreateNewClicked() {
    if (!m_block) {
        QMessageBox::warning(this, "Error", "No subsystem block set.");
        return;
    }

    // 기본 빈 모델 생성
    QJsonObject emptyModel = {
        {"blocks", QJsonArray()},
        {"connections", QJsonArray()}
    };

    m_block->setEmbeddedModel(emptyModel);

    // 모델 편집 요청 신호 발생
    emit editSubsystemRequested(m_block);

    // 편집 버튼 활성화
    m_editContentButton->setEnabled(true);
}

void SubsystemDialog::onEditContentClicked() {
    if (!m_block) {
        QMessageBox::warning(this, "Error", "No subsystem block set.");
        return;
    }

    bool isEmbedded = m_modeCombo->currentData().toBool();

    if (isEmbedded) {
        // 이미 내장 모델이 있으면 편집 요청
        emit editSubsystemRequested(m_block);
    } else {
        // 외부 파일 모드: 파일 경로 확인
        QString filePath = m_pathEdit->text();
        if (filePath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please select a subsystem file first.");
            return;
        }

        // 파일 로드 후 편집 요청
        if (m_block->loadFromFile(filePath)) {
            emit editSubsystemRequested(m_block);
        } else {
            QMessageBox::critical(this, "Error", "Failed to load subsystem from file.");
        }
    }
}

void SubsystemDialog::applySettings() {
    if (!m_block) return;

    // 이름 업데이트
    QLineEdit* nameEdit = findChild<QLineEdit*>();
    if (nameEdit) {
        m_block->setName(nameEdit->text());
    }

    // 모드 설정
    bool isEmbedded = m_modeCombo->currentData().toBool();

    if (isEmbedded) {
        // 내장 모델 모드
        if (m_block->getEmbeddedModel().isEmpty()) {
            // 빈 모델 생성
            QJsonObject emptyModel = {
                {"blocks", QJsonArray()},
                {"connections", QJsonArray()}
            };
            m_block->setEmbeddedModel(emptyModel);
        }
    } else {
        // 외부 파일 모드
        QString filePath = m_pathEdit->text();
        if (!filePath.isEmpty()) {
            m_block->setSubsystemPath(filePath);
        }
    }
}
