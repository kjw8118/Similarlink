#include "blockfactory.h"
#include "block.h"

#include <QSet>

#include <iostream>

// 싱글톤 인스턴스 초기화
BlockFactory* BlockFactory::s_instance = nullptr;

BlockInfo::BlockInfo()
    : m_type(Block::INVALID),
    m_name("Unknown"),
    m_category("Unknown"),
    m_description("Unknown"),
    m_creator([](const QString& name, QGraphicsItem* parent) -> Block* {return new SourceBlock(name, parent);}),
    m_icon(QIcon())
{

}

BlockInfo::BlockInfo(Block::BlockType type,
          const QString& name,
          const QString& category,
          const QString& description,
          const BlockCreatorFunc& creator,
          const QIcon& icon)
    : m_type(type),
    m_name(name),
    m_category(category),
    m_description(description),
    m_creator(creator),
    m_icon(icon) {
}

Block::BlockType BlockInfo::type() const { return m_type; }
QString BlockInfo::name() const { return m_name; }
QString BlockInfo::category() const { return m_category; }
QString BlockInfo::description() const { return m_description; }
QIcon BlockInfo::icon() const { return m_icon; }

// 블록 인스턴스 생성
Block* BlockInfo::createInstance(const QString& instanceName, QGraphicsItem* parent) const {
    if (m_creator) {
        return m_creator(instanceName.isEmpty() ? m_name : instanceName, parent);
    }
    return nullptr;
}

// 코드 생성 함수 등록 (각 언어별)
void BlockInfo::registerCodeGenerator(const QString& language, CodeGeneratorFunc func) {

    m_codeGenerators[language] = func;

    std::cout << "func reg " << m_name.toStdString() << ", " << language.toStdString() << (bool)m_codeGenerators.contains(language) << ", " << (bool)m_codeGenerators[language] << std::endl;
}

// 코드 생성
QString BlockInfo::generateCode(const QString& language, const Block* block) const {
    std::cout << language.toStdString() << ", " << block->getType() << ", " << block->getName().toStdString() << ", " << m_codeGenerators.size() << ", " << (bool)m_codeGenerators.contains(language) << ", " << (bool)m_codeGenerators[language] << std::endl;
    if (m_codeGenerators.contains(language) && m_codeGenerators[language]) {
        return m_codeGenerators[language](block, block->getProperties());
    }
    return QString("// Code generation not supported for block type: %1").arg(m_name);
}


// 싱글톤 인스턴스 얻기
BlockFactory* BlockFactory::instance() {
    if (!s_instance) {
        s_instance = new BlockFactory();
    }
    return s_instance;
}

// 생성자
BlockFactory::BlockFactory(QObject* parent)
    : QObject(parent) {
    // 기본 블록 타입 등록
    registerDefaultBlockTypes();
}

// 블록 정보 등록
void BlockFactory::registerBlockType(const BlockInfo& blockInfo) {
    std::cout << "registered " << blockInfo.type() << ", " << blockInfo.name().toStdString() << std::endl;
    m_blockInfoMap[blockInfo.type()] = blockInfo;
    m_nameToTypeMap[blockInfo.name()] = blockInfo.type();
}

// 등록된 모든 블록 타입 목록 가져오기
QList<BlockInfo> BlockFactory::getAllBlockTypes() const {
    return m_blockInfoMap.values();
}

// 카테고리별 블록 타입 목록 가져오기
QList<BlockInfo> BlockFactory::getBlockTypesByCategory(const QString& category) const {
    QList<BlockInfo> result;
    for (const auto& info : m_blockInfoMap) {
        if (info.category() == category) {
            result.append(info);
        }
    }
    return result;
}

// 특정 타입의 블록 정보 가져오기
BlockInfo BlockFactory::getBlockInfo(Block::BlockType type) const {
    return m_blockInfoMap.value(type);
}

BlockInfo BlockFactory::getBlockInfoByName(const QString& name) const {
    if (m_nameToTypeMap.contains(name)) {
        return m_blockInfoMap.value(m_nameToTypeMap[name]);
    }
    return BlockInfo(Block::INVALID, "", "", "", nullptr);
}

// 블록 인스턴스 생성
Block* BlockFactory::createBlock(Block::BlockType type, const QString& name, QGraphicsItem* parent) const {
    std::cout << "create " << type << ", " << name.toStdString() << std::endl;
    if (m_blockInfoMap.contains(type)) {
        std::cout << "contains " << name.toStdString() << std::endl;
        return m_blockInfoMap[type].createInstance(name, parent);
    }
    return nullptr;
}

Block* BlockFactory::createBlockByName(const QString& typeName, const QString& name, QGraphicsItem* parent) const {
    std::cout << "byN " << typeName.toStdString() << ", " << name.toStdString() << std::endl;
    if (m_nameToTypeMap.contains(typeName)) {
        return createBlock(m_nameToTypeMap[typeName], name, parent);
    }
    return nullptr;
}

// 모든 카테고리 목록 얻기
QStringList BlockFactory::getAllCategories() const {
    QSet<QString> categories;
    for (const auto& info : m_blockInfoMap) {
        categories.insert(info.category());
    }
    return QStringList(categories.begin(), categories.end());
}


// 타입에 따른 코드 생성
QString BlockFactory::generateCode(const QString& language, const Block* block) const {
    std::cout << block->getType() <<  ", " << block->getName().toStdString() << std::endl;
    if (block) {
        return m_blockInfoMap[block->getType()].generateCode(language, block);
    }
    return QString();
}

// 모든 블록 타입 정보 등록
void BlockFactory::registerDefaultBlockTypes() {
    // 기본 블록 타입 등록
    // Source 블록
    registerBlockType(BlockInfo(
        Block::SOURCE,
        "Source",
        "Sources",
        "Constant value source block",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new SourceBlock(name, parent);
        }
        ));

    // Gain 블록
    registerBlockType(BlockInfo(
        Block::GAIN,
        "Gain",
        "Math Operations",
        "Multiplies input by a constant value",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new GainBlock(name, parent);
        }
        ));

    // Sum 블록
    registerBlockType(BlockInfo(
        Block::SUM,
        "Sum",
        "Math Operations",
        "Adds multiple inputs together",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new SumBlock(name, parent);
        }
        ));

    // Product 블록
    registerBlockType(BlockInfo(
        Block::PRODUCT,
        "Product",
        "Math Operations",
        "Multiplies multiple inputs together",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new ProductBlock(name, parent);
        }
        ));

    // Integrator 블록
    registerBlockType(BlockInfo(
        Block::INTEGRATOR,
        "Integrator",
        "Continuous",
        "Integrates input signal over time",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new IntegratorBlock(name, parent);
        }
        ));

    // Derivative 블록
    registerBlockType(BlockInfo(
        Block::DERIVATIVE,
        "Derivative",
        "Continuous",
        "Differentiates input signal over time",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new DerivativeBlock(name, parent);
        }
        ));

    // Scope 블록
    registerBlockType(BlockInfo(
        Block::SCOPE,
        "Scope",
        "Sinks",
        "Displays input signal on a graph",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new ScopeBlock(name, parent);
        }
        ));

    // In 블록
    registerBlockType(BlockInfo(
        Block::IN,
        "In",
        "Ports & Subsystems",
        "Input port for external signals",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new InBlock(name, parent);
        }
        ));

    // Out 블록
    registerBlockType(BlockInfo(
        Block::OUT,
        "Out",
        "Ports & Subsystems",
        "Output port for external signals",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new OutBlock(name, parent);
        }
        ));

    // LookupTable1D 블록
    registerBlockType(BlockInfo(
        Block::LOOKUP_TABLE_1D,
        "Lookup Table 1D",
        "Lookup Tables",
        "Performs 1D table lookup",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new LookupTable1DBlock(name, parent);
        }
        ));

    // LookupTable2D 블록
    registerBlockType(BlockInfo(
        Block::LOOKUP_TABLE_2D,
        "Lookup Table 2D",
        "Lookup Tables",
        "Performs 2D table lookup",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new LookupTable2DBlock(name, parent);
        }
        ));

    // Min 블록
    registerBlockType(BlockInfo(
        Block::MIN,
        "Min",
        "Math Operations",
        "Outputs the minimum of two inputs",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new MinBlock(name, parent);
        }
        ));

    // Max 블록
    registerBlockType(BlockInfo(
        Block::MAX,
        "Max",
        "Math Operations",
        "Outputs the maximum of two inputs",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new MaxBlock(name, parent);
        }
        ));

    // Saturation 블록
    registerBlockType(BlockInfo(
        Block::SATURATION,
        "Saturation",
        "Discontinuities",
        "Limits input signal between lower and upper bounds",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new SaturationBlock(name, parent);
        }
        ));

    std::cout << Block::SATURATION << " -> reg saturation" << std::endl;
    // RateLimiter 블록
    registerBlockType(BlockInfo(
        Block::RATE_LIMITER,
        "Rate Limiter",
        "Discontinuities",
        "Limits the rate of change of the signal",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new RateLimiterBlock(name, parent);
        }
        ));

    // Switch 블록
    registerBlockType(BlockInfo(
        Block::SWITCH,
        "Switch",
        "Signal Routing",
        "Switches between two inputs based on a condition",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new SwitchBlock(name, parent);
        }
        ));

    // Divide 블록
    registerBlockType(BlockInfo(
        Block::DIVIDE,
        "Divide",
        "Math Operations",
        "Divides first input by second input",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new DivideBlock(name, parent);
        }
        ));

    // And 블록
    registerBlockType(BlockInfo(
        Block::AND,
        "AND",
        "Logic Operations",
        "Performs logical AND operation",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new AndBlock(name, parent);
        }
        ));

    // Or 블록
    registerBlockType(BlockInfo(
        Block::OR,
        "OR",
        "Logic Operations",
        "Performs logical OR operation",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new OrBlock(name, parent);
        }
        ));

    // Not 블록
    registerBlockType(BlockInfo(
        Block::NOT,
        "NOT",
        "Logic Operations",
        "Performs logical NOT operation",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new NotBlock(name, parent);
        }
        ));

    // Xor 블록
    registerBlockType(BlockInfo(
        Block::XOR,
        "XOR",
        "Logic Operations",
        "Performs logical XOR operation",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new XorBlock(name, parent);
        }
        ));

    // Clock 블록
    registerBlockType(BlockInfo(
        Block::CLOCK,
        "Clock",
        "Sources",
        "Outputs simulation time",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new ClockBlock(name, parent);
        }
        ));

    // Ramp 블록
    registerBlockType(BlockInfo(
        Block::RAMP,
        "Ramp",
        "Sources",
        "Outputs a signal that starts at a specified time and increases at a specified rate",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new RampBlock(name, parent);
        }
        ));

    // Step 블록
    registerBlockType(BlockInfo(
        Block::STEP,
        "Step",
        "Sources",
        "Outputs a step signal at a specified time",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new StepBlock(name, parent);
        }
        ));

    // SineWave 블록
    registerBlockType(BlockInfo(
        Block::SINE_WAVE,
        "Sine Wave",
        "Sources",
        "Outputs a sinusoidal signal",
        [](const QString& name, QGraphicsItem* parent) -> Block* {
            return new SineWaveBlock(name, parent);
        }
        ));

    // 코드 생성 함수 등록 (C++)
    // 여기서는 예시로 몇 가지 블록에 대해서만 구현
    m_blockInfoMap[Block::SOURCE].registerCodeGenerator("cpp",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            double value = props["value"].toDouble();
                                                            return QString("[](const std::vector<double>& inputs) { return %1; }").arg(value);
                                                        }
                                                        );

    m_blockInfoMap[Block::GAIN].registerCodeGenerator("cpp",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double gain = props["gain"].toDouble();
                                                          return QString("[](const std::vector<double>& inputs) { return inputs[0] * %1; }").arg(gain);
                                                      }
                                                      );

    m_blockInfoMap[Block::SUM].registerCodeGenerator("cpp",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "[](const std::vector<double>& inputs) { double sum = 0; for (auto v : inputs) sum += v; return sum; }";
                                                     }
                                                     );

    // Product 블록
    m_blockInfoMap[Block::PRODUCT].registerCodeGenerator("cpp",
                                                         [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                             return "[](const std::vector<double>& inputs) { double prod = 1.0; for (auto v : inputs) prod *= v; return prod; }";
                                                         }
                                                         );

    // Integrator 블록
    m_blockInfoMap[Block::INTEGRATOR].registerCodeGenerator("cpp",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                double initialValue = props["initial"].toDouble();
                                                                return QString("[](const std::vector<double>& inputs) { static double state = %1; if (!inputs.empty()) state += inputs[0] * 0.01; return state; }").arg(initialValue);
                                                            }
                                                            );

    // Derivative 블록
    m_blockInfoMap[Block::DERIVATIVE].registerCodeGenerator("cpp",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                return "[](const std::vector<double>& inputs) { static double prev = 0.0; static bool init = false; if (!inputs.empty()) { if (!init) { init = true; prev = inputs[0]; return 0.0; } double result = (inputs[0] - prev) / 0.01; prev = inputs[0]; return result; } return 0.0; }";
                                                            }
                                                            );

    // Scope 블록
    m_blockInfoMap[Block::SCOPE].registerCodeGenerator("cpp",
                                                       [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                           return "[](const std::vector<double>& inputs) { return inputs.empty() ? 0.0 : inputs[0]; }";
                                                       }
                                                       );

    // In 블록
    m_blockInfoMap[Block::IN].registerCodeGenerator("cpp",
                                                    [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                        QString varName = props["variable"].toString();
                                                        return QString("[this](const std::vector<double>& inputs) { return m_inputs[\"%1\"]; }").arg(varName);
                                                    }
                                                    );

    // Out 블록
    m_blockInfoMap[Block::OUT].registerCodeGenerator("cpp",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "[](const std::vector<double>& inputs) { return inputs.empty() ? 0.0 : inputs[0]; }";
                                                     }
                                                     );

    // Lookup Table 1D 블록
    m_blockInfoMap[Block::LOOKUP_TABLE_1D].registerCodeGenerator("cpp",
                                                                 [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                     QString xDataStr = props["xData"].toString();
                                                                     QString yDataStr = props["yData"].toString();

                                                                     return QString(
                                                                                "[](const std::vector<double>& inputs) {\n"
                                                                                "    if (inputs.empty()) return 0.0;\n"
                                                                                "    double x = inputs[0];\n"
                                                                                "    // xData 및 yData 파싱\n"
                                                                                "    std::vector<double> xData, yData;\n"
                                                                                "    std::stringstream ssX(\"%1\");\n"
                                                                                "    std::stringstream ssY(\"%2\");\n"
                                                                                "    double val;\n"
                                                                                "    while (ssX >> val) {\n"
                                                                                "        xData.push_back(val);\n"
                                                                                "        if (ssX.peek() == ',') ssX.ignore();\n"
                                                                                "    }\n"
                                                                                "    while (ssY >> val) {\n"
                                                                                "        yData.push_back(val);\n"
                                                                                "        if (ssY.peek() == ',') ssY.ignore();\n"
                                                                                "    }\n"
                                                                                "    if (xData.empty() || yData.empty() || xData.size() != yData.size()) return 0.0;\n\n"
                                                                                "    // 범위 외 처리\n"
                                                                                "    if (x <= xData.front()) return yData.front();\n"
                                                                                "    if (x >= xData.back()) return yData.back();\n\n"
                                                                                "    // 보간 처리\n"
                                                                                "    for (size_t i = 0; i < xData.size() - 1; i++) {\n"
                                                                                "        if (x >= xData[i] && x <= xData[i+1]) {\n"
                                                                                "            double t = (x - xData[i]) / (xData[i+1] - xData[i]);\n"
                                                                                "            return yData[i] * (1 - t) + yData[i+1] * t;\n"
                                                                                "        }\n"
                                                                                "    }\n"
                                                                                "    return 0.0;\n"
                                                                                "}"
                                                                                ).arg(xDataStr).arg(yDataStr);
                                                                 }
                                                                 );

    // Lookup Table 2D 블록
    m_blockInfoMap[Block::LOOKUP_TABLE_2D].registerCodeGenerator("cpp",
                                                                 [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                     return "[](const std::vector<double>& inputs) { return inputs.size() < 2 ? 0.0 : 0.0; /* 2D 룩업은 복잡한 구현 필요 */ }";
                                                                 }
                                                                 );

    // Min 블록
    m_blockInfoMap[Block::MIN].registerCodeGenerator("cpp",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "[](const std::vector<double>& inputs) { return inputs.size() < 2 ? 0.0 : std::min(inputs[0], inputs[1]); }";
                                                     }
                                                     );

    // Max 블록
    m_blockInfoMap[Block::MAX].registerCodeGenerator("cpp",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "[](const std::vector<double>& inputs) { return inputs.size() < 2 ? 0.0 : std::max(inputs[0], inputs[1]); }";
                                                     }
                                                     );

    // Saturation 블록
    m_blockInfoMap[Block::SATURATION].registerCodeGenerator("cpp",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                double upperLimit = props["upperLimit"].toDouble();
                                                                double lowerLimit = props["lowerLimit"].toDouble();
                                                                return QString(
                                                                           "[](const std::vector<double>& inputs) {\n"
                                                                           "    if (inputs.empty()) return 0.0;\n"
                                                                           "    double val = inputs[0];\n"
                                                                           "    if (val > %1) return %1;\n"
                                                                           "    if (val < %2) return %2;\n"
                                                                           "    return val;\n"
                                                                           "}"
                                                                           ).arg(upperLimit).arg(lowerLimit);
                                                            }
                                                            );

    // Rate Limiter 블록
    m_blockInfoMap[Block::RATE_LIMITER].registerCodeGenerator("cpp",
                                                              [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                  double risingLimit = props["risingLimit"].toDouble();
                                                                  double fallingLimit = props["fallingLimit"].toDouble();

                                                                  return QString(
                                                                             "[](const std::vector<double>& inputs) {\n"
                                                                             "    static double prev = 0.0;\n"
                                                                             "    static bool init = false;\n"
                                                                             "    if (inputs.empty()) return 0.0;\n"
                                                                             "    double input = inputs[0];\n"
                                                                             "    if (!init) {\n"
                                                                             "        prev = input;\n"
                                                                             "        init = true;\n"
                                                                             "        return input;\n"
                                                                             "    }\n"
                                                                             "    double rate = (input - prev) / 0.01;\n"
                                                                             "    if (rate > %1) rate = %1;\n"
                                                                             "    else if (rate < %2) rate = %2;\n"
                                                                             "    double output = prev + rate * 0.01;\n"
                                                                             "    prev = output;\n"
                                                                             "    return output;\n"
                                                                             "}"
                                                                             ).arg(risingLimit).arg(fallingLimit);
                                                              }
                                                              );

    // Switch 블록
    m_blockInfoMap[Block::SWITCH].registerCodeGenerator("cpp",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            double threshold = props["threshold"].toDouble();
                                                            QString op = props["operator"].toString();
                                                            bool passFirst = props["passFirstInput"].toBool();

                                                            QString conditionCode;
                                                            if (op == ">=") conditionCode = "condition >= threshold";
                                                            else if (op == ">") conditionCode = "condition > threshold";
                                                            else if (op == "==") conditionCode = "std::abs(condition - threshold) < 1e-10";
                                                            else if (op == "!=") conditionCode = "std::abs(condition - threshold) >= 1e-10";
                                                            else if (op == "<") conditionCode = "condition < threshold";
                                                            else if (op == "<=") conditionCode = "condition <= threshold";
                                                            else conditionCode = "condition >= threshold";  // 기본값

                                                            return QString(
                                                                       "[](const std::vector<double>& inputs) {\n"
                                                                       "    if (inputs.size() < 3) return 0.0;\n"
                                                                       "    double input1 = inputs[0];\n"
                                                                       "    double condition = inputs[1];\n"
                                                                       "    double input2 = inputs[2];\n"
                                                                       "    double threshold = %1;\n"
                                                                       "    bool conditionMet = (%2);\n"
                                                                       "    return conditionMet ? %3 : %4;\n"
                                                                       "}"
                                                                       ).arg(threshold).arg(conditionCode)
                                                                .arg(passFirst ? "input1" : "input2")
                                                                .arg(passFirst ? "input2" : "input1");
                                                        }
                                                        );

    // Divide 블록
    m_blockInfoMap[Block::DIVIDE].registerCodeGenerator("cpp",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            return "[](const std::vector<double>& inputs) { if (inputs.size() < 2) return 0.0; double denominator = inputs[1]; if (std::abs(denominator) < 1e-10) return (inputs[0] >= 0) ? 1e10 : -1e10; return inputs[0] / denominator; }";
                                                        }
                                                        );

    // AND 블록
    m_blockInfoMap[Block::AND].registerCodeGenerator("cpp",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString(
                                                                    "[](const std::vector<double>& inputs) {\n"
                                                                    "    double threshold = %1;\n"
                                                                    "    for (double input : inputs) {\n"
                                                                    "        if (input < threshold) return 0.0;\n"
                                                                    "    }\n"
                                                                    "    return 1.0;\n"
                                                                    "}"
                                                                    ).arg(threshold);
                                                     }
                                                     );

    // OR 블록
    m_blockInfoMap[Block::OR].registerCodeGenerator("cpp",
                                                    [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                        double threshold = props["threshold"].toDouble();
                                                        return QString(
                                                                   "[](const std::vector<double>& inputs) {\n"
                                                                   "    double threshold = %1;\n"
                                                                   "    for (double input : inputs) {\n"
                                                                   "        if (input >= threshold) return 1.0;\n"
                                                                   "    }\n"
                                                                   "    return 0.0;\n"
                                                                   "}"
                                                                   ).arg(threshold);
                                                    }
                                                    );

    // NOT 블록
    m_blockInfoMap[Block::NOT].registerCodeGenerator("cpp",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString(
                                                                    "[](const std::vector<double>& inputs) {\n"
                                                                    "    if (inputs.empty()) return 1.0;\n"
                                                                    "    double threshold = %1;\n"
                                                                    "    return (inputs[0] < threshold) ? 1.0 : 0.0;\n"
                                                                    "}"
                                                                    ).arg(threshold);
                                                     }
                                                     );

    // XOR 블록
    m_blockInfoMap[Block::XOR].registerCodeGenerator("cpp",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString(
                                                                    "[](const std::vector<double>& inputs) {\n"
                                                                    "    double threshold = %1;\n"
                                                                    "    int trueCount = 0;\n"
                                                                    "    for (double input : inputs) {\n"
                                                                    "        if (input >= threshold) trueCount++;\n"
                                                                    "    }\n"
                                                                    "    return (trueCount %% 2 == 1) ? 1.0 : 0.0;\n"
                                                                    "}"
                                                                    ).arg(threshold);
                                                     }
                                                     );

    // Clock 블록
    m_blockInfoMap[Block::CLOCK].registerCodeGenerator("cpp",
                                                       [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                           double period = props["period"].toDouble();
                                                           double offset = props["offset"].toDouble();
                                                           return QString(
                                                                      "[this](const std::vector<double>& inputs) {\n"
                                                                      "    double t = m_currentTime;\n"
                                                                      "    double period = %1;\n"
                                                                      "    double offset = %2;\n"
                                                                      "    double adjustedTime = t - offset;\n"
                                                                      "    if (adjustedTime < 0) return 0.0;\n"
                                                                      "    return std::fmod(adjustedTime, period) / period;\n"
                                                                      "}"
                                                                      ).arg(period).arg(offset);
                                                       }
                                                       );

    // Ramp 블록
    m_blockInfoMap[Block::RAMP].registerCodeGenerator("cpp",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double slope = props["slope"].toDouble();
                                                          double startTime = props["startTime"].toDouble();
                                                          double initialOutput = props["initialOutput"].toDouble();
                                                          return QString(
                                                                     "[this](const std::vector<double>& inputs) {\n"
                                                                     "    double t = m_currentTime;\n"
                                                                     "    double startTime = %1;\n"
                                                                     "    double initialOutput = %2;\n"
                                                                     "    double slope = %3;\n"
                                                                     "    return t < startTime ? initialOutput : initialOutput + slope * (t - startTime);\n"
                                                                     "}"
                                                                     ).arg(startTime).arg(initialOutput).arg(slope);
                                                      }
                                                      );

    // Step 블록
    m_blockInfoMap[Block::STEP].registerCodeGenerator("cpp",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double stepTime = props["stepTime"].toDouble();
                                                          double initialValue = props["initialValue"].toDouble();
                                                          double finalValue = props["finalValue"].toDouble();
                                                          return QString(
                                                                     "[this](const std::vector<double>& inputs) {\n"
                                                                     "    double t = m_currentTime;\n"
                                                                     "    return t < %1 ? %2 : %3;\n"
                                                                     "}"
                                                                     ).arg(stepTime).arg(initialValue).arg(finalValue);
                                                      }
                                                      );

    // Sine Wave 블록
    m_blockInfoMap[Block::SINE_WAVE].registerCodeGenerator("cpp",
                                                           [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                               double amplitude = props["amplitude"].toDouble();
                                                               double frequency = props["frequency"].toDouble();
                                                               double phase = props["phase"].toDouble();
                                                               double bias = props["bias"].toDouble();
                                                               return QString(
                                                                          "[this](const std::vector<double>& inputs) {\n"
                                                                          "    double t = m_currentTime;\n"
                                                                          "    return %1 * std::sin(2 * M_PI * %2 * t + %3) + %4;\n"
                                                                          "}"
                                                                          ).arg(amplitude).arg(frequency).arg(phase).arg(bias);
                                                           }
                                                           );

    // Python 코드 생성 함수 등록
    m_blockInfoMap[Block::SOURCE].registerCodeGenerator("python",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            double value = props["value"].toDouble();
                                                            return QString("lambda inputs: %1").arg(value);
                                                        }
                                                        );

    m_blockInfoMap[Block::GAIN].registerCodeGenerator("python",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double gain = props["gain"].toDouble();
                                                          return QString("lambda inputs: inputs[0] * %1").arg(gain);
                                                      }
                                                      );

    m_blockInfoMap[Block::SUM].registerCodeGenerator("python",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "lambda inputs: sum(inputs)";
                                                     }
                                                     );

    // Product 블록
    m_blockInfoMap[Block::PRODUCT].registerCodeGenerator("python",
                                                         [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                             return "lambda inputs: np.prod(inputs) if inputs else 0.0";
                                                         }
                                                         );

    // Integrator 블록
    m_blockInfoMap[Block::INTEGRATOR].registerCodeGenerator("python",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                double initialValue = props["initial"].toDouble();
                                                                return QString("lambda inputs: self._integrate(inputs[0] if inputs else 0.0, %1)").arg(initialValue);
                                                            }
                                                            );

    // Derivative 블록
    m_blockInfoMap[Block::DERIVATIVE].registerCodeGenerator("python",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                return "lambda inputs: self._differentiate(inputs[0] if inputs else 0.0)";
                                                            }
                                                            );

    // Scope 블록
    m_blockInfoMap[Block::SCOPE].registerCodeGenerator("python",
                                                       [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                           return "lambda inputs: inputs[0] if inputs else 0.0";
                                                       }
                                                       );

    // In 블록
    m_blockInfoMap[Block::IN].registerCodeGenerator("python",
                                                    [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                        QString varName = props["variable"].toString();
                                                        return QString("lambda inputs: self.inputs.get('%1', 0.0)").arg(varName);
                                                    }
                                                    );

    // Out 블록
    m_blockInfoMap[Block::OUT].registerCodeGenerator("python",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "lambda inputs: inputs[0] if inputs else 0.0";
                                                     }
                                                     );

    // Lookup Table 1D 블록
    m_blockInfoMap[Block::LOOKUP_TABLE_1D].registerCodeGenerator("python",
                                                                 [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                     return "lambda inputs: self._lookup_table_1d(inputs[0] if inputs else 0.0)";
                                                                 }
                                                                 );

    // Lookup Table 2D 블록
    m_blockInfoMap[Block::LOOKUP_TABLE_2D].registerCodeGenerator("python",
                                                                 [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                     return "lambda inputs: self._lookup_table_2d(inputs[0] if inputs else 0.0, inputs[1] if len(inputs) > 1 else 0.0)";
                                                                 }
                                                                 );

    // Min 블록
    m_blockInfoMap[Block::MIN].registerCodeGenerator("python",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "lambda inputs: min(inputs) if inputs else 0.0";
                                                     }
                                                     );

    // Max 블록
    m_blockInfoMap[Block::MAX].registerCodeGenerator("python",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "lambda inputs: max(inputs) if inputs else 0.0";
                                                     }
                                                     );

    // Saturation 블록
    m_blockInfoMap[Block::SATURATION].registerCodeGenerator("python",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                double upperLimit = props["upperLimit"].toDouble();
                                                                double lowerLimit = props["lowerLimit"].toDouble();
                                                                return QString("lambda inputs: self._saturate(inputs[0] if inputs else 0.0, %1, %2)").arg(upperLimit).arg(lowerLimit);
                                                            }
                                                            );

    // Rate Limiter 블록
    m_blockInfoMap[Block::RATE_LIMITER].registerCodeGenerator("python",
                                                              [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                  double risingLimit = props["risingLimit"].toDouble();
                                                                  double fallingLimit = props["fallingLimit"].toDouble();
                                                                  return QString("lambda inputs: self._rate_limit(inputs[0] if inputs else 0.0, %1, %2)").arg(risingLimit).arg(fallingLimit);
                                                              }
                                                              );

    // Switch 블록
    m_blockInfoMap[Block::SWITCH].registerCodeGenerator("python",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            double threshold = props["threshold"].toDouble();
                                                            QString op = props["operator"].toString();
                                                            bool passFirst = props["passFirstInput"].toBool();

                                                            QString conditionCode;
                                                            if (op == ">=") conditionCode = "condition >= threshold";
                                                            else if (op == ">") conditionCode = "condition > threshold";
                                                            else if (op == "==") conditionCode = "abs(condition - threshold) < 1e-10";
                                                            else if (op == "!=") conditionCode = "abs(condition - threshold) >= 1e-10";
                                                            else if (op == "<") conditionCode = "condition < threshold";
                                                            else if (op == "<=") conditionCode = "condition <= threshold";
                                                            else conditionCode = "condition >= threshold";  // 기본값

                                                            return QString("lambda inputs: self._switch(inputs, %1, '%2', %3)").arg(threshold).arg(conditionCode).arg(passFirst ? "True" : "False");
                                                        }
                                                        );

    // Divide 블록
    m_blockInfoMap[Block::DIVIDE].registerCodeGenerator("python",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            return "lambda inputs: inputs[0] / inputs[1] if len(inputs) > 1 and abs(inputs[1]) > 1e-10 else float('inf')";
                                                        }
                                                        );

    // AND 블록
    m_blockInfoMap[Block::AND].registerCodeGenerator("python",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString("lambda inputs: 1.0 if all(x >= %1 for x in inputs) else 0.0").arg(threshold);
                                                     }
                                                     );

    // OR 블록
    m_blockInfoMap[Block::OR].registerCodeGenerator("python",
                                                    [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                        double threshold = props["threshold"].toDouble();
                                                        return QString("lambda inputs: 1.0 if any(x >= %1 for x in inputs) else 0.0").arg(threshold);
                                                    }
                                                    );

    // NOT 블록
    m_blockInfoMap[Block::NOT].registerCodeGenerator("python",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString("lambda inputs: 0.0 if inputs and inputs[0] >= %1 else 1.0").arg(threshold);
                                                     }
                                                     );

    // XOR 블록
    m_blockInfoMap[Block::XOR].registerCodeGenerator("python",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString("lambda inputs: 1.0 if sum(1 for x in inputs if x >= %1) %% 2 == 1 else 0.0").arg(threshold);
                                                     }
                                                     );

    // Clock 블록
    m_blockInfoMap[Block::CLOCK].registerCodeGenerator("python",
                                                       [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                           double period = props["period"].toDouble();
                                                           double offset = props["offset"].toDouble();
                                                           return QString("lambda inputs: self._clock_signal(%1, %2)").arg(period).arg(offset);
                                                       }
                                                       );

    // Ramp 블록
    m_blockInfoMap[Block::RAMP].registerCodeGenerator("python",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double slope = props["slope"].toDouble();
                                                          double startTime = props["startTime"].toDouble();
                                                          double initialOutput = props["initialOutput"].toDouble();
                                                          return QString("lambda inputs: self._ramp_signal(%1, %2, %3)").arg(slope).arg(startTime).arg(initialOutput);
                                                      }
                                                      );

    // Step 블록
    m_blockInfoMap[Block::STEP].registerCodeGenerator("python",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double stepTime = props["stepTime"].toDouble();
                                                          double initialValue = props["initialValue"].toDouble();
                                                          double finalValue = props["finalValue"].toDouble();
                                                          return QString("lambda inputs: self._step_signal(%1, %2, %3)").arg(stepTime).arg(initialValue).arg(finalValue);
                                                      }
                                                      );

    // Sine Wave 블록
    m_blockInfoMap[Block::SINE_WAVE].registerCodeGenerator("python",
                                                           [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                               double amplitude = props["amplitude"].toDouble();
                                                               double frequency = props["frequency"].toDouble();
                                                               double phase = props["phase"].toDouble();
                                                               double bias = props["bias"].toDouble();
                                                               return QString("lambda inputs: self._sine_signal(%1, %2, %3, %4)").arg(amplitude).arg(frequency).arg(phase).arg(bias);
                                                           }
                                                           );

    // Source 블록
    m_blockInfoMap[Block::SOURCE].registerCodeGenerator("capl",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            double value = props["value"].toDouble();
                                                            return QString("// SourceBlock\n  return %1;").arg(value);
                                                        }
                                                        );

    // Gain 블록
    m_blockInfoMap[Block::GAIN].registerCodeGenerator("capl",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double gain = props["gain"].toDouble();
                                                          return QString("// GainBlock\n  return inputs[0] * %1;").arg(gain);
                                                      }
                                                      );

    // Sum 블록
    m_blockInfoMap[Block::SUM].registerCodeGenerator("capl",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "// SumBlock\n  float sum = 0;\n  for (int i = 0; i < inputCount; i++) {\n    sum += inputs[i];\n  }\n  return sum;";
                                                     }
                                                     );

    // Product 블록
    m_blockInfoMap[Block::PRODUCT].registerCodeGenerator("capl",
                                                         [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                             return "// ProductBlock\n  float prod = 1.0;\n  for (int i = 0; i < inputCount; i++) {\n    prod *= inputs[i];\n  }\n  return prod;";
                                                         }
                                                         );

    // Integrator 블록
    m_blockInfoMap[Block::INTEGRATOR].registerCodeGenerator("capl",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                double initialValue = props["initial"].toDouble();
                                                                return QString("// IntegratorBlock\n  integratorState += inputs[0] * TIME_STEP;\n  return integratorState;");
                                                            }
                                                            );

    // Derivative 블록
    m_blockInfoMap[Block::DERIVATIVE].registerCodeGenerator("capl",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                return "// DerivativeBlock\n  if (!isInitialized) {\n    prevValue = inputs[0];\n    isInitialized = 1;\n    return 0.0;\n  }\n  float result = (inputs[0] - prevValue) / TIME_STEP;\n  prevValue = inputs[0];\n  return result;";
                                                            }
                                                            );

    // Scope 블록
    m_blockInfoMap[Block::SCOPE].registerCodeGenerator("capl",
                                                       [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                           return "// ScopeBlock\n  return inputs[0];";
                                                       }
                                                       );

    // In 블록
    m_blockInfoMap[Block::IN].registerCodeGenerator("capl",
                                                    [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                        QString varName = props["variable"].toString();
                                                        return QString("// InBlock\n  return input_%1;").arg(varName);
                                                    }
                                                    );

    // Out 블록
    m_blockInfoMap[Block::OUT].registerCodeGenerator("capl",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "// OutBlock\n  return inputs[0];";
                                                     }
                                                     );

    // LookupTable1D 블록
    m_blockInfoMap[Block::LOOKUP_TABLE_1D].registerCodeGenerator("capl",
                                                                 [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                     QString xDataStr = props["xData"].toString();
                                                                     QString yDataStr = props["yData"].toString();

                                                                     return "// LookupTable1DBlock\n  // Simple implementation - would need full table parsing\n  float x = inputs[0];\n  // Return default interpolated value\n  return x;";
                                                                 }
                                                                 );

    // LookupTable2D 블록
    m_blockInfoMap[Block::LOOKUP_TABLE_2D].registerCodeGenerator("capl",
                                                                 [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                     return "// LookupTable2DBlock\n  // Complex implementation required\n  return 0.0;";
                                                                 }
                                                                 );

    // Min 블록
    m_blockInfoMap[Block::MIN].registerCodeGenerator("capl",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "// MinBlock\n  if (inputCount < 2) return 0.0;\n  return (inputs[0] < inputs[1]) ? inputs[0] : inputs[1];";
                                                     }
                                                     );

    // Max 블록
    m_blockInfoMap[Block::MAX].registerCodeGenerator("capl",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         return "// MaxBlock\n  if (inputCount < 2) return 0.0;\n  return (inputs[0] > inputs[1]) ? inputs[0] : inputs[1];";
                                                     }
                                                     );

    // Saturation 블록
    m_blockInfoMap[Block::SATURATION].registerCodeGenerator("capl",
                                                            [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                double upperLimit = props["upperLimit"].toDouble();
                                                                double lowerLimit = props["lowerLimit"].toDouble();
                                                                return QString("// SaturationBlock\n  if (inputs[0] > %1) return %1;\n  if (inputs[0] < %2) return %2;\n  return inputs[0];").arg(upperLimit).arg(lowerLimit);
                                                            }
                                                            );

    // RateLimiter 블록
    m_blockInfoMap[Block::RATE_LIMITER].registerCodeGenerator("capl",
                                                              [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                                  double risingLimit = props["risingLimit"].toDouble();
                                                                  double fallingLimit = props["fallingLimit"].toDouble();

                                                                  return QString("// RateLimiterBlock\n  if (!isInitialized) {\n    prevOutput = inputs[0];\n    isInitialized = 1;\n    return inputs[0];\n  }\n  float rate = (inputs[0] - prevOutput) / TIME_STEP;\n  if (rate > %1) rate = %1;\n  else if (rate < %2) rate = %2;\n  float output = prevOutput + rate * TIME_STEP;\n  prevOutput = output;\n  return output;").arg(risingLimit).arg(fallingLimit);
                                                              }
                                                              );

    // Switch 블록
    m_blockInfoMap[Block::SWITCH].registerCodeGenerator("capl",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            double threshold = props["threshold"].toDouble();
                                                            QString op = props["operator"].toString();
                                                            bool passFirst = props["passFirstInput"].toBool();

                                                            QString condition;
                                                            if (op == ">=") condition = "condition >= threshold";
                                                            else if (op == ">") condition = "condition > threshold";
                                                            else if (op == "==") condition = "abs(condition - threshold) < 1e-10";
                                                            else if (op == "!=") condition = "abs(condition - threshold) >= 1e-10";
                                                            else if (op == "<") condition = "condition < threshold";
                                                            else if (op == "<=") condition = "condition <= threshold";
                                                            else condition = "condition >= threshold";  // 기본값

                                                            QString conditionCode = QString("// SwitchBlock\n  if (inputCount < 3) return 0.0;\n  float input1 = inputs[0];\n  float condition = inputs[1];\n  float input2 = inputs[2];\n  float threshold = %1;\n  if (%2) {\n    return %3;\n  } else {\n    return %4;\n  }")
                                                                                        .arg(threshold)
                                                                                        .arg(condition)
                                                                                        .arg(passFirst ? "input1" : "input2")
                                                                                        .arg(passFirst ? "input2" : "input1");

                                                            return conditionCode;
                                                        }
                                                        );

    // Divide 블록
    m_blockInfoMap[Block::DIVIDE].registerCodeGenerator("capl",
                                                        [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                            return "// DivideBlock\n  if (inputCount < 2) return 0.0;\n  if (abs(inputs[1]) < 1e-10) {\n    return (inputs[0] >= 0) ? 1e10 : -1e10;\n  }\n  return inputs[0] / inputs[1];";
                                                        }
                                                        );

    // AND 블록
    m_blockInfoMap[Block::AND].registerCodeGenerator("capl",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString("// ANDBlock\n  float threshold = %1;\n  for (int i = 0; i < inputCount; i++) {\n    if (inputs[i] < threshold) return 0.0;\n  }\n  return 1.0;").arg(threshold);
                                                     }
                                                     );

    // OR 블록
    m_blockInfoMap[Block::OR].registerCodeGenerator("capl",
                                                    [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                        double threshold = props["threshold"].toDouble();
                                                        return QString("// ORBlock\n  float threshold = %1;\n  for (int i = 0; i < inputCount; i++) {\n    if (inputs[i] >= threshold) return 1.0;\n  }\n  return 0.0;").arg(threshold);
                                                    }
                                                    );

    // NOT 블록
    m_blockInfoMap[Block::NOT].registerCodeGenerator("capl",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString("// NOTBlock\n  if (inputCount == 0) return 1.0;\n  float threshold = %1;\n  return (inputs[0] < threshold) ? 1.0 : 0.0;").arg(threshold);
                                                     }
                                                     );

    // XOR 블록
    m_blockInfoMap[Block::XOR].registerCodeGenerator("capl",
                                                     [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                         double threshold = props["threshold"].toDouble();
                                                         return QString("// XORBlock\n  float threshold = %1;\n  int trueCount = 0;\n  for (int i = 0; i < inputCount; i++) {\n    if (inputs[i] >= threshold) trueCount++;\n  }\n  return (trueCount %% 2 == 1) ? 1.0 : 0.0;").arg(threshold);
                                                     }
                                                     );

    // Clock 블록
    m_blockInfoMap[Block::CLOCK].registerCodeGenerator("capl",
                                                       [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                           double period = props["period"].toDouble();
                                                           double offset = props["offset"].toDouble();
                                                           return QString("// ClockBlock\n  float adjustedTime = currentTime - %1;\n  if (adjustedTime < 0) return 0.0;\n  return (adjustedTime %% %2) / %2;").arg(offset).arg(period);
                                                       }
                                                       );

    // Ramp 블록
    m_blockInfoMap[Block::RAMP].registerCodeGenerator("capl",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double slope = props["slope"].toDouble();
                                                          double startTime = props["startTime"].toDouble();
                                                          double initialOutput = props["initialOutput"].toDouble();
                                                          return QString("// RampBlock\n  if (currentTime < %1) return %2;\n  return %2 + %3 * (currentTime - %1);").arg(startTime).arg(initialOutput).arg(slope);
                                                      }
                                                      );

    // Step 블록
    m_blockInfoMap[Block::STEP].registerCodeGenerator("capl",
                                                      [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                          double stepTime = props["stepTime"].toDouble();
                                                          double initialValue = props["initialValue"].toDouble();
                                                          double finalValue = props["finalValue"].toDouble();
                                                          return QString("// StepBlock\n  return (currentTime < %1) ? %2 : %3;").arg(stepTime).arg(initialValue).arg(finalValue);
                                                      }
                                                      );

    // SineWave 블록
    m_blockInfoMap[Block::SINE_WAVE].registerCodeGenerator("capl",
                                                           [](const Block* block, const QMap<QString, QVariant>& props) -> QString {
                                                               double amplitude = props["amplitude"].toDouble();
                                                               double frequency = props["frequency"].toDouble();
                                                               double phase = props["phase"].toDouble();
                                                               double bias = props["bias"].toDouble();
                                                               return QString("// SineWaveBlock\n  return %1 * sin(2 * PI * %2 * currentTime + %3) + %4;").arg(amplitude).arg(frequency).arg(phase).arg(bias);
                                                           }
                                                           );
}
