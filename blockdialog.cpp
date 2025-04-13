#include "blockdialog.h"
#include "similarlink.h"

BlockPropertyDialog::BlockPropertyDialog(Block* block, QWidget* parent)
    : QDialog(parent), m_block(block), m_originalName(block->getName()) {
    setWindowTitle("Block Properties");

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Name field
    QFormLayout* formLayout = new QFormLayout();
    m_nameEdit = new QLineEdit(block->getName());
    formLayout->addRow("Name:", m_nameEdit);

    // Block-specific properties
    QMap<QString, QVariant> props = block->getProperties();
    for (auto it = props.begin(); it != props.end(); ++it) {
        QLineEdit* edit = new QLineEdit(it.value().toString());
        formLayout->addRow(it.key() + ":", edit);
        m_propertyEdits[it.key()] = edit;
    }

    layout->addLayout(formLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Cancel");

    // 이름 변경 시 중복 체크
    connect(m_nameEdit, &QLineEdit::textChanged, this, &BlockPropertyDialog::validateName);

    connect(m_okButton, &QPushButton::clicked, this, &BlockPropertyDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &BlockPropertyDialog::reject);

    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // 초기 이름 유효성 체크
    validateName(m_nameEdit->text());
}

void BlockPropertyDialog::validateName(const QString &name) {
    // 빈 이름인 경우
    if (name.isEmpty()) {
        m_nameEdit->setStyleSheet("background-color: #FFAAAA;");
        m_okButton->setEnabled(false);
        return;
    }

    // 원래 이름과 같으면 중복 검사 불필요
    if (name == m_originalName) {
        m_nameEdit->setStyleSheet("");
        m_okButton->setEnabled(true);
        return;
    }

    // 부모 윈도우 찾기
    Similarlink* mainWindow = qobject_cast<Similarlink*>(parentWidget());
    if (!mainWindow) {
        // 부모 윈도우를 찾을 수 없으면 중복 체크 생략
        m_nameEdit->setStyleSheet("");
        m_okButton->setEnabled(true);
        return;
    }

    // 이름 중복 체크
    if (mainWindow->isBlockNameExists(name)) {
        m_nameEdit->setStyleSheet("background-color: #FFAAAA;");
        m_okButton->setEnabled(false);
    } else {
        m_nameEdit->setStyleSheet("");
        m_okButton->setEnabled(true);
    }
}

void BlockPropertyDialog::accept() {
    // Update block properties
    m_block->setName(m_nameEdit->text());

    QMap<QString, QVariant> props = m_block->getProperties();
    for (auto it = m_propertyEdits.begin(); it != m_propertyEdits.end(); ++it) {
        props[it.key()] = QVariant(it.value()->text());
    }
    m_block->setProperties(props);

    QDialog::accept();
}

