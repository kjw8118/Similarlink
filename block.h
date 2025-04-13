#ifndef BLOCK_H
#define BLOCK_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>


class Block : public QGraphicsItem {
    //Q_OBJECT
public:
    enum BlockType {
        SOURCE,
        GAIN,
        SUM,
        PRODUCT,
        SCOPE,
        INTEGRATOR,
        DERIVATIVE,
        LOOKUP_TABLE_1D,  // 추가: 1D Lookup Table
        LOOKUP_TABLE_2D,  // 추가: 2D Lookup Table
        MIN,          // 추가: 최소값 선택
        MAX,          // 추가: 최대값 선택
        SATURATION,   // 추가: 포화 제한
        RATE_LIMITER, // 추가: 변화율 제한
        SWITCH,       // 추가: 스위치 (조건부 선택)
        DIVIDE,       // 추가: 나눗셈
        AND,          // 추가: 논리 AND
        OR,           // 추가: 논리 OR
        NOT,          // 추가: 논리 NOT
        XOR,          // 추가: 논리 XOR
        CLOCK,      // 추가: Clock 블록
        RAMP,       // 추가: Ramp 블록
        STEP,       // 추가: Step 블록
        SINE_WAVE,  // 추가: Sine Wave 블록
        IN,     // 추가: 입력 블록
        OUT,    // 추가: 출력 블록
        CUSTOM,
        INVALID
    };

    Block(BlockType type, const QString& name, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    virtual void drawPorts(QPainter* painter);

    virtual void setupPorts();

    QPointF getInputPortPos(int index) const;

    QPointF getOutputPortPos(int index) const ;

    bool containsInputPort(const QPointF& point, int& portIndex) const;

    bool containsOutputPort(const QPointF& point, int& portIndex) const ;


    // 호버 이벤트 처리
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    // 호버 나갈 때 하이라이트 해제
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    virtual double compute(const std::vector<double>& inputs) const;

    BlockType getType() const;
    QString getName() const;
    void setName(const QString& name);

    int getInputPortCount() const;
    int getOutputPortCount() const;

    // Properties specific to the block
    virtual void setProperties(const QMap<QString, QVariant>& properties);

    virtual QMap<QString, QVariant> getProperties() const;

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    QString getTypeDisplayName() const {
        // 블록 타입에 따른 표시 이름 반환
        switch (m_type) {
        case SOURCE: return "Source";
        case GAIN: return "Gain";
        case SUM: return "Sum";
        case PRODUCT: return "Product";
        case INTEGRATOR: return "Integrator";
        case DERIVATIVE: return "Derivative";
        case SCOPE: return "Scope";
        case IN: return "In";
        case OUT: return "Out";
        case LOOKUP_TABLE_1D: return "Lookup Table 1D";
        case LOOKUP_TABLE_2D: return "Lookup Table 2D";
        case MIN: return "Min";
        case MAX: return "Max";
        case SATURATION: return "Saturation";
        case RATE_LIMITER: return "Rate Limiter";
        case SWITCH: return "Switch";
        case DIVIDE: return "Divide";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case XOR: return "XOR";
        case CLOCK: return "Clock";
        case RAMP: return "Ramp";
        case STEP: return "Step";
        case SINE_WAVE: return "Sine Wave";
        case CUSTOM: return "Custom";
        default: return "Unknown";
        }
    }

protected:
    BlockType m_type;
    QString m_name;
    qreal m_width;
    qreal m_height;
    std::vector<QPointF> m_inputPorts;
    std::vector<QPointF> m_outputPorts;
    QMap<QString, QVariant> m_properties;

    // 하이라이트된 포트 인덱스
    int m_highlightedInputPort;
    int m_highlightedOutputPort;
};






class SourceBlock : public Block {
public:
    SourceBlock(const QString& name = "Source", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    double compute(const std::vector<double>& inputs) const override;
};

class GainBlock : public Block {
public:
    GainBlock(const QString& name = "Gain", QGraphicsItem* parent = nullptr);

    double compute(const std::vector<double>& inputs) const override;
};

class SumBlock : public Block {
public:
    SumBlock(const QString& name = "Sum", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;
};

class ProductBlock : public Block {
public:
    ProductBlock(const QString& name = "Product", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override;
};

class IntegratorBlock : public Block {
public:
    IntegratorBlock(const QString& name = "Integrator", QGraphicsItem* parent = nullptr);

    double compute(const std::vector<double>& inputs) const override ;

    void resetState();

private:
    mutable double m_accumulator;
};

class DerivativeBlock : public Block {
public:
    DerivativeBlock(const QString& name = "Derivative", QGraphicsItem* parent = nullptr);

    double compute(const std::vector<double>& inputs) const override;

    void resetState();

private:
    mutable double m_lastInput;
};

class ScopeBlock : public Block {
public:
    ScopeBlock(const QString& name = "Scope", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;

    const std::vector<double>& getValues() const ;

private:
    mutable std::vector<double> m_values;
};

// InBlock 클래스 구현
class InBlock : public Block {
public:
    InBlock(const QString& name = "In", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override;
};

// OutBlock 클래스 구현
class OutBlock : public Block {
public:
    OutBlock(const QString& name = "Out", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override ;
};


//==================================================================
// 1D Lookup Table 블록 구현
//==================================================================
class LookupTable1DBlock : public Block {
public:
    LookupTable1DBlock(const QString& name = "Lookup Table 1D", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// 2D Lookup Table 블록 구현
//==================================================================
class LookupTable2DBlock : public Block {
public:
    LookupTable2DBlock(const QString& name = "Lookup Table 2D", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override ;
};

//==================================================================
// Min 블록 구현
//==================================================================
class MinBlock : public Block {
public:
    MinBlock(const QString& name = "Min", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// Max 블록 구현
//==================================================================
class MaxBlock : public Block {
public:
    MaxBlock(const QString& name = "Max", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override ;
};

//==================================================================
// Saturation 블록 구현
//==================================================================
class SaturationBlock : public Block {
public:
    SaturationBlock(const QString& name = "Saturation", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// Rate Limiter 블록 구현
//==================================================================
class RateLimiterBlock : public Block {
public:
    RateLimiterBlock(const QString& name = "Rate Limiter", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override;

    void resetState();

private:
    mutable double m_prevOutput;  // 이전 출력값
    mutable double m_prevTime;    // 이전 시간
    mutable bool m_initialized;   // 초기화 여부
};

//==================================================================
// Switch 블록 구현
//==================================================================
class SwitchBlock : public Block {
public:
    SwitchBlock(const QString& name = "Switch", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override ;
};

//==================================================================
// Divide 블록 구현
//==================================================================
class DivideBlock : public Block {
public:
    DivideBlock(const QString& name = "Divide", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// 논리 AND 블록 구현
//==================================================================
class AndBlock : public Block {
public:
    AndBlock(const QString& name = "AND", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override ;
};

//==================================================================
// 논리 OR 블록 구현
//==================================================================
class OrBlock : public Block {
public:
    OrBlock(const QString& name = "OR", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// 논리 NOT 블록 구현
//==================================================================
class NotBlock : public Block {
public:
    NotBlock(const QString& name = "NOT", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// 논리 XOR 블록 구현
//==================================================================
class XorBlock : public Block {
public:
    XorBlock(const QString& name = "XOR", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override ;
};

//==================================================================
// Clock 블록 구현
//==================================================================
class ClockBlock : public Block {
public:
    ClockBlock(const QString& name = "Clock", QGraphicsItem* parent = nullptr);

    void setupPorts() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    double compute(const std::vector<double>& inputs) const override ;
};

//==================================================================
// Ramp 블록 구현
//==================================================================
class RampBlock : public Block {
public:
    RampBlock(const QString& name = "Ramp", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// Step 블록 구현
//==================================================================
class StepBlock : public Block {
public:
    StepBlock(const QString& name = "Step", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override;
};

//==================================================================
// Sine Wave 블록 구현
//==================================================================
class SineWaveBlock : public Block {
public:
    SineWaveBlock(const QString& name = "Sine Wave", QGraphicsItem* parent = nullptr);

    void setupPorts() override ;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override ;

    double compute(const std::vector<double>& inputs) const override;
};






#endif // BLOCK_H
