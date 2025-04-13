#ifndef SIGNALPREVIEWWIDGET_H
#define SIGNALPREVIEWWIDGET_H


#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QMap>
#include <QVariant>
#include <QDoubleSpinBox>

#include <cmath>


#include "block.h"

class SignalPreviewWidget : public QWidget {
    Q_OBJECT

public:
    SignalPreviewWidget(Block* block, QMap<QString, QWidget*>* propertyEdits, QWidget* parent = nullptr)
        : QWidget(parent), m_block(block), m_propertyEdits(propertyEdits) {
        setMinimumHeight(120);
        setAutoFillBackground(true);

        // 흰색 배경 설정
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 그리드 그리기
        drawPreviewGrid(&painter);

        // 신호 그리기
        drawPreviewSignal(&painter);
    }

private:
    // 미리보기 그리드 그리기
    void drawPreviewGrid(QPainter* painter) {
        int width = this->width();
        int height = this->height();
        double centerY = height / 2.0;

        // 배경
        painter->fillRect(QRect(0, 0, width, height), Qt::white);

        // 테두리
        painter->setPen(QPen(Qt::lightGray, 1));
        painter->drawRect(0, 0, width-1, height-1);

        // 수평 중앙선 (0 값선)
        painter->setPen(QPen(Qt::gray, 1, Qt::DashLine));
        painter->drawLine(0, centerY, width, centerY);

        // 세로 격자선 (0.5초 간격)
        painter->setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
        for (int i = 1; i <= 10; i++) {
            int x = width * i / 10;
            painter->drawLine(x, 0, x, height);
        }

        // 시간 축 표시
        painter->setPen(Qt::black);
        painter->setFont(QFont("Arial", 7));
        for (int i = 0; i <= 5; i++) {
            int x = width * i / 5;
            painter->drawText(QRect(x-10, height-15, 20, 15), Qt::AlignCenter, QString::number(i));
        }
    }

    // 신호 미리보기 그리기
    void drawPreviewSignal(QPainter* painter) {
        int width = this->width();
        int height = this->height();
        double centerY = height / 2.0;
        double maxY = height / 2.5;  // 최대 진폭 (화면 범위 내)

        // 미리보기용 속성 값 가져오기
        QMap<QString, QVariant> props;
        for (auto it = m_propertyEdits->begin(); it != m_propertyEdits->end(); ++it) {
            QString propName = it.key();
            QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(it.value());
            if (spinBox) {
                props[propName] = spinBox->value();
            }
        }

        // 신호 경로 그리기
        QPainterPath path;
        bool firstPoint = true;

        // 블록 타입에 따라 다른 신호 그리기
        switch (m_block->getType()) {
        case Block::CLOCK: {
            double period = props["period"].toDouble();
            double offset = props["offset"].toDouble();

            // 5초 동안의 신호 그리기
            for (double t = 0; t <= 5.0; t += 0.01) {
                double value = std::fmod(t - offset, period) / period;  // 0~1 정규화된 값
                double y = centerY - value * maxY;
                double x = width * t / 5.0;

                if (firstPoint) {
                    path.moveTo(x, y);
                    firstPoint = false;
                } else {
                    path.lineTo(x, y);
                }
            }
            break;
        }

        case Block::RAMP: {
            double slope = props["slope"].toDouble();
            double startTime = props["startTime"].toDouble();
            double initialOutput = props["initialOutput"].toDouble();

            // 값 범위 제한 (그래프 표시 범위 내로)
            double maxValue = 2.0;
            double scaleFactor = std::min(1.0, maxValue / (std::abs(slope) * 5.0 + std::abs(initialOutput)));

            // 5초 동안의 신호 그리기
            for (double t = 0; t <= 5.0; t += 0.01) {
                double value = (t < startTime) ? initialOutput : initialOutput + slope * (t - startTime);
                value *= scaleFactor;  // 스케일 조정
                double y = centerY - value * maxY / maxValue;
                double x = width * t / 5.0;

                if (firstPoint) {
                    path.moveTo(x, y);
                    firstPoint = false;
                } else {
                    path.lineTo(x, y);
                }
            }
            break;
        }

        case Block::STEP: {
            double stepTime = props["stepTime"].toDouble();
            double initialValue = props["initialValue"].toDouble();
            double finalValue = props["finalValue"].toDouble();

            // 값 범위 제한 (그래프 표시 범위 내로)
            double maxValue = 2.0;
            double scaleFactor = std::min(1.0, maxValue / std::max(std::abs(initialValue), std::abs(finalValue)));

            // 5초 동안의 신호 그리기
            for (double t = 0; t <= 5.0; t += 0.01) {
                double value = (t < stepTime) ? initialValue : finalValue;
                value *= scaleFactor;  // 스케일 조정
                double y = centerY - value * maxY / maxValue;
                double x = width * t / 5.0;

                if (firstPoint) {
                    path.moveTo(x, y);
                    firstPoint = false;
                } else {
                    path.lineTo(x, y);
                }
            }
            break;
        }

        case Block::SINE_WAVE: {
            double amplitude = props["amplitude"].toDouble();
            double frequency = props["frequency"].toDouble();
            double phase = props["phase"].toDouble();
            double bias = props["bias"].toDouble();

            // 값 범위 제한 (그래프 표시 범위 내로)
            double maxValue = 2.0;
            double scaleFactor = std::min(1.0, maxValue / (amplitude + std::abs(bias)));

            // 5초 동안의 신호 그리기
            for (double t = 0; t <= 5.0; t += 0.01) {
                double value = amplitude * std::sin(2 * M_PI * frequency * t + phase) + bias;
                value *= scaleFactor;  // 스케일 조정
                double y = centerY - value * maxY / maxValue;
                double x = width * t / 5.0;

                if (firstPoint) {
                    path.moveTo(x, y);
                    firstPoint = false;
                } else {
                    path.lineTo(x, y);
                }
            }
            break;
        }

        default:
            break;
        }

        // 신호 그리기
        painter->setPen(QPen(Qt::blue, 2));
        painter->drawPath(path);
    }

    Block* m_block;
    QMap<QString, QWidget*>* m_propertyEdits;
};


#endif // SIGNALPREVIEWWIDGET_H
