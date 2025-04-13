#ifndef TABLEDATAEDITOR_H
#define TABLEDATAEDITOR_H


#include <QWidget>
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>


class TableDataEditor : public QWidget {
public:
    TableDataEditor(const QString& title, bool isMatrix = false, QWidget* parent = nullptr)
        : QWidget(parent), m_isMatrix(isMatrix) {
        QVBoxLayout* layout = new QVBoxLayout(this);

        // 타이틀 라벨
        QLabel* titleLabel = new QLabel(title);
        titleLabel->setFont(QFont("Arial", 10, QFont::Bold));
        layout->addWidget(titleLabel);

        // 설명 라벨
        QString description = isMatrix ?
                                  "Enter values as comma-separated numbers. Matrix data is stored in row-major order." :
                                  "Enter values as comma-separated numbers.";
        QLabel* descLabel = new QLabel(description);
        descLabel->setWordWrap(true);
        layout->addWidget(descLabel);

        // 텍스트 에디터
        m_textEdit = new QTextEdit();
        m_textEdit->setAcceptRichText(false);
        m_textEdit->setTabChangesFocus(true);
        layout->addWidget(m_textEdit);

        // 매트릭스 뷰 (2D 테이블인 경우)
        if (m_isMatrix) {
            QHBoxLayout* matrixLayout = new QHBoxLayout();

            // 행 수 설정
            QLabel* rowsLabel = new QLabel("Rows:");
            m_rowsSpinBox = new QSpinBox();
            m_rowsSpinBox->setRange(1, 100);
            m_rowsSpinBox->setValue(1);

            // 열 수 설정
            QLabel* colsLabel = new QLabel("Columns:");
            m_colsSpinBox = new QSpinBox();
            m_colsSpinBox->setRange(1, 100);
            m_colsSpinBox->setValue(1);

            // 미리보기 버튼
            QPushButton* previewButton = new QPushButton("Preview as Table");

            matrixLayout->addWidget(rowsLabel);
            matrixLayout->addWidget(m_rowsSpinBox);
            matrixLayout->addWidget(colsLabel);
            matrixLayout->addWidget(m_colsSpinBox);
            matrixLayout->addWidget(previewButton);

            layout->addLayout(matrixLayout);

            // 미리보기 테이블
            m_tableWidget = new QTableWidget();
            m_tableWidget->setMinimumHeight(150);
            layout->addWidget(m_tableWidget);
            m_tableWidget->hide();

            // 버튼 연결
            connect(previewButton, &QPushButton::clicked, this, &TableDataEditor::updateTablePreview);

            // 테이블 편집 시 텍스트 업데이트
            connect(m_tableWidget, &QTableWidget::cellChanged, this, [this](int row, int column) {
                updateTextFromTable();
            });
        }

        setLayout(layout);
    }

    void setValue(const QString& value) {
        m_textEdit->setText(value);

        if (m_isMatrix) {
            // 행과 열 수 추정
            QStringList values = value.split(',');
            int totalCount = values.size();

            // 행과 열 개수 추정
            int guessRows = 1;
            int guessCols = totalCount;

            // 완전 제곱수인지 확인 (행/열이 같은 정사각형)
            double sqRoot = std::sqrt(totalCount);
            if (std::abs(sqRoot - std::round(sqRoot)) < 0.001) {
                guessRows = static_cast<int>(std::round(sqRoot));
                guessCols = guessRows;
            }
            // 또는 공통 약수 중 적당한 것을 찾음
            else {
                for (int i = 2; i <= std::sqrt(totalCount); i++) {
                    if (totalCount % i == 0) {
                        guessRows = i;
                        guessCols = totalCount / i;
                    }
                }
            }

            m_rowsSpinBox->setValue(guessRows);
            m_colsSpinBox->setValue(guessCols);
        }
    }

    QString getValue() const {
        return m_textEdit->toPlainText();
    }

private slots:
    void updateTablePreview() {
        if (!m_isMatrix) return;

        m_tableWidget->show();

        int rows = m_rowsSpinBox->value();
        int cols = m_colsSpinBox->value();

        // 테이블 준비
        m_tableWidget->setRowCount(rows);
        m_tableWidget->setColumnCount(cols);

        // 데이터 파싱 및 테이블에 채우기
        QStringList values = m_textEdit->toPlainText().split(',', Qt::SkipEmptyParts);
        int index = 0;

        for (int i = 0; i < rows && index < values.size(); i++) {
            for (int j = 0; j < cols && index < values.size(); j++) {
                QTableWidgetItem* item = new QTableWidgetItem(values[index].trimmed());
                m_tableWidget->setItem(i, j, item);
                index++;
            }
        }

        // 남은 셀 비우기
        while (index < rows * cols) {
            int i = index / cols;
            int j = index % cols;
            QTableWidgetItem* item = new QTableWidgetItem("0");
            m_tableWidget->setItem(i, j, item);
            index++;
        }
    }

    void updateTextFromTable() {
        if (!m_isMatrix || !m_tableWidget->isVisible()) return;

        int rows = m_tableWidget->rowCount();
        int cols = m_tableWidget->columnCount();

        QStringList values;

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                QTableWidgetItem* item = m_tableWidget->item(i, j);
                if (item) {
                    values.append(item->text());
                } else {
                    values.append("0");
                }
            }
        }

        m_textEdit->setText(values.join(", "));
    }

private:
    QTextEdit* m_textEdit;
    bool m_isMatrix;
    QSpinBox* m_rowsSpinBox;
    QSpinBox* m_colsSpinBox;
    QTableWidget* m_tableWidget;
};

#endif // TABLEDATAEDITOR_H
