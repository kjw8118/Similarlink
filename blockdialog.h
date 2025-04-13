#ifndef BLOCKDIALOG_H
#define BLOCKDIALOG_H

#include <QTabWidget>
#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include <QPaintEvent>

#include <QTimer>

#include "block.h"


#include "tabledataeditor.h"
#include "signalpreviewwidget.h"


class BlockPropertyDialog : public QDialog {
    //Q_OBJECT
public:
    BlockPropertyDialog(Block* block, QWidget* parent = nullptr);

    void accept() override;

private slots:
    void validateName(const QString &name);

private:
    Block* m_block;
    QLineEdit* m_nameEdit;
    QMap<QString, QLineEdit*> m_propertyEdits;
    QString m_originalName;  // 원래 블록 이름 저장
    QPushButton* m_okButton; // OK 버튼 참조 저장
};




class LimiterPropertyDialog : public QDialog {
public:
    LimiterPropertyDialog(Block* block, QWidget* parent = nullptr)
        : QDialog(parent), m_block(block) {

        bool isRateLimiter = (block->getType() == Block::RATE_LIMITER);

        setWindowTitle(isRateLimiter ? "Rate Limiter Properties" : "Saturation Properties");
        resize(350, 250);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 이름 필드
        QFormLayout* formLayout = new QFormLayout();
        m_nameEdit = new QLineEdit(block->getName());
        formLayout->addRow("Name:", m_nameEdit);

        mainLayout->addLayout(formLayout);

        // 그룹 박스로 제한 설정 포함
        QGroupBox* limiterGroup = new QGroupBox(isRateLimiter ? "Rate Limits" : "Saturation Limits");
        QVBoxLayout* limiterLayout = new QVBoxLayout(limiterGroup);

        // 상한값 설정
        QHBoxLayout* upperLayout = new QHBoxLayout();
        QString upperLabel = isRateLimiter ? "Rising Limit:" : "Upper Limit:";
        QLabel* upLabel = new QLabel(upperLabel);
        m_upperEdit = new QDoubleSpinBox();
        m_upperEdit->setRange(-1000000, 1000000);
        m_upperEdit->setDecimals(4);

        QString upperProp = isRateLimiter ? "risingLimit" : "upperLimit";
        m_upperEdit->setValue(block->getProperties()[upperProp].toDouble());

        upperLayout->addWidget(upLabel);
        upperLayout->addWidget(m_upperEdit);
        limiterLayout->addLayout(upperLayout);

        // 하한값 설정
        QHBoxLayout* lowerLayout = new QHBoxLayout();
        QString lowerLabel = isRateLimiter ? "Falling Limit:" : "Lower Limit:";
        QLabel* lowLabel = new QLabel(lowerLabel);
        m_lowerEdit = new QDoubleSpinBox();
        m_lowerEdit->setRange(-1000000, 1000000);
        m_lowerEdit->setDecimals(4);

        QString lowerProp = isRateLimiter ? "fallingLimit" : "lowerLimit";
        m_lowerEdit->setValue(block->getProperties()[lowerProp].toDouble());

        lowerLayout->addWidget(lowLabel);
        lowerLayout->addWidget(m_lowerEdit);
        limiterLayout->addLayout(lowerLayout);

        // 샘플 시간 설정 (Rate Limiter만 해당)
        if (isRateLimiter) {
            QHBoxLayout* sampleTimeLayout = new QHBoxLayout();
            QLabel* stLabel = new QLabel("Sample Time:");
            m_sampleTimeEdit = new QDoubleSpinBox();
            m_sampleTimeEdit->setRange(0.0001, 1000);
            m_sampleTimeEdit->setDecimals(4);
            m_sampleTimeEdit->setValue(block->getProperties()["sampleTime"].toDouble());

            sampleTimeLayout->addWidget(stLabel);
            sampleTimeLayout->addWidget(m_sampleTimeEdit);
            limiterLayout->addLayout(sampleTimeLayout);
        } else {
            m_sampleTimeEdit = nullptr;
        }

        mainLayout->addWidget(limiterGroup);

        // 블록 설명
        QGroupBox* descGroup = new QGroupBox("Description");
        QVBoxLayout* descGroupLayout = new QVBoxLayout(descGroup);

        QLabel* gateDescLabel = new QLabel();
        gateDescLabel->setWordWrap(true);

        if (isRateLimiter) {
            gateDescLabel->setText("Rate Limiter restricts how quickly a signal can change. "
                                   "The Rising Limit caps the positive rate of change, "
                                   "while the Falling Limit caps the negative rate of change.");
        } else {
            gateDescLabel->setText("Saturation limits the signal value between the upper and lower bounds. "
                                   "Values above the upper limit are clamped to the upper limit. "
                                   "Values below the lower limit are clamped to the lower limit.");
        }

        descGroupLayout->addWidget(gateDescLabel);
        mainLayout->addWidget(descGroup);

        // 버튼
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK");
        QPushButton* cancelButton = new QPushButton("Cancel");

        connect(okButton, &QPushButton::clicked, this, &LimiterPropertyDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);
    }

    void accept() override {
        // 블록 이름 업데이트
        m_block->setName(m_nameEdit->text());

        // 속성 업데이트
        QMap<QString, QVariant> props = m_block->getProperties();

        bool isRateLimiter = (m_block->getType() == Block::RATE_LIMITER);

        if (isRateLimiter) {
            props["risingLimit"] = m_upperEdit->value();
            props["fallingLimit"] = m_lowerEdit->value();
            if (m_sampleTimeEdit) {
                props["sampleTime"] = m_sampleTimeEdit->value();
            }
        } else {
            props["upperLimit"] = m_upperEdit->value();
            props["lowerLimit"] = m_lowerEdit->value();
        }

        m_block->setProperties(props);

        QDialog::accept();
    }

private:
    Block* m_block;
    QLineEdit* m_nameEdit;
    QDoubleSpinBox* m_upperEdit;
    QDoubleSpinBox* m_lowerEdit;
    QDoubleSpinBox* m_sampleTimeEdit;
};


class LogicGatePropertyDialog : public QDialog {
public:
    LogicGatePropertyDialog(Block* block, QWidget* parent = nullptr)
        : QDialog(parent), m_block(block) {

        QString typeName;
        switch (block->getType()) {
        case Block::AND: typeName = "AND"; break;
        case Block::OR: typeName = "OR"; break;
        case Block::NOT: typeName = "NOT"; break;
        case Block::XOR: typeName = "XOR"; break;
        default: typeName = "Logic Gate"; break;
        }

        setWindowTitle(typeName + " Properties");
        resize(350, 250);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 이름 필드
        QFormLayout* formLayout = new QFormLayout();
        m_nameEdit = new QLineEdit(block->getName());
        formLayout->addRow("Name:", m_nameEdit);

        mainLayout->addLayout(formLayout);

        // 그룹 박스로 로직 게이트 설정 포함
        QGroupBox* logicGroup = new QGroupBox("Logic Settings");
        QVBoxLayout* logicLayout = new QVBoxLayout(logicGroup);

        // 임계값 설정
        QHBoxLayout* thresholdLayout = new QHBoxLayout();
        QLabel* thresholdLabel = new QLabel("Threshold:");
        m_thresholdEdit = new QDoubleSpinBox();
        m_thresholdEdit->setRange(0, 1);
        m_thresholdEdit->setSingleStep(0.1);
        m_thresholdEdit->setDecimals(2);
        m_thresholdEdit->setValue(block->getProperties()["threshold"].toDouble());
        thresholdLayout->addWidget(thresholdLabel);
        thresholdLayout->addWidget(m_thresholdEdit);
        logicLayout->addLayout(thresholdLayout);

        // 설명 레이블
        QLabel* descLabel = new QLabel("Values >= threshold are considered logical '1'");
        logicLayout->addWidget(descLabel);

        // NOT 블록이 아닌 경우만 입력 포트 수 설정 표시
        if (block->getType() != Block::NOT) {
            QHBoxLayout* inputCountLayout = new QHBoxLayout();
            QLabel* inputCountLabel = new QLabel("Input Ports:");
            m_inputCountSpin = new QSpinBox();
            m_inputCountSpin->setRange(2, 5);
            m_inputCountSpin->setValue(block->getProperties()["inputCount"].toInt());
            inputCountLayout->addWidget(inputCountLabel);
            inputCountLayout->addWidget(m_inputCountSpin);
            logicLayout->addLayout(inputCountLayout);
        } else {
            m_inputCountSpin = nullptr;
        }

        mainLayout->addWidget(logicGroup);

        // 로직 게이트 동작 설명
        QGroupBox* descGroup = new QGroupBox("Description");
        QVBoxLayout* descGroupLayout = new QVBoxLayout(descGroup);

        QLabel* gateDescLabel = new QLabel();
        gateDescLabel->setWordWrap(true);

        switch (block->getType()) {
        case Block::AND:
            gateDescLabel->setText("AND gate outputs 1 only when ALL inputs are >=threshold.");
            break;
        case Block::OR:
            gateDescLabel->setText("OR gate outputs 1 when ANY input is >=threshold.");
            break;
        case Block::NOT:
            gateDescLabel->setText("NOT gate inverts the input: outputs 1 when input is <threshold, 0 otherwise.");
            break;
        case Block::XOR:
            gateDescLabel->setText("XOR gate outputs 1 when an ODD number of inputs are >=threshold.");
            break;
        default:
            break;
        }

        descGroupLayout->addWidget(gateDescLabel);
        mainLayout->addWidget(descGroup);

        // 버튼
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK");
        QPushButton* cancelButton = new QPushButton("Cancel");

        connect(okButton, &QPushButton::clicked, this, &LogicGatePropertyDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);
    }

    void accept() override {
        // 블록 이름 업데이트
        m_block->setName(m_nameEdit->text());

        // 속성 업데이트
        QMap<QString, QVariant> props = m_block->getProperties();

        props["threshold"] = m_thresholdEdit->value();

        // NOT 블록이 아닌 경우만 입력 포트 수 업데이트
        if (m_inputCountSpin && m_block->getType() != Block::NOT) {
            props["inputCount"] = m_inputCountSpin->value();
        }

        m_block->setProperties(props);

        // 포트 재설정
        m_block->setupPorts();

        QDialog::accept();
    }

private:
    Block* m_block;
    QLineEdit* m_nameEdit;
    QDoubleSpinBox* m_thresholdEdit;
    QSpinBox* m_inputCountSpin;
};



class LookupTablePropertyDialog : public QDialog {
public:
    LookupTablePropertyDialog(Block* block, QWidget* parent = nullptr)
        : QDialog(parent), m_block(block) {

        bool is2D = (block->getType() == Block::LOOKUP_TABLE_2D);

        setWindowTitle(is2D ? "2D Lookup Table Properties" : "1D Lookup Table Properties");
        resize(600, is2D ? 800 : 600);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 이름 필드
        QFormLayout* formLayout = new QFormLayout();
        m_nameEdit = new QLineEdit(block->getName());
        formLayout->addRow("Name:", m_nameEdit);

        mainLayout->addLayout(formLayout);

        // 탭 위젯 생성
        QTabWidget* tabWidget = new QTabWidget();

        // 데이터 탭
        QWidget* dataTab = new QWidget();
        QVBoxLayout* dataLayout = new QVBoxLayout(dataTab);

        // X 데이터 편집기
        m_xDataEditor = new TableDataEditor("X Data (Breakpoints)", false);
        m_xDataEditor->setValue(block->getProperties()["xData"].toString());
        dataLayout->addWidget(m_xDataEditor);

        // Y 데이터 편집기 (2D인 경우에만)
        if (is2D) {
            m_yDataEditor = new TableDataEditor("Y Data (Breakpoints)", false);
            m_yDataEditor->setValue(block->getProperties()["yData"].toString());
            dataLayout->addWidget(m_yDataEditor);

            // Z 데이터 편집기 (매트릭스)
            m_zDataEditor = new TableDataEditor("Z Data (Table Values)", true);
            m_zDataEditor->setValue(block->getProperties()["zData"].toString());
            dataLayout->addWidget(m_zDataEditor);
        } else {
            // Y 데이터 편집기 (1D의 출력값)
            m_yDataEditor = new TableDataEditor("Y Data (Output Values)", false);
            m_yDataEditor->setValue(block->getProperties()["yData"].toString());
            dataLayout->addWidget(m_yDataEditor);
        }

        tabWidget->addTab(dataTab, "Data");

        // 설정 탭
        QWidget* settingsTab = new QWidget();
        QFormLayout* settingsLayout = new QFormLayout(settingsTab);

        // 보간 방법
        m_interpolationCombo = new QComboBox();
        m_interpolationCombo->addItem("Linear", "linear");
        m_interpolationCombo->addItem("Nearest", "nearest");

        QString currentInterp = block->getProperties()["interpolation"].toString();
        int interpIndex = m_interpolationCombo->findData(currentInterp);
        m_interpolationCombo->setCurrentIndex(interpIndex != -1 ? interpIndex : 0);

        settingsLayout->addRow("Interpolation Method:", m_interpolationCombo);

        // 외삽 방법
        m_extrapolationCombo = new QComboBox();
        m_extrapolationCombo->addItem("Linear", "linear");
        m_extrapolationCombo->addItem("Nearest", "nearest");
        m_extrapolationCombo->addItem("Constant", "constant");

        QString currentExtrap = block->getProperties()["extrapolation"].toString();
        int extrapIndex = m_extrapolationCombo->findData(currentExtrap);
        m_extrapolationCombo->setCurrentIndex(extrapIndex != -1 ? extrapIndex : 0);

        settingsLayout->addRow("Extrapolation Method:", m_extrapolationCombo);

        tabWidget->addTab(settingsTab, "Settings");

        mainLayout->addWidget(tabWidget);

        // 버튼
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK");
        QPushButton* cancelButton = new QPushButton("Cancel");

        connect(okButton, &QPushButton::clicked, this, &LookupTablePropertyDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);
    }

    void accept() override {
        // 블록 이름 업데이트
        m_block->setName(m_nameEdit->text());

        // 속성 업데이트
        QMap<QString, QVariant> props = m_block->getProperties();

        // X, Y 데이터 업데이트
        props["xData"] = m_xDataEditor->getValue();
        props["yData"] = m_yDataEditor->getValue();

        // 2D 테이블인 경우 Z 데이터 업데이트
        if (m_block->getType() == Block::LOOKUP_TABLE_2D && m_zDataEditor) {
            props["zData"] = m_zDataEditor->getValue();
        }

        // 보간, 외삽 방법 업데이트
        props["interpolation"] = m_interpolationCombo->currentData().toString();
        props["extrapolation"] = m_extrapolationCombo->currentData().toString();

        m_block->setProperties(props);

        QDialog::accept();
    }

private:
    Block* m_block;
    QLineEdit* m_nameEdit;
    TableDataEditor* m_xDataEditor;
    TableDataEditor* m_yDataEditor;
    TableDataEditor* m_zDataEditor;
    QComboBox* m_interpolationCombo;
    QComboBox* m_extrapolationCombo;
};




class ScopeDialog : public QDialog {
    //Q_OBJECT
public:
    ScopeDialog(ScopeBlock* scope, QWidget* parent = nullptr)
        : QDialog(parent), m_scope(scope) {
        setWindowTitle("Scope: " + scope->getName());
        resize(600, 400);

        QVBoxLayout* layout = new QVBoxLayout(this);

        // Create a custom widget for plotting
        m_plotWidget = new QWidget();
        m_plotWidget->setMinimumSize(580, 350);
        layout->addWidget(m_plotWidget);

        // Update timer
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &ScopeDialog::updatePlot);
        m_timer->start(50); // 20Hz update rate
    }

    void paintEvent(QPaintEvent* event) override {
        QDialog::paintEvent(event);

        // Draw the plot
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Get scope data
        const std::vector<double>& values = m_scope->getValues();
        if (values.empty()) return;

        // Calculate scale
        double maxVal = *std::max_element(values.begin(), values.end());
        double minVal = *std::min_element(values.begin(), values.end());
        double range = std::max(1.0, maxVal - minVal) * 1.1; // Add 10% margin

        // Draw axes
        QRect plotRect = m_plotWidget->rect();
        painter.setPen(Qt::black);
        painter.drawRect(plotRect);

        // Draw horizontal grid lines
        painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
        for (int i = 1; i < 10; ++i) {
            int y = plotRect.top() + (i * plotRect.height() / 10);
            painter.drawLine(plotRect.left(), y, plotRect.right(), y);
        }

        // Draw vertical grid lines
        for (int i = 1; i < 10; ++i) {
            int x = plotRect.left() + (i * plotRect.width() / 10);
            painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
        }

        // Draw the waveform
        painter.setPen(QPen(Qt::blue, 2));
        QPainterPath path;

        bool first = true;
        int totalPoints = std::min(500, static_cast<int>(values.size()));
        int step = std::max(1, static_cast<int>(values.size()) / totalPoints);

        for (int i = 0; i < totalPoints; ++i) {
            int idx = values.size() - 1 - (totalPoints - 1 - i) * step;
            if (idx < 0) continue;

            double normalizedValue = 1.0 - (values[idx] - minVal) / range;
            QPointF point(
                plotRect.left() + (i * plotRect.width() / totalPoints),
                plotRect.top() + normalizedValue * plotRect.height()
                );

            if (first) {
                path.moveTo(point);
                first = false;
            } else {
                path.lineTo(point);
            }
        }

        painter.drawPath(path);
    }

private slots:
    void updatePlot() {
        // Force redraw
        update();
    }

private:
    ScopeBlock* m_scope;
    QWidget* m_plotWidget;
    QTimer* m_timer;
};



class SignalGeneratorPropertyDialog : public QDialog {
public:
    SignalGeneratorPropertyDialog(Block* block, QWidget* parent = nullptr)
        : QDialog(parent), m_block(block) {

        // 블록 타입에 따른 제목 설정
        QString typeName;
        QMap<QString, QVariant> properties;

        switch (block->getType()) {
        case Block::CLOCK:
            typeName = "Clock";
            break;
        case Block::RAMP:
            typeName = "Ramp";
            break;
        case Block::STEP:
            typeName = "Step";
            break;
        case Block::SINE_WAVE:
            typeName = "Sine Wave";
            break;
        default:
            typeName = "Signal Generator";
            break;
        }

        setWindowTitle(typeName + " Properties");
        resize(400, 350);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 이름 필드
        QFormLayout* formLayout = new QFormLayout();
        m_nameEdit = new QLineEdit(block->getName());
        formLayout->addRow("Name:", m_nameEdit);

        mainLayout->addLayout(formLayout);

        // 블록 타입에 따른 속성 설정 UI 생성
        QGroupBox* propertiesGroup = new QGroupBox("Signal Properties");
        QFormLayout* propertiesLayout = new QFormLayout(propertiesGroup);

        // 속성 목록 생성
        m_propertyEdits.clear();

        switch (block->getType()) {
        case Block::CLOCK:
            addDoubleProperty(propertiesLayout, "period", "Period (seconds):", 0.001, 1000.0, 1.0);
            addDoubleProperty(propertiesLayout, "offset", "Time Offset (seconds):", 0.0, 1000.0, 0.0);
            break;

        case Block::RAMP:
            addDoubleProperty(propertiesLayout, "slope", "Slope:", -1000.0, 1000.0, 1.0);
            addDoubleProperty(propertiesLayout, "startTime", "Start Time (seconds):", 0.0, 1000.0, 0.0);
            addDoubleProperty(propertiesLayout, "initialOutput", "Initial Output:", -1000.0, 1000.0, 0.0);
            break;

        case Block::STEP:
            addDoubleProperty(propertiesLayout, "stepTime", "Step Time (seconds):", 0.0, 1000.0, 1.0);
            addDoubleProperty(propertiesLayout, "initialValue", "Initial Value:", -1000.0, 1000.0, 0.0);
            addDoubleProperty(propertiesLayout, "finalValue", "Final Value:", -1000.0, 1000.0, 1.0);
            break;

        case Block::SINE_WAVE:
            addDoubleProperty(propertiesLayout, "amplitude", "Amplitude:", 0.0, 1000.0, 1.0);
            addDoubleProperty(propertiesLayout, "frequency", "Frequency (Hz):", 0.001, 1000.0, 1.0);
            addDoubleProperty(propertiesLayout, "phase", "Phase (radians):", -6.28, 6.28, 0.0);
            addDoubleProperty(propertiesLayout, "bias", "Bias (offset):", -1000.0, 1000.0, 0.0);
            break;

        default:
            break;
        }

        mainLayout->addWidget(propertiesGroup);

        // 신호 미리보기
        QGroupBox* previewGroup = new QGroupBox("Signal Preview");
        QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

        // 커스텀 미리보기 위젯 생성
        m_previewWidget = new SignalPreviewWidget(m_block, &m_propertyEdits);
        previewLayout->addWidget(m_previewWidget);

        // 설명 라벨
        QLabel* previewLabel = new QLabel("Signal preview over a 5-second period");
        previewLabel->setAlignment(Qt::AlignCenter);
        previewLayout->addWidget(previewLabel);

        mainLayout->addWidget(previewGroup);

        // 속성 값 변경 시 미리보기 업데이트
        for (auto it = m_propertyEdits.begin(); it != m_propertyEdits.end(); ++it) {
            QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(it.value());
            if (spinBox) {
                connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                        m_previewWidget, QOverload<>::of(&QWidget::update));
            }
        }

        // 버튼
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK");
        QPushButton* cancelButton = new QPushButton("Cancel");

        connect(okButton, &QPushButton::clicked, this, &SignalGeneratorPropertyDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);
    }

    void accept() override {
        // 블록 이름 업데이트
        m_block->setName(m_nameEdit->text());

        // 속성 업데이트
        QMap<QString, QVariant> props = m_block->getProperties();

        for (auto it = m_propertyEdits.begin(); it != m_propertyEdits.end(); ++it) {
            QString propName = it.key();
            QWidget* editor = it.value();

            QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(editor);
            if (spinBox) {
                props[propName] = spinBox->value();
            }
        }

        m_block->setProperties(props);

        QDialog::accept();
    }

private:
    // 속성 에디터 추가 헬퍼 함수
    void addDoubleProperty(QFormLayout* layout, const QString& propName, const QString& label,
                           double min, double max, double defaultValue) {
        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        spinBox->setRange(min, max);
        spinBox->setDecimals(3);
        spinBox->setSingleStep(0.1);

        // 기존 속성값 가져오기
        QVariant value = m_block->getProperties()[propName];
        if (value.isValid()) {
            spinBox->setValue(value.toDouble());
        } else {
            spinBox->setValue(defaultValue);
        }

        layout->addRow(label, spinBox);

        m_propertyEdits[propName] = spinBox;
    }

    Block* m_block;
    QLineEdit* m_nameEdit;
    QMap<QString, QWidget*> m_propertyEdits;
    SignalPreviewWidget* m_previewWidget;  // 커스텀 위젯 사용
};



class SwitchPropertyDialog : public QDialog {
public:
    SwitchPropertyDialog(Block* block, QWidget* parent = nullptr)
        : QDialog(parent), m_block(block) {

        setWindowTitle("Switch Properties");
        resize(400, 300);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 이름 필드
        QFormLayout* formLayout = new QFormLayout();
        m_nameEdit = new QLineEdit(block->getName());
        formLayout->addRow("Name:", m_nameEdit);

        mainLayout->addLayout(formLayout);

        // 그룹 박스로 스위치 설정 포함
        QGroupBox* switchGroup = new QGroupBox("Switch Condition");
        QVBoxLayout* switchLayout = new QVBoxLayout(switchGroup);

        // 임계값 설정
        QHBoxLayout* thresholdLayout = new QHBoxLayout();
        QLabel* thresholdLabel = new QLabel("Threshold:");
        m_thresholdEdit = new QDoubleSpinBox();
        m_thresholdEdit->setRange(-1000000, 1000000);
        m_thresholdEdit->setDecimals(4);
        m_thresholdEdit->setValue(block->getProperties()["threshold"].toDouble());
        thresholdLayout->addWidget(thresholdLabel);
        thresholdLayout->addWidget(m_thresholdEdit);
        switchLayout->addLayout(thresholdLayout);

        // 비교 연산자 설정
        QHBoxLayout* operatorLayout = new QHBoxLayout();
        QLabel* operatorLabel = new QLabel("Operator:");
        m_operatorCombo = new QComboBox();
        m_operatorCombo->addItem(">=", ">=");
        m_operatorCombo->addItem(">", ">");
        m_operatorCombo->addItem("==", "==");
        m_operatorCombo->addItem("!=", "!=");
        m_operatorCombo->addItem("<", "<");
        m_operatorCombo->addItem("<=", "<=");

        QString currentOp = block->getProperties()["operator"].toString();
        int opIndex = m_operatorCombo->findData(currentOp);
        m_operatorCombo->setCurrentIndex(opIndex != -1 ? opIndex : 0);

        operatorLayout->addWidget(operatorLabel);
        operatorLayout->addWidget(m_operatorCombo);
        switchLayout->addLayout(operatorLayout);

        // 스위치 동작 설명
        QLabel* descLabel = new QLabel("The condition is evaluated as: condition [operator] threshold");
        descLabel->setWordWrap(true);
        switchLayout->addWidget(descLabel);

        // 조건 충족 시 동작 선택
        m_passFirstInput = new QRadioButton("If condition is true, pass first input (u1)");
        m_passSecondInput = new QRadioButton("If condition is true, pass second input (u2)");

        bool passFirst = block->getProperties()["passFirstInput"].toBool();
        m_passFirstInput->setChecked(passFirst);
        m_passSecondInput->setChecked(!passFirst);

        switchLayout->addWidget(m_passFirstInput);
        switchLayout->addWidget(m_passSecondInput);

        mainLayout->addWidget(switchGroup);

        // 현재 상태 시각적으로 표시
        QGroupBox* previewGroup = new QGroupBox("Preview");
        QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

        m_previewLabel = new QLabel();
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->setMinimumHeight(50);
        updatePreview();

        previewLayout->addWidget(m_previewLabel);
        mainLayout->addWidget(previewGroup);

        // 상태 업데이트를 위한 연결
        connect(m_operatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SwitchPropertyDialog::updatePreview);
        connect(m_thresholdEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &SwitchPropertyDialog::updatePreview);
        connect(m_passFirstInput, &QRadioButton::toggled,
                this, &SwitchPropertyDialog::updatePreview);

        // 버튼
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK");
        QPushButton* cancelButton = new QPushButton("Cancel");

        connect(okButton, &QPushButton::clicked, this, &SwitchPropertyDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);
    }

    void updatePreview() {
        QString op = m_operatorCombo->currentData().toString();
        double threshold = m_thresholdEdit->value();
        bool passFirst = m_passFirstInput->isChecked();

        QString passStr = passFirst ? "u1" : "u2";
        QString notPassStr = passFirst ? "u2" : "u1";

        QString text = QString("<b>If condition %1 %2 then output = %3 else output = %4</b>")
                           .arg(op).arg(threshold).arg(passStr).arg(notPassStr);

        m_previewLabel->setText(text);
    }

    void accept() override {
        // 블록 이름 업데이트
        m_block->setName(m_nameEdit->text());

        // 속성 업데이트
        QMap<QString, QVariant> props = m_block->getProperties();

        props["threshold"] = m_thresholdEdit->value();
        props["operator"] = m_operatorCombo->currentData().toString();
        props["passFirstInput"] = m_passFirstInput->isChecked();

        m_block->setProperties(props);

        QDialog::accept();
    }

private:
    Block* m_block;
    QLineEdit* m_nameEdit;
    QDoubleSpinBox* m_thresholdEdit;
    QComboBox* m_operatorCombo;
    QRadioButton* m_passFirstInput;
    QRadioButton* m_passSecondInput;
    QLabel* m_previewLabel;
};


#endif // BLOCKDIALOG_H
