#ifndef SUBSYSTEMDIALOG_H
#define SUBSYSTEMDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>

#include "subsystemblock.h"

class SubsystemDialog : public QDialog {
    Q_OBJECT

public:
    SubsystemDialog(SubsystemBlock* block, QWidget* parent = nullptr);

    // 서브시스템 블록 설정
    void setSubsystemBlock(SubsystemBlock* block);

    // 서브시스템 정의 방법 (내장/외부 선택)
    void setEmbeddedMode(bool embedded);

private slots:
    void onModeChanged(int index);
    void onBrowseClicked();
    void onButtonClicked(QAbstractButton* button);
    void onCreateNewClicked();
    void onEditContentClicked();

private:
    // 서브시스템 블록에 선택한 설정 적용
    void applySettings();

private:
    SubsystemBlock* m_block;

    // UI 요소들
    QComboBox* m_modeCombo;
    QLineEdit* m_pathEdit;
    QPushButton* m_browseButton;
    QPushButton* m_createNewButton;
    QPushButton* m_editContentButton;
    QDialogButtonBox* m_buttonBox;

    QVBoxLayout* m_embeddedLayout;
    QVBoxLayout* m_externalLayout;
signals:
    // 서브시스템 편집 요청 신호
    void editSubsystemRequested(SubsystemBlock* subsystem);
};

#endif // SUBSYSTEMDIALOG_H
