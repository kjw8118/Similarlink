#include "codegenerator.h"

#include "similarlink.h"

CppCodeGenerator::CppCodeGenerator(SimulationScene* scene, QObject* parent)
    : QObject(parent), m_scene(scene) {

}

// 현재 다이어그램 상태를 C++ 코드로 생성
QString CppCodeGenerator::generateCode() {
    QString code;
    QTextStream stream(&code);

    // 헤더 및 라이브러리 포함
    stream << "#include <iostream>\n";
    stream << "#include <vector>\n";
    stream << "#include <cmath>\n";
    stream << "#include <functional>\n";
    stream << "#include <string>\n";
    stream << "#include <map>\n\n";

    // 네임스페이스 시작
    stream << "namespace SimulinkClone {\n\n";

    // 기본 함수 타입 정의
    stream << "// 기본 함수 타입 정의\n";
    stream << "using BlockFunction = std::function<double(const std::vector<double>&)>;\n\n";

    // 블록 클래스 정의
    generateBlockClass(stream);

    // 메인 시뮬레이션 클래스 정의
    generateSimulationClass(stream);

    // 시뮬레이션 인스턴스 생성
    generateSimulationInstance(stream);

    // 네임스페이스 종료
    stream << "} // namespace SimulinkClone\n\n";

    // 메인 함수
    stream << "int main() {\n";
    stream << "    // 시뮬레이션 실행\n";
    stream << "    SimulinkClone::runSimulation();\n";
    stream << "    return 0;\n";
    stream << "}\n";

    return code;
}


void CppCodeGenerator::generateBlockClass(QTextStream& stream) {
    stream << "// 블록 클래스 정의\n";
    stream << "class Block {\n";
    stream << "public:\n";
    stream << "    Block(const std::string& name, BlockFunction function)\n";
    stream << "        : m_name(name), m_function(function) {}\n\n";

    stream << "    double compute(const std::vector<double>& inputs) const {\n";
    stream << "        return m_function(inputs);\n";
    stream << "    }\n\n";

    stream << "    const std::string& getName() const { return m_name; }\n\n";

    stream << "private:\n";
    stream << "    std::string m_name;\n";
    stream << "    BlockFunction m_function;\n";
    stream << "};\n\n";
}
#include <iostream>
void CppCodeGenerator::generateSimulationClass(QTextStream& stream) {
    // 블록 및 연결 정보 수집
    QMap<Block*, int> blockIndices;
    QList<Block*> orderedBlocks;
    QList<QPair<int, int>> connections;
    QList<QPair<int, int>> inputConnections;
    QList<QPair<int, int>> outputConnections;

    // 인/아웃 블록 수집
    QList<InBlock*> inBlocks;
    QList<OutBlock*> outBlocks;

    // 시간 간격 가져오기
    double timeStep = 0.01; // 기본값
    Similarlink* mainWindow = qobject_cast<Similarlink*>(parent());
    if (mainWindow) {
        timeStep = mainWindow->getSimulationTimeStep();
    }

    // 블록 및 연결 정보 수집
    collectBlocksAndConnections(blockIndices, orderedBlocks, connections, inBlocks, outBlocks);

    // 시뮬레이션 클래스 정의
    stream << "// 시뮬레이션 클래스 정의\n";
    stream << "class Simulation {\n";
    stream << "public:\n";
    stream << "    Simulation() {\n";
    stream << "        // 블록 생성\n";

    /*// 블록 생성 코드
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];

        // 블록 타입에 따른 함수 생성
        QString functionCode;

        if (dynamic_cast<SourceBlock*>(block)) {
            double value = block->getProperties()["value"].toDouble();
            functionCode = QString("[](const std::vector<double>& inputs) { return %1; }").arg(value);
        }
        else if (dynamic_cast<GainBlock*>(block)) {
            double gain = block->getProperties()["gain"].toDouble();
            functionCode = QString("[](const std::vector<double>& inputs) { return inputs[0] * %1; }").arg(gain);
        }
        else if (dynamic_cast<SumBlock*>(block)) {
            functionCode = "[](const std::vector<double>& inputs) { double sum = 0; for (auto v : inputs) sum += v; return sum; }";
        }
        else if (dynamic_cast<ProductBlock*>(block)) {
            functionCode = "[](const std::vector<double>& inputs) { double prod = 1; for (auto v : inputs) prod *= v; return prod; }";
        }
        else if (dynamic_cast<IntegratorBlock*>(block)) {
            double initialValue = block->getProperties()["initial"].toDouble();
            functionCode = QString("// 적분기는 내부 상태가 필요\n        [this](const std::vector<double>& inputs) { static double state = %1; state += inputs[0] * %2; return state; }").arg(initialValue).arg(timeStep);
        }
        else if (dynamic_cast<DerivativeBlock*>(block)) {
            functionCode = QString("// 미분기는 이전 값이 필요\n        [this](const std::vector<double>& inputs) { static double prevValue = 0; double result = (inputs[0] - prevValue) / %1; prevValue = inputs[0]; return result; }").arg(timeStep);
        }
        else if (dynamic_cast<InBlock*>(block)) {
            // InBlock은 외부에서 값을 받음
            InBlock* inBlock = dynamic_cast<InBlock*>(block);
            QString varName = inBlock->getProperties()["variable"].toString();
            functionCode = QString("[this](const std::vector<double>& inputs) { return m_inputs[\"%1\"]; }").arg(varName);
        }
        else if (dynamic_cast<OutBlock*>(block)) {
            // OutBlock은 출력 저장
            functionCode = "[this](const std::vector<double>& inputs) { return inputs[0]; }";
        }
        // Signal Generator 계열 블록 처리
        else if (dynamic_cast<ClockBlock*>(block)) {
            double period = block->getProperties()["period"].toDouble();
            double offset = block->getProperties()["offset"].toDouble();
            functionCode = QString("[this](const std::vector<double>& inputs) { double t = m_currentTime; double period = %1; double offset = %2; double adjustedTime = t - offset; return adjustedTime < 0 ? 0 : std::fmod(adjustedTime, period) / period; }").arg(period).arg(offset);
        }
        else if (dynamic_cast<RampBlock*>(block)) {
            double slope = block->getProperties()["slope"].toDouble();
            double startTime = block->getProperties()["startTime"].toDouble();
            double initialOutput = block->getProperties()["initialOutput"].toDouble();
            functionCode = QString("[this](const std::vector<double>& inputs) { double t = m_currentTime; double startTime = %1; double initialOutput = %2; double slope = %3; return t < startTime ? initialOutput : initialOutput + slope * (t - startTime); }").arg(startTime).arg(initialOutput).arg(slope);
        }
        else if (dynamic_cast<StepBlock*>(block)) {
            double stepTime = block->getProperties()["stepTime"].toDouble();
            double initialValue = block->getProperties()["initialValue"].toDouble();
            double finalValue = block->getProperties()["finalValue"].toDouble();
            functionCode = QString("[this](const std::vector<double>& inputs) { double t = m_currentTime; return t < %1 ? %2 : %3; }").arg(stepTime).arg(initialValue).arg(finalValue);
        }
        else if (dynamic_cast<SineWaveBlock*>(block)) {
            double amplitude = block->getProperties()["amplitude"].toDouble();
            double frequency = block->getProperties()["frequency"].toDouble();
            double phase = block->getProperties()["phase"].toDouble();
            double bias = block->getProperties()["bias"].toDouble();
            functionCode = QString("[this](const std::vector<double>& inputs) { double t = m_currentTime; return %1 * std::sin(2 * M_PI * %2 * t + %3) + %4; }").arg(amplitude).arg(frequency).arg(phase).arg(bias);
        }
        // 룩업 테이블 블록 추가
        else if (dynamic_cast<LookupTable1DBlock*>(block)) {
            QString xDataStr = block->getProperties()["xData"].toString();
            QString yDataStr = block->getProperties()["yData"].toString();
            QString interpolation = block->getProperties()["interpolation"].toString();
            QString extrapolation = block->getProperties()["extrapolation"].toString();

            functionCode = QString(
                               "[](const std::vector<double>& inputs) {\n"
                               "            if (inputs.empty()) return 0.0;\n"
                               "            double x = inputs[0];\n"
                               "            // xData 및 yData 파싱\n"
                               "            std::vector<double> xData, yData;\n"
                               "            std::stringstream ssX(\"%1\");\n"
                               "            std::stringstream ssY(\"%2\");\n"
                               "            double val;\n"
                               "            while (ssX >> val) {\n"
                               "                xData.push_back(val);\n"
                               "                if (ssX.peek() == ',') ssX.ignore();\n"
                               "            }\n"
                               "            while (ssY >> val) {\n"
                               "                yData.push_back(val);\n"
                               "                if (ssY.peek() == ',') ssY.ignore();\n"
                               "            }\n"
                               "            if (xData.empty() || yData.empty() || xData.size() != yData.size()) return 0.0;\n\n"
                               "            // 범위를 벗어나는 경우\n"
                               "            if (x <= xData.front()) {\n"
                               "                // %3 (외삽 방법)\n"
                               "                return yData.front();\n"
                               "            } else if (x >= xData.back()) {\n"
                               "                return yData.back();\n"
                               "            }\n\n"
                               "            // 보간\n"
                               "            for (size_t i = 0; i < xData.size() - 1; i++) {\n"
                               "                if (x >= xData[i] && x <= xData[i+1]) {\n"
                               "                    // %4 (보간 방법)\n"
                               "                    double t = (x - xData[i]) / (xData[i+1] - xData[i]);\n"
                               "                    return yData[i] * (1 - t) + yData[i+1] * t;\n"
                               "                }\n"
                               "            }\n"
                               "            return 0.0;\n"
                               "        }"
                               ).arg(xDataStr).arg(yDataStr).arg(extrapolation).arg(interpolation);
        }
        else if (dynamic_cast<LookupTable2DBlock*>(block)) {
            QString xDataStr = block->getProperties()["xData"].toString();
            QString yDataStr = block->getProperties()["yData"].toString();
            QString zDataStr = block->getProperties()["zData"].toString();

            functionCode = QString(
                "[](const std::vector<double>& inputs) {\n"
                "            if (inputs.size() < 2) return 0.0;\n"
                "            double x = inputs[0];\n"
                "            double y = inputs[1];\n"
                "            // 간소화된 2D 테이블 구현 - 실제 구현에서는 더 복잡한 파싱 필요\n"
                "            // 기본 구현으로 가장 가까운 값 반환\n"
                "            return 0.0; // 실제 애플리케이션에서는 구현 필요\n"
                "        }"
                );
        }
        // 논리 게이트 블록 추가
        else if (dynamic_cast<AndBlock*>(block)) {
            double threshold = block->getProperties()["threshold"].toDouble();
            functionCode = QString(
                               "[](const std::vector<double>& inputs) {\n"
                               "            double threshold = %1;\n"
                               "            for (double input : inputs) {\n"
                               "                if (input < threshold) return 0.0;\n"
                               "            }\n"
                               "            return 1.0;\n"
                               "        }"
                               ).arg(threshold);
        }
        else if (dynamic_cast<OrBlock*>(block)) {
            double threshold = block->getProperties()["threshold"].toDouble();
            functionCode = QString(
                               "[](const std::vector<double>& inputs) {\n"
                               "            double threshold = %1;\n"
                               "            for (double input : inputs) {\n"
                               "                if (input >= threshold) return 1.0;\n"
                               "            }\n"
                               "            return 0.0;\n"
                               "        }"
                               ).arg(threshold);
        }
        else if (dynamic_cast<NotBlock*>(block)) {
            double threshold = block->getProperties()["threshold"].toDouble();
            functionCode = QString(
                               "[](const std::vector<double>& inputs) {\n"
                               "            if (inputs.empty()) return 1.0;\n"
                               "            double threshold = %1;\n"
                               "            return (inputs[0] < threshold) ? 1.0 : 0.0;\n"
                               "        }"
                               ).arg(threshold);
        }
        else if (dynamic_cast<XorBlock*>(block)) {
            double threshold = block->getProperties()["threshold"].toDouble();
            functionCode = QString(
                               "[](const std::vector<double>& inputs) {\n"
                               "            double threshold = %1;\n"
                               "            int trueCount = 0;\n"
                               "            for (double input : inputs) {\n"
                               "                if (input >= threshold) trueCount++;\n"
                               "            }\n"
                               "            return (trueCount %% 2 == 1) ? 1.0 : 0.0;\n"
                               "        }"
                               ).arg(threshold);
        }
        // 연산 및 제한 블록 추가
        else if (dynamic_cast<MinBlock*>(block)) {
            functionCode =
                "[](const std::vector<double>& inputs) {\n"
                "            if (inputs.size() < 2) return 0.0;\n"
                "            return std::min(inputs[0], inputs[1]);\n"
                "        }";
        }
        else if (dynamic_cast<MaxBlock*>(block)) {
            functionCode =
                "[](const std::vector<double>& inputs) {\n"
                "            if (inputs.size() < 2) return 0.0;\n"
                "            return std::max(inputs[0], inputs[1]);\n"
                "        }";
        }
        else if (dynamic_cast<DivideBlock*>(block)) {
            functionCode =
                "[](const std::vector<double>& inputs) {\n"
                "            if (inputs.size() < 2) return 0.0;\n"
                "            double denominator = inputs[1];\n"
                "            if (std::abs(denominator) < 1e-10) {\n"
                "                return (inputs[0] >= 0) ? 1e10 : -1e10;\n"
                "            }\n"
                "            return inputs[0] / denominator;\n"
                "        }";
        }
        else if (dynamic_cast<SaturationBlock*>(block)) {
            double upperLimit = block->getProperties()["upperLimit"].toDouble();
            double lowerLimit = block->getProperties()["lowerLimit"].toDouble();
            functionCode = QString(
                               "[](const std::vector<double>& inputs) {\n"
                               "            if (inputs.empty()) return 0.0;\n"
                               "            double input = inputs[0];\n"
                               "            double upperLimit = %1;\n"
                               "            double lowerLimit = %2;\n"
                               "            if (input > upperLimit) return upperLimit;\n"
                               "            if (input < lowerLimit) return lowerLimit;\n"
                               "            return input;\n"
                               "        }"
                               ).arg(upperLimit).arg(lowerLimit);
        }
        else if (dynamic_cast<RateLimiterBlock*>(block)) {
            double risingLimit = block->getProperties()["risingLimit"].toDouble();
            double fallingLimit = block->getProperties()["fallingLimit"].toDouble();
            double sampleTime = block->getProperties()["sampleTime"].toDouble();

            functionCode = QString(
                               "[this](const std::vector<double>& inputs) {\n"
                               "            if (inputs.empty()) return 0.0;\n"
                               "            static double prevOutput = 0.0;\n"
                               "            static bool initialized = false;\n"
                               "            \n"
                               "            double input = inputs[0];\n"
                               "            double risingLimit = %1;\n"
                               "            double fallingLimit = %2;\n"
                               "            double sampleTime = %3;\n"
                               "            \n"
                               "            if (!initialized) {\n"
                               "                prevOutput = input;\n"
                               "                initialized = true;\n"
                               "                return input;\n"
                               "            }\n"
                               "            \n"
                               "            double rate = (input - prevOutput) / sampleTime;\n"
                               "            if (rate > risingLimit) rate = risingLimit;\n"
                               "            else if (rate < fallingLimit) rate = fallingLimit;\n"
                               "            \n"
                               "            double output = prevOutput + rate * sampleTime;\n"
                               "            prevOutput = output;\n"
                               "            return output;\n"
                               "        }"
                               ).arg(risingLimit).arg(fallingLimit).arg(sampleTime);
        }
        else if (dynamic_cast<SwitchBlock*>(block)) {
            double threshold = block->getProperties()["threshold"].toDouble();
            QString op = block->getProperties()["operator"].toString();
            bool passFirst = block->getProperties()["passFirstInput"].toBool();

            QString conditionCode;
            if (op == ">=") conditionCode = "condition >= threshold";
            else if (op == ">") conditionCode = "condition > threshold";
            else if (op == "==") conditionCode = "std::abs(condition - threshold) < 1e-10";
            else if (op == "!=") conditionCode = "std::abs(condition - threshold) >= 1e-10";
            else if (op == "<") conditionCode = "condition < threshold";
            else if (op == "<=") conditionCode = "condition <= threshold";

            functionCode = QString(
                               "[](const std::vector<double>& inputs) {\n"
                               "            if (inputs.size() < 3) return 0.0;\n"
                               "            double input1 = inputs[0];\n"
                               "            double condition = inputs[1];\n"
                               "            double input2 = inputs[2];\n"
                               "            double threshold = %1;\n"
                               "            \n"
                               "            bool conditionMet = (%2);\n"
                               "            return conditionMet ? %3 : %4;\n"
                               "        }"
                               ).arg(threshold).arg(conditionCode)
                               .arg(passFirst ? "input1" : "input2")
                               .arg(passFirst ? "input2" : "input1");
        }
        // 기본 경우
        else {
            functionCode = "[](const std::vector<double>& inputs) { return 0.0; }";
        }

        stream << "        m_blocks.push_back(Block(\"" << block->getName() << "\", " << functionCode << "));\n";
    }*/
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];
        std::cout << block->getName().toStdString() << std::endl;
        // 블록 팩토리를 통해 코드 생성
        QString functionCode = BlockFactory::instance()->generateCode("cpp", block);

        // 코드 생성이 실패한 경우 기본 구현 사용
        if (functionCode.isEmpty()) {
            // 기존의 하드코딩된 방식으로 폴백
            // ...
        }

        stream << "        m_blocks.push_back(Block(\"" << block->getName() << "\", " << functionCode << "));\n";
    }

    stream << "\n        // 연결 정의\n";
    for (const auto& conn : connections) {
        stream << "        m_connections.push_back({" << conn.first << ", " << conn.second << "});\n";
    }

    stream << "    }\n\n";

    // 시뮬레이션 실행 함수 - 지속 시간과 시간 간격 매개변수 추가
    double duration = 10.0; // 기본값
    if (mainWindow) {
        duration = mainWindow->getSimulationDuration();
    }

    stream << "    void run(double duration = " << duration
           << ", double timeStep = " << timeStep << ") {\n";

    stream << "        // 입력값 설정 (예시)\n";
    for (const auto& inBlock : inBlocks) {
        QString varName = inBlock->getProperties()["variable"].toString();
        stream << "        m_inputs[\"" << varName << "\"] = 1.0; // 기본값 설정\n";
    }

    stream << "\n        // 시뮬레이션 루프\n";
    stream << "        for (double t = 0; t < duration; t += timeStep) {\n";
    stream << "            m_currentTime = t; // 현재 시간 업데이트\n";
    stream << "            // 블록 계산\n";
    stream << "            std::map<int, double> outputs;\n";

    // 개선된 토폴로지 정렬 기반 계산 알고리즘
    stream << "            // 토폴로지 정렬 기반 블록 계산\n";
    stream << "            bool progress = true;\n";
    stream << "            int iteration = 0;\n";
    stream << "            int maxIterations = m_blocks.size() * 2; // 최대 반복 횟수 제한\n\n";

    stream << "            // 모든 블록이 계산될 때까지 반복\n";
    stream << "            while (progress && iteration < maxIterations) {\n";
    stream << "                progress = false;\n";
    stream << "                iteration++;\n\n";

    stream << "                for (int i = 0; i < m_blocks.size(); i++) {\n";
    stream << "                    // 이미 계산된 블록은 건너뜀\n";
    stream << "                    if (outputs.find(i) != outputs.end()) continue;\n\n";

    stream << "                    std::vector<double> inputs;\n";
    stream << "                    bool allInputsReady = true;\n\n";

    stream << "                    // 현재 블록의 모든 입력 수집\n";
    stream << "                    for (const auto& conn : m_connections) {\n";
    stream << "                        if (conn.second == i) {\n";
    stream << "                            // 입력 블록의 출력이 아직 계산되지 않았으면 건너뜀\n";
    stream << "                            if (outputs.find(conn.first) == outputs.end()) {\n";
    stream << "                                allInputsReady = false;\n";
    stream << "                                break;\n";
    stream << "                            }\n";
    stream << "                            inputs.push_back(outputs[conn.first]);\n";
    stream << "                        }\n";
    stream << "                    }\n\n";

    stream << "                    // 모든 입력이 준비되면 블록 계산\n";
    stream << "                    if (allInputsReady) {\n";
    stream << "                        outputs[i] = m_blocks[i].compute(inputs);\n";
    stream << "                        progress = true; // 하나라도 계산되면 진행 중으로 표시\n";
    stream << "                    }\n";
    stream << "                }\n";
    stream << "            }\n\n";

    // 출력 표시
    stream << "            // 출력값 표시\n";
    for (const auto& outBlock : outBlocks) {
        int blockIndex = blockIndices[outBlock];
        QString varName = outBlock->getProperties()["variable"].toString();
        stream << "            std::cout << \"t=\" << t << \", " << varName << "=\" << outputs[" << blockIndex << "] << std::endl;\n";
    }

    stream << "        }\n";
    stream << "    }\n\n";

    // 멤버 변수
    stream << "private:\n";
    stream << "    std::vector<Block> m_blocks;\n";
    stream << "    std::vector<std::pair<int, int>> m_connections; // 소스 블록 인덱스 -> 대상 블록 인덱스\n";
    stream << "    std::map<std::string, double> m_inputs; // 입력 변수 -> 값\n";
    stream << "    double m_currentTime = 0.0; // 현재 시뮬레이션 시간\n";
    stream << "};\n\n";
}
void CppCodeGenerator::generateSimulationInstance(QTextStream& stream) {

    // 현재 설정된 시뮬레이션 지속 시간 가져오기
    double duration = 10.0; // 기본값
    double timeStep = 0.01; // 기본값

    // Similarlink 지속 시간 가져오기 (this가 CodeGenerator 인스턴스임을 가정)
    Similarlink* mainWindow = qobject_cast<Similarlink*>(parent());
    if (mainWindow) {
        // 여기서는 m_durationSpinBox에 직접 접근할 수 없으므로
        // MainWindow에 getDuration() 같은 메서드를 추가하는 것이 좋습니다
        duration = mainWindow->getSimulationDuration();
        timeStep = mainWindow->getSimulationTimeStep();
    }

    stream << "// 시뮬레이션 실행 함수\n";
    stream << "void runSimulation() {\n";
    stream << "    Simulation sim;\n";
    stream << "    sim.run(" << duration << ", " << timeStep << "); // 사용자 설정 지속 시간 및 시간 간격\n";
    stream << "}\n\n";
}

// 블록과 연결 정보 수집
void CppCodeGenerator::collectBlocksAndConnections(QMap<Block*, int>& blockIndices,
                                 QList<Block*>& orderedBlocks,
                                 QList<QPair<int, int>>& connections,
                                 QList<InBlock*>& inBlocks,
                                 QList<OutBlock*>& outBlocks) {
    // 모든 블록 수집 및 인덱스 할당
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block) {
            blockIndices[block] = orderedBlocks.size();
            orderedBlocks.append(block);

            // In/Out 블록 식별
            if (InBlock* inBlock = dynamic_cast<InBlock*>(block)) {
                inBlocks.append(inBlock);
            }
            else if (OutBlock* outBlock = dynamic_cast<OutBlock*>(block)) {
                outBlocks.append(outBlock);
            }
        }
    }

    // 연결 수집
    for (auto connection : m_scene->getConnections()) {
        Block* sourceBlock = connection->getSourceBlock();
        Block* destBlock = connection->getDestBlock();

        if (blockIndices.contains(sourceBlock) && blockIndices.contains(destBlock)) {
            int sourceIndex = blockIndices[sourceBlock];
            int destIndex = blockIndices[destBlock];
            connections.append(qMakePair(sourceIndex, destIndex));
        }
    }
}







PythonCodeGenerator::PythonCodeGenerator(SimulationScene* scene, QObject* parent)
    : QObject(parent), m_scene(scene) {
}

// 현재 다이어그램 상태를 Python 코드로 생성
QString PythonCodeGenerator::generateCode() {
    QString code;
    QTextStream stream(&code);

    // 필요한 라이브러리 임포트
    stream << "import numpy as np\n";
    stream << "import matplotlib.pyplot as plt\n";
    stream << "from typing import List, Dict, Callable, Tuple\n\n";

    // Block 클래스 정의
    generateBlockClass(stream);

    // 메인 시뮬레이션 클래스 정의
    generateSimulationClass(stream);

    // 시뮬레이션 인스턴스 생성
    generateSimulationInstance(stream);

    // 메인 실행 부분
    stream << "if __name__ == '__main__':\n";
    stream << "    run_simulation()\n";

    return code;
}


void PythonCodeGenerator::generateBlockClass(QTextStream& stream) {
    stream << "# Block 클래스 정의\n";
    stream << "class Block:\n";
    stream << "    def __init__(self, name: str, compute_function: Callable[[List[float]], float]):\n";
    stream << "        self.name = name\n";
    stream << "        self.compute_function = compute_function\n\n";

    stream << "    def compute(self, inputs: List[float]) -> float:\n";
    stream << "        return self.compute_function(inputs)\n\n";
}

void PythonCodeGenerator::generateSimulationClass(QTextStream& stream) {
    // 블록 및 연결 정보 수집
    QMap<Block*, int> blockIndices;
    QList<Block*> orderedBlocks;
    QList<QPair<int, int>> connections;
    QList<InBlock*> inBlocks;
    QList<OutBlock*> outBlocks;

    // 시간 간격 가져오기
    double timeStep = 0.01; // 기본값
    Similarlink* mainWindow = qobject_cast<Similarlink*>(parent());
    if (mainWindow) {
        timeStep = mainWindow->getSimulationTimeStep();
    }

    // 블록 및 연결 정보 수집
    collectBlocksAndConnections(blockIndices, orderedBlocks, connections, inBlocks, outBlocks);

    // 시뮬레이션 클래스 정의
    stream << "# 시뮬레이션 클래스 정의\n";
    stream << "class Simulation:\n";
    stream << "    def __init__(self):\n";
    stream << "        # 현재 시간 초기화\n";
    stream << "        self.current_time = 0.0\n";
    stream << "        # 블록 목록 초기화\n";
    stream << "        self.blocks = []\n";
    stream << "        # 연결 정보 초기화 (source_idx, dest_idx)\n";
    stream << "        self.connections = []\n";
    stream << "        # 입력 변수 초기화\n";
    stream << "        self.inputs = {}\n\n";

    stream << "        # 블록 생성\n";

    /*// 블록 생성 코드
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];

        // 블록 타입에 따른 함수 생성
        QString functionCode;

        if (dynamic_cast<SourceBlock*>(block)) {
            double value = block->getProperties()["value"].toDouble();
            functionCode = QString("lambda inputs: %1").arg(value);
        }
        else if (dynamic_cast<GainBlock*>(block)) {
            double gain = block->getProperties()["gain"].toDouble();
            functionCode = QString("lambda inputs: inputs[0] * %1").arg(gain);
        }
        else if (dynamic_cast<SumBlock*>(block)) {
            functionCode = "lambda inputs: sum(inputs)";
        }
        else if (dynamic_cast<ProductBlock*>(block)) {
            functionCode = "lambda inputs: np.prod(inputs) if inputs else 0.0";
        }
        else if (dynamic_cast<IntegratorBlock*>(block)) {
            double initialValue = block->getProperties()["initial"].toDouble();
            functionCode = QString(
                               "lambda inputs: self._integrate(inputs[0] if inputs else 0.0, %1)").arg(initialValue);

            // 적분 메서드 추가 (클래스 끝에 추가할 예정)
            stream << "    def _integrate(self, input_value, initial_state=0.0):\n";
            stream << "        # 상태 유지를 위한 클로저 사용\n";
            stream << "        if not hasattr(self._integrate, 'state'):\n";
            stream << "            self._integrate.state = initial_state\n";
            stream << "        # 시간 간격 설정\n";
            stream << "        dt = " << timeStep << "\n";
            stream << "        # 적분 계산\n";
            stream << "        self._integrate.state += input_value * dt\n";
            stream << "        return self._integrate.state\n\n";
        }
        else if (dynamic_cast<DerivativeBlock*>(block)) {
            functionCode = QString(
                "lambda inputs: self._differentiate(inputs[0] if inputs else 0.0)");

            // 미분 메서드 추가
            stream << "    def _differentiate(self, input_value):\n";
            stream << "        # 상태 유지를 위한 클로저 사용\n";
            stream << "        if not hasattr(self._differentiate, 'prev_value'):\n";
            stream << "            self._differentiate.prev_value = input_value\n";
            stream << "            return 0.0\n";
            stream << "        # 시간 간격 설정\n";
            stream << "        dt = " << timeStep << "\n";
            stream << "        # 미분 계산\n";
            stream << "        result = (input_value - self._differentiate.prev_value) / dt\n";
            stream << "        self._differentiate.prev_value = input_value\n";
            stream << "        return result\n\n";
        }
        else if (dynamic_cast<InBlock*>(block)) {
            // InBlock은 외부에서 값을 받음
            InBlock* inBlock = dynamic_cast<InBlock*>(block);
            QString varName = inBlock->getProperties()["variable"].toString();
            functionCode = QString("lambda inputs: self.inputs.get('%1', 0.0)").arg(varName);
        }
        else if (dynamic_cast<OutBlock*>(block)) {
            // OutBlock은 출력 저장
            functionCode = "lambda inputs: inputs[0] if inputs else 0.0";
        }
        // Signal Generator 계열 블록 처리
        else if (dynamic_cast<ClockBlock*>(block)) {
            double period = block->getProperties()["period"].toDouble();
            double offset = block->getProperties()["offset"].toDouble();
            functionCode = QString(
                               "lambda inputs: self._clock_signal(%1, %2)").arg(period).arg(offset);

            // 클럭 신호 메서드 추가
            stream << "    def _clock_signal(self, period, offset):\n";
            stream << "        # 클럭 신호 생성\n";
            stream << "        t = self.current_time\n";
            stream << "        adjusted_time = t - offset\n";
            stream << "        if adjusted_time < 0:\n";
            stream << "            return 0.0\n";
            stream << "        return (adjusted_time % period) / period\n\n";
        }
        else if (dynamic_cast<RampBlock*>(block)) {
            double slope = block->getProperties()["slope"].toDouble();
            double startTime = block->getProperties()["startTime"].toDouble();
            double initialOutput = block->getProperties()["initialOutput"].toDouble();
            functionCode = QString(
                               "lambda inputs: self._ramp_signal(%1, %2, %3)").arg(slope).arg(startTime).arg(initialOutput);

            // 램프 신호 메서드 추가
            stream << "    def _ramp_signal(self, slope, start_time, initial_output):\n";
            stream << "        # 램프 신호 생성\n";
            stream << "        t = self.current_time\n";
            stream << "        if t < start_time:\n";
            stream << "            return initial_output\n";
            stream << "        return initial_output + slope * (t - start_time)\n\n";
        }
        else if (dynamic_cast<StepBlock*>(block)) {
            double stepTime = block->getProperties()["stepTime"].toDouble();
            double initialValue = block->getProperties()["initialValue"].toDouble();
            double finalValue = block->getProperties()["finalValue"].toDouble();
            functionCode = QString(
                               "lambda inputs: self._step_signal(%1, %2, %3)").arg(stepTime).arg(initialValue).arg(finalValue);

            // 스텝 신호 메서드 추가
            stream << "    def _step_signal(self, step_time, initial_value, final_value):\n";
            stream << "        # 스텝 신호 생성\n";
            stream << "        t = self.current_time\n";
            stream << "        return initial_value if t < step_time else final_value\n\n";
        }
        else if (dynamic_cast<SineWaveBlock*>(block)) {
            double amplitude = block->getProperties()["amplitude"].toDouble();
            double frequency = block->getProperties()["frequency"].toDouble();
            double phase = block->getProperties()["phase"].toDouble();
            double bias = block->getProperties()["bias"].toDouble();
            functionCode = QString(
                               "lambda inputs: self._sine_signal(%1, %2, %3, %4)")
                               .arg(amplitude).arg(frequency).arg(phase).arg(bias);

            // 사인파 신호 메서드 추가
            stream << "    def _sine_signal(self, amplitude, frequency, phase, bias):\n";
            stream << "        # 사인파 신호 생성\n";
            stream << "        t = self.current_time\n";
            stream << "        return amplitude * np.sin(2 * np.pi * frequency * t + phase) + bias\n\n";
        }
        // 제한 블록 추가
        else if (dynamic_cast<SaturationBlock*>(block)) {
            double upperLimit = block->getProperties()["upperLimit"].toDouble();
            double lowerLimit = block->getProperties()["lowerLimit"].toDouble();
            functionCode = QString(
                               "lambda inputs: self._saturate(inputs[0] if inputs else 0.0, %1, %2)")
                               .arg(upperLimit).arg(lowerLimit);

            // 새튜레이션 메서드 추가
            stream << "    def _saturate(self, input_value, upper_limit, lower_limit):\n";
            stream << "        # 값 제한\n";
            stream << "        if input_value > upper_limit:\n";
            stream << "            return upper_limit\n";
            stream << "        if input_value < lower_limit:\n";
            stream << "            return lower_limit\n";
            stream << "        return input_value\n\n";
        }
        // 기타 블록 타입들에 대한 함수 정의...
        else {
            functionCode = "lambda inputs: 0.0  # 미구현 블록 타입";
        }

        stream << "        self.blocks.append(Block(\"" << block->getName() << "\", " << functionCode << "))\n";
    }
    */
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];

        // 블록 팩토리를 통해 파이썬 코드 생성
        QString functionCode = BlockFactory::instance()->generateCode("python", block);

        // 코드 생성이 실패한 경우 기본 구현 사용
        if (functionCode.isEmpty()) {
            // 기본 구현으로 폴백

        }

        stream << "        self.blocks.append(Block(\"" << block->getName() << "\", " << functionCode << "))\n";
    }

    stream << "\n        # 연결 정의\n";
    for (const auto& conn : connections) {
        stream << "        self.connections.append((" << conn.first << ", " << conn.second << "))  # "
               << orderedBlocks[conn.first]->getName() << " -> "
               << orderedBlocks[conn.second]->getName() << "\n";
    }

    // 실행 메서드
    double duration = 10.0; // 기본값
    if (mainWindow) {
        duration = mainWindow->getSimulationDuration();
    }

    stream << "\n    def run(self, duration=" << duration << ", time_step=" << timeStep << "):\n";
    stream << "        \"\"\"시뮬레이션 실행\"\"\"\n";
    stream << "        # 입력값 설정 (예시)\n";
    for (const auto& inBlock : inBlocks) {
        QString varName = inBlock->getProperties()["variable"].toString();
        stream << "        self.inputs[\"" << varName << "\"] = 1.0  # 기본값 설정\n";
    }

    stream << "\n        # 결과 저장용 딕셔너리\n";
    stream << "        results = {\"time\": []}\n";
    for (const auto& outBlock : outBlocks) {
        QString varName = outBlock->getProperties()["variable"].toString();
        stream << "        results[\"" << varName << "\"] = []\n";
    }

    stream << "\n        # 시뮬레이션 루프\n";
    stream << "        for t in np.arange(0, duration, time_step):\n";
    stream << "            self.current_time = t  # 현재 시간 업데이트\n";
    stream << "            \n";
    stream << "            # 블록 계산\n";
    stream << "            outputs = {}  # 블록 출력값 저장\n";
    stream << "            \n";
    stream << "            # 토폴로지 정렬 기반 블록 계산\n";
    stream << "            progress = True\n";
    stream << "            iteration = 0\n";
    stream << "            max_iterations = len(self.blocks) * 2  # 최대 반복 횟수 제한\n";
    stream << "            \n";
    stream << "            # 모든 블록이 계산될 때까지 반복\n";
    stream << "            while progress and iteration < max_iterations:\n";
    stream << "                progress = False\n";
    stream << "                iteration += 1\n";
    stream << "                \n";
    stream << "                for i in range(len(self.blocks)):\n";
    stream << "                    # 이미 계산된 블록은 건너뜀\n";
    stream << "                    if i in outputs:\n";
    stream << "                        continue\n";
    stream << "                    \n";
    stream << "                    inputs = []\n";
    stream << "                    all_inputs_ready = True\n";
    stream << "                    \n";
    stream << "                    # 현재 블록의 모든 입력 수집\n";
    stream << "                    for src, dest in self.connections:\n";
    stream << "                        if dest == i:\n";
    stream << "                            # 입력 블록의 출력이 아직 계산되지 않았으면 건너뜀\n";
    stream << "                            if src not in outputs:\n";
    stream << "                                all_inputs_ready = False\n";
    stream << "                                break\n";
    stream << "                            inputs.append(outputs[src])\n";
    stream << "                    \n";
    stream << "                    # 모든 입력이 준비되면 블록 계산\n";
    stream << "                    if all_inputs_ready:\n";
    stream << "                        outputs[i] = self.blocks[i].compute(inputs)\n";
    stream << "                        progress = True  # 하나라도 계산되면 진행 중으로 표시\n";
    stream << "            \n";
    stream << "            # 결과 저장\n";
    stream << "            results[\"time\"].append(t)\n";

    for (const auto& outBlock : outBlocks) {
        int blockIndex = blockIndices[outBlock];
        QString varName = outBlock->getProperties()["variable"].toString();
        stream << "            results[\"" << varName << "\"].append(outputs.get(" << blockIndex << ", 0.0))\n";
        stream << "            print(f\"t={t:.3f}, " << varName << "={outputs.get(" << blockIndex << ", 0.0):.6f}\")\n";
    }

    stream << "        \n";
    stream << "        return results\n\n";

    // 시각화 메서드 추가
    stream << "    def visualize_results(self, results):\n";
    stream << "        \"\"\"결과 시각화\"\"\"\n";
    stream << "        plt.figure(figsize=(10, 6))\n";
    stream << "        \n";
    stream << "        # 모든 출력값 그래프로 표시\n";
    for (const auto& outBlock : outBlocks) {
        QString varName = outBlock->getProperties()["variable"].toString();
        stream << "        plt.plot(results[\"time\"], results[\"" << varName << "\"], label=\"" << varName << "\")\n";
    }

    stream << "        \n";
    stream << "        plt.title(\"Simulation Results\")\n";
    stream << "        plt.xlabel(\"Time (s)\")\n";
    stream << "        plt.ylabel(\"Value\")\n";
    stream << "        plt.grid(True)\n";
    stream << "        plt.legend()\n";
    stream << "        plt.tight_layout()\n";
    stream << "        plt.show()\n\n";
}

void PythonCodeGenerator::generateSimulationInstance(QTextStream& stream) {
    // 현재 설정된 시뮬레이션 지속 시간 가져오기
    double duration = 10.0; // 기본값
    double timeStep = 0.01; // 기본값

    // Similarlink 지속 시간 가져오기
    Similarlink* mainWindow = qobject_cast<Similarlink*>(parent());
    if (mainWindow) {
        duration = mainWindow->getSimulationDuration();
        timeStep = mainWindow->getSimulationTimeStep();
    }

    stream << "# 시뮬레이션 실행 함수\n";
    stream << "def run_simulation():\n";
    stream << "    # 시뮬레이션 생성\n";
    stream << "    sim = Simulation()\n";
    stream << "    \n";
    stream << "    # 시뮬레이션 실행\n";
    stream << "    print(\"시뮬레이션 시작...\")\n";
    stream << "    results = sim.run(" << duration << ", " << timeStep << ")  # 사용자 설정 지속 시간 및 시간 간격\n";
    stream << "    print(\"시뮬레이션 완료!\")\n";
    stream << "    \n";
    stream << "    # 결과 시각화\n";
    stream << "    sim.visualize_results(results)\n";
    stream << "    \n";
    stream << "    return results\n\n";
}

// 블록과 연결 정보 수집
void PythonCodeGenerator::collectBlocksAndConnections(QMap<Block*, int>& blockIndices,
                                                      QList<Block*>& orderedBlocks,
                                                      QList<QPair<int, int>>& connections,
                                                      QList<InBlock*>& inBlocks,
                                                      QList<OutBlock*>& outBlocks) {
    // 모든 블록 수집 및 인덱스 할당
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block) {
            blockIndices[block] = orderedBlocks.size();
            orderedBlocks.append(block);

            // In/Out 블록 식별
            if (InBlock* inBlock = dynamic_cast<InBlock*>(block)) {
                inBlocks.append(inBlock);
            }
            else if (OutBlock* outBlock = dynamic_cast<OutBlock*>(block)) {
                outBlocks.append(outBlock);
            }
        }
    }

    // 연결 수집
    for (auto connection : m_scene->getConnections()) {
        Block* sourceBlock = connection->getSourceBlock();
        Block* destBlock = connection->getDestBlock();

        if (blockIndices.contains(sourceBlock) && blockIndices.contains(destBlock)) {
            int sourceIndex = blockIndices[sourceBlock];
            int destIndex = blockIndices[destBlock];
            connections.append(qMakePair(sourceIndex, destIndex));
        }
    }
}















CaplCodeGenerator::CaplCodeGenerator(SimulationScene* scene, QObject* parent)
    : QObject(parent), m_scene(scene) {
}

// 현재 다이어그램 상태를 CAPL 코드로 생성
QString CaplCodeGenerator::generateCode() {
    QString code;
    QTextStream stream(&code);

    // CAPL 파일 헤더 주석
    stream << "/*********************************************************************\n";
    stream << " * CAPL 코드 생성기\n";
    stream << " * Generated by Similarlink\n";
    stream << " *\n";
    stream << " * 이 파일은 자동 생성되었습니다.\n";
    stream << " *********************************************************************/\n\n";

    // 변수 선언 및 초기화
    generateVariables(stream);

    // 시그널 핸들러
    generateSignalHandlers(stream);

    // 주요 함수
    generateMainFunctions(stream);

    // 블록 함수 구현
    generateBlockFunctions(stream);

    // 유틸리티 함수
    generateUtilityFunctions(stream);

    return code;
}


void CaplCodeGenerator::generateVariables(QTextStream& stream) {
    // 블록 및 연결 정보 수집
    QMap<Block*, int> blockIndices;
    QList<Block*> orderedBlocks;
    QList<QPair<int, int>> connections;
    QList<InBlock*> inBlocks;
    QList<OutBlock*> outBlocks;

    collectBlocksAndConnections(blockIndices, orderedBlocks, connections, inBlocks, outBlocks);

    // 시간 간격 가져오기
    double timeStep = 0.01; // 기본값
    Similarlink* mainWindow = qobject_cast<Similarlink*>(parent());
    if (mainWindow) {
        timeStep = mainWindow->getSimulationTimeStep();
    }

    // 모든 상수 정의
    stream << "// 상수 정의\n";
    stream << "const float TIME_STEP = " << timeStep << ";  // 시뮬레이션 시간 간격 (초)\n";

    double duration = 10.0; // 기본값
    if (mainWindow) {
        duration = mainWindow->getSimulationDuration();
    }
    stream << "const float SIMULATION_DURATION = " << duration << ";  // 시뮬레이션 지속 시간 (초)\n\n";

    // 전역 변수 선언
    stream << "// 전역 변수\n";
    stream << "float currentTime = 0.0;  // 현재 시뮬레이션 시간\n";
    stream << "float blockOutputs[" << orderedBlocks.size() << "];  // 각 블록의 출력값\n";
    stream << "byte blocksCalculated[" << orderedBlocks.size() << "];  // 계산 완료된 블록 표시\n\n";

    // 특수 블록을 위한 상태 변수
    stream << "// 블록 상태 변수\n";
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];

        if (dynamic_cast<IntegratorBlock*>(block)) {
            double initialValue = block->getProperties()["initial"].toDouble();
            stream << "float integratorState_" << i << " = " << initialValue << ";  // 적분기 상태 (" << block->getName() << ")\n";
        }
        else if (dynamic_cast<DerivativeBlock*>(block)) {
            stream << "float derivativePrev_" << i << " = 0.0;  // 미분기 이전 값 (" << block->getName() << ")\n";
            stream << "byte derivativeInit_" << i << " = 0;  // 미분기 초기화 상태 (" << block->getName() << ")\n";
        }
        else if (dynamic_cast<RateLimiterBlock*>(block)) {
            stream << "float rateLimiterPrev_" << i << " = 0.0;  // 변화율 제한기 이전 출력 (" << block->getName() << ")\n";
            stream << "byte rateLimiterInit_" << i << " = 0;  // 변화율 제한기 초기화 상태 (" << block->getName() << ")\n";
        }
    }
    stream << "\n";

    // 입력 변수
    if (!inBlocks.isEmpty()) {
        stream << "// 입력 변수\n";
        for (const auto& inBlock : inBlocks) {
            int blockIndex = blockIndices[inBlock];
            QString varName = inBlock->getProperties()["variable"].toString();
            stream << "float input_" << varName << " = 1.0;  // 입력 변수 (" << inBlock->getName() << ")\n";
        }
        stream << "\n";
    }

    // 출력 변수
    if (!outBlocks.isEmpty()) {
        stream << "// 출력 변수\n";
        for (const auto& outBlock : outBlocks) {
            int blockIndex = blockIndices[outBlock];
            QString varName = outBlock->getProperties()["variable"].toString();
            stream << "float output_" << varName << ";  // 출력 변수 (" << outBlock->getName() << ")\n";
        }
        stream << "\n";
    }

    // 블록 이름 매핑 주석
    stream << "// 블록 인덱스 매핑:\n";
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        stream << "// " << i << ": " << orderedBlocks[i]->getName() << "\n";
    }
    stream << "\n";

    // 연결 정보 주석
    stream << "// 연결 정보:\n";
    for (const auto& conn : connections) {
        stream << "// " << orderedBlocks[conn.first]->getName() << " -> "
               << orderedBlocks[conn.second]->getName() << "\n";
    }
    stream << "\n";
}

void CaplCodeGenerator::generateSignalHandlers(QTextStream& stream) {
    // CAN 메시지 수신 핸들러
    stream << "// CAN 메시지 수신 핸들러\n";
    stream << "on message * {\n";
    stream << "  // CAN 메시지 수신 시 필요한 데이터 처리\n";
    stream << "  // 실제 구현에서는 특정 메시지 ID에 따라 처리\n";
    stream << "}\n\n";

    // 키 입력 핸들러
    stream << "// 키 입력 핸들러\n";
    stream << "on key 's' {\n";
    stream << "  write(\"시뮬레이션 시작\");\n";
    stream << "  startSimulation();\n";
    stream << "}\n\n";

    stream << "on key 'q' {\n";
    stream << "  write(\"시뮬레이션 중지\");\n";
    stream << "  stopSimulation();\n";
    stream << "}\n\n";

    // 타이머 핸들러
    stream << "// 타이머 핸들러 - 시뮬레이션 스텝\n";
    stream << "on timer SimulationTimer {\n";
    stream << "  // 시뮬레이션 한 스텝 실행\n";
    stream << "  simulationStep();\n";
    stream << "}\n\n";
}

void CaplCodeGenerator::generateMainFunctions(QTextStream& stream) {
    // 블록 및 연결 정보 수집
    QMap<Block*, int> blockIndices;
    QList<Block*> orderedBlocks;
    QList<QPair<int, int>> connections;
    QList<InBlock*> inBlocks;
    QList<OutBlock*> outBlocks;

    collectBlocksAndConnections(blockIndices, orderedBlocks, connections, inBlocks, outBlocks);

    // 시작 함수
    stream << "// 시뮬레이션 시작 함수\n";
    stream << "void startSimulation() {\n";
    stream << "  // 시뮬레이션 초기화\n";
    stream << "  currentTime = 0.0;\n";
    stream << "  \n";
    stream << "  // 모든 블록 초기화\n";
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];

        if (dynamic_cast<IntegratorBlock*>(block)) {
            double initialValue = block->getProperties()["initial"].toDouble();
            stream << "  integratorState_" << i << " = " << initialValue << ";\n";
        }
        else if (dynamic_cast<DerivativeBlock*>(block)) {
            stream << "  derivativePrev_" << i << " = 0.0;\n";
            stream << "  derivativeInit_" << i << " = 0;\n";
        }
        else if (dynamic_cast<RateLimiterBlock*>(block)) {
            stream << "  rateLimiterPrev_" << i << " = 0.0;\n";
            stream << "  rateLimiterInit_" << i << " = 0;\n";
        }
    }
    stream << "  \n";
    stream << "  // 시뮬레이션 타이머 시작\n";
    stream << "  setTimer(SimulationTimer, TIME_STEP * 1000);  // ms 단위로 변환\n";
    stream << "  \n";
    stream << "  write(\"시뮬레이션 시작됨. 지속 시간: %f초, 시간 간격: %f초\", SIMULATION_DURATION, TIME_STEP);\n";
    stream << "}\n\n";

    // 중지 함수
    stream << "// 시뮬레이션 중지 함수\n";
    stream << "void stopSimulation() {\n";
    stream << "  // 시뮬레이션 타이머 중지\n";
    stream << "  cancelTimer(SimulationTimer);\n";
    stream << "  \n";
    stream << "  write(\"시뮬레이션 중지됨. 현재 시간: %f초\", currentTime);\n";
    stream << "}\n\n";

    // 시뮬레이션 스텝 함수
    stream << "// 시뮬레이션 스텝 함수\n";
    stream << "void simulationStep() {\n";
    stream << "  // 시뮬레이션 종료 확인\n";
    stream << "  if (currentTime >= SIMULATION_DURATION) {\n";
    stream << "    write(\"시뮬레이션 완료!\");\n";
    stream << "    stopSimulation();\n";
    stream << "    return;\n";
    stream << "  }\n";
    stream << "  \n";
    stream << "  // 블록 계산 상태 초기화\n";
    stream << "  int i;\n";
    stream << "  for (i = 0; i < " << orderedBlocks.size() << "; i++) {\n";
    stream << "    blocksCalculated[i] = 0;\n";
    stream << "  }\n";
    stream << "  \n";
    stream << "  // 모든 블록이 계산될 때까지 반복\n";
    stream << "  byte progress = 1;\n";
    stream << "  int iteration = 0;\n";
    stream << "  int maxIterations = " << orderedBlocks.size() * 2 << ";\n";
    stream << "  \n";
    stream << "  while (progress && iteration < maxIterations) {\n";
    stream << "    progress = 0;\n";
    stream << "    iteration++;\n";
    stream << "    \n";
    stream << "    // 각 블록에 대해 계산 시도\n";
    stream << "    for (i = 0; i < " << orderedBlocks.size() << "; i++) {\n";
    stream << "      // 이미 계산된 블록은 건너뜀\n";
    stream << "      if (blocksCalculated[i]) continue;\n";
    stream << "      \n";
    stream << "      // 블록 계산 시도\n";
    stream << "      if (tryCalculateBlock(i)) {\n";
    stream << "        progress = 1;\n";
    stream << "      }\n";
    stream << "    }\n";
    stream << "  }\n";
    stream << "  \n";
    stream << "  // 출력 변수 업데이트\n";
    for (const auto& outBlock : outBlocks) {
        int blockIndex = blockIndices[outBlock];
        QString varName = outBlock->getProperties()["variable"].toString();
        stream << "  output_" << varName << " = blockOutputs[" << blockIndex << "];\n";
        stream << "  write(\"t=%f, " << varName << "=%f\", currentTime, output_" << varName << ");\n";
    }
    stream << "  \n";
    stream << "  // 시간 업데이트\n";
    stream << "  currentTime += TIME_STEP;\n";
    stream << "  \n";
    stream << "  // 다음 스텝 예약\n";
    stream << "  setTimer(SimulationTimer, TIME_STEP * 1000);\n";
    stream << "}\n\n";
}

void CaplCodeGenerator::generateBlockFunctions(QTextStream& stream) {
    // 블록 및 연결 정보 수집
    QMap<Block*, int> blockIndices;
    QList<Block*> orderedBlocks;
    QList<QPair<int, int>> connections;
    QList<InBlock*> inBlocks;
    QList<OutBlock*> outBlocks;

    collectBlocksAndConnections(blockIndices, orderedBlocks, connections, inBlocks, outBlocks);

    // 블록 계산 시도 함수
    stream << "// 블록 계산 시도 함수\n";
    stream << "byte tryCalculateBlock(int blockIndex) {\n";
    stream << "  // 블록 입력 수집 및 계산 시도\n";
    stream << "  float inputs[10];  // 최대 10개 입력 지원\n";
    stream << "  int inputCount = 0;\n";
    stream << "  byte allInputsReady = 1;\n";
    stream << "  int i;\n";
    stream << "  \n";

    // 모든 연결 확인
    stream << "  // 블록 입력 연결 확인\n";
    stream << "  for (i = 0; i < " << connections.size() << "; i++) {\n";
    stream << "    int srcIndex, destIndex;\n";

    // 각 연결에 대한 조건 체크
    for (int i = 0; i < connections.size(); ++i) {
        if (i == 0) {
            stream << "    if (i == 0) {\n";
        } else {
            stream << "    else if (i == " << i << ") {\n";
        }
        stream << "      srcIndex = " << connections[i].first << ";\n";
        stream << "      destIndex = " << connections[i].second << ";\n";
        stream << "    }\n";
    }

    stream << "    \n";
    stream << "    // 현재 블록을 대상으로 하는 연결인지 확인\n";
    stream << "    if (destIndex == blockIndex) {\n";
    stream << "      // 소스 블록이 계산되지 않았으면 건너뜀\n";
    stream << "      if (!blocksCalculated[srcIndex]) {\n";
    stream << "        allInputsReady = 0;\n";
    stream << "        break;\n";
    stream << "      }\n";
    stream << "      \n";
    stream << "      // 입력 배열에 소스 블록의 출력 추가\n";
    stream << "      inputs[inputCount] = blockOutputs[srcIndex];\n";
    stream << "      inputCount++;\n";
    stream << "    }\n";
    stream << "  }\n";
    stream << "  \n";

    stream << "  // 모든 입력이 준비되지 않았으면 건너뜀\n";
    stream << "  if (!allInputsReady) {\n";
    stream << "    return 0;\n";
    stream << "  }\n";
    stream << "  \n";

    // 각 블록 타입에 따른 계산 함수 호출
    stream << "  // 블록 타입에 따른 계산\n";
    /*for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];

        if (i == 0) {
            stream << "  if (blockIndex == 0) {\n";
        } else {
            stream << "  else if (blockIndex == " << i << ") {\n";
        }

        if (dynamic_cast<SourceBlock*>(block)) {
            double value = block->getProperties()["value"].toDouble();
            stream << "    // SourceBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = " << value << ";\n";
        }
        else if (dynamic_cast<GainBlock*>(block)) {
            double gain = block->getProperties()["gain"].toDouble();
            stream << "    // GainBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = inputs[0] * " << gain << ";\n";
        }
        else if (dynamic_cast<SumBlock*>(block)) {
            stream << "    // SumBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = 0.0;\n";
            stream << "    for (i = 0; i < inputCount; i++) {\n";
            stream << "      blockOutputs[blockIndex] += inputs[i];\n";
            stream << "    }\n";
        }
        else if (dynamic_cast<ProductBlock*>(block)) {
            stream << "    // ProductBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = 1.0;\n";
            stream << "    for (i = 0; i < inputCount; i++) {\n";
            stream << "      blockOutputs[blockIndex] *= inputs[i];\n";
            stream << "    }\n";
        }
        else if (dynamic_cast<IntegratorBlock*>(block)) {
            stream << "    // IntegratorBlock: " << block->getName() << "\n";
            stream << "    integratorState_" << i << " += inputs[0] * TIME_STEP;\n";
            stream << "    blockOutputs[blockIndex] = integratorState_" << i << ";\n";
        }
        else if (dynamic_cast<DerivativeBlock*>(block)) {
            stream << "    // DerivativeBlock: " << block->getName() << "\n";
            stream << "    if (!derivativeInit_" << i << ") {\n";
            stream << "      derivativePrev_" << i << " = inputs[0];\n";
            stream << "      derivativeInit_" << i << " = 1;\n";
            stream << "      blockOutputs[blockIndex] = 0.0;\n";
            stream << "    } else {\n";
            stream << "      blockOutputs[blockIndex] = (inputs[0] - derivativePrev_" << i << ") / TIME_STEP;\n";
            stream << "      derivativePrev_" << i << " = inputs[0];\n";
            stream << "    }\n";
        }
        else if (dynamic_cast<InBlock*>(block)) {
            InBlock* inBlock = dynamic_cast<InBlock*>(block);
            QString varName = inBlock->getProperties()["variable"].toString();
            stream << "    // InBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = input_" << varName << ";\n";
        }
        else if (dynamic_cast<OutBlock*>(block)) {
            stream << "    // OutBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = inputs[0];\n";
        }
        // 신호 생성기 블록
        else if (dynamic_cast<ClockBlock*>(block)) {
            double period = block->getProperties()["period"].toDouble();
            double offset = block->getProperties()["offset"].toDouble();
            stream << "    // ClockBlock: " << block->getName() << "\n";
            stream << "    {\n";
            stream << "      float adjustedTime = currentTime - " << offset << ";\n";
            stream << "      if (adjustedTime < 0) {\n";
            stream << "        blockOutputs[blockIndex] = 0.0;\n";
            stream << "      } else {\n";
            stream << "        blockOutputs[blockIndex] = (adjustedTime % " << period << ") / " << period << ";\n";
            stream << "      }\n";
            stream << "    }\n";
        }
        else if (dynamic_cast<RampBlock*>(block)) {
            double slope = block->getProperties()["slope"].toDouble();
            double startTime = block->getProperties()["startTime"].toDouble();
            double initialOutput = block->getProperties()["initialOutput"].toDouble();
            stream << "    // RampBlock: " << block->getName() << "\n";
            stream << "    if (currentTime < " << startTime << ") {\n";
            stream << "      blockOutputs[blockIndex] = " << initialOutput << ";\n";
            stream << "    } else {\n";
            stream << "      blockOutputs[blockIndex] = " << initialOutput << " + " << slope << " * (currentTime - " << startTime << ");\n";
            stream << "    }\n";
        }
        else if (dynamic_cast<StepBlock*>(block)) {
            double stepTime = block->getProperties()["stepTime"].toDouble();
            double initialValue = block->getProperties()["initialValue"].toDouble();
            double finalValue = block->getProperties()["finalValue"].toDouble();
            stream << "    // StepBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = (currentTime < " << stepTime << ") ? "
                   << initialValue << " : " << finalValue << ";\n";
        }
        else if (dynamic_cast<SineWaveBlock*>(block)) {
            double amplitude = block->getProperties()["amplitude"].toDouble();
            double frequency = block->getProperties()["frequency"].toDouble();
            double phase = block->getProperties()["phase"].toDouble();
            double bias = block->getProperties()["bias"].toDouble();
            stream << "    // SineWaveBlock: " << block->getName() << "\n";
            stream << "    blockOutputs[blockIndex] = " << amplitude << " * sin(2 * PI * " << frequency
                   << " * currentTime + " << phase << ") + " << bias << ";\n";
        }
        // 제한 블록
        else if (dynamic_cast<SaturationBlock*>(block)) {
            double upperLimit = block->getProperties()["upperLimit"].toDouble();
            double lowerLimit = block->getProperties()["lowerLimit"].toDouble();
            stream << "    // SaturationBlock: " << block->getName() << "\n";
            stream << "    if (inputs[0] > " << upperLimit << ") {\n";
            stream << "      blockOutputs[blockIndex] = " << upperLimit << ";\n";
            stream << "    } else if (inputs[0] < " << lowerLimit << ") {\n";
            stream << "      blockOutputs[blockIndex] = " << lowerLimit << ";\n";
            stream << "    } else {\n";
            stream << "      blockOutputs[blockIndex] = inputs[0];\n";
            stream << "    }\n";
        }
        else if (dynamic_cast<RateLimiterBlock*>(block)) {
            double risingLimit = block->getProperties()["risingLimit"].toDouble();
            double fallingLimit = block->getProperties()["fallingLimit"].toDouble();
            stream << "    // RateLimiterBlock: " << block->getName() << "\n";
            stream << "    if (!rateLimiterInit_" << i << ") {\n";
            stream << "      rateLimiterPrev_" << i << " = inputs[0];\n";
            stream << "      rateLimiterInit_" << i << " = 1;\n";
            stream << "      blockOutputs[blockIndex] = inputs[0];\n";
            stream << "    } else {\n";
            stream << "      float rate = (inputs[0] - rateLimiterPrev_" << i << ") / TIME_STEP;\n";
            stream << "      if (rate > " << risingLimit << ") {\n";
            stream << "        rate = " << risingLimit << ";\n";
            stream << "      } else if (rate < " << fallingLimit << ") {\n";
            stream << "        rate = " << fallingLimit << ";\n";
            stream << "      }\n";
            stream << "      blockOutputs[blockIndex] = rateLimiterPrev_" << i << " + rate * TIME_STEP;\n";
            stream << "      rateLimiterPrev_" << i << " = blockOutputs[blockIndex];\n";
            stream << "    }\n";
        }
        // 기타 타입
        else {
            stream << "    // 미구현 블록: " << block->getName() << " (타입 " << block->getType() << ")\n";
            stream << "    blockOutputs[blockIndex] = 0.0;\n";
        }

        stream << "  }\n";
    }
    */
    for (int i = 0; i < orderedBlocks.size(); ++i) {
        Block* block = orderedBlocks[i];

        if (i == 0) {
            stream << "  if (blockIndex == 0) {\n";
        } else {
            stream << "  else if (blockIndex == " << i << ") {\n";
        }

        // 블록 팩토리를 통해 CAPL 코드 생성
        QString blockCode = BlockFactory::instance()->generateCode("capl", block);
        if (!blockCode.isEmpty()) {
            stream << "    // " << block->getName() << "\n";
            stream << "    " << blockCode << "\n";
        }
        // 코드 생성이 실패한 경우 기본 구현 사용
        else {
            // 기존 하드코딩된 방식으로 폴백

        }

        stream << "  }\n";
    }
    stream << "  \n";
    stream << "  // 계산 완료로 표시\n";
    stream << "  blocksCalculated[blockIndex] = 1;\n";
    stream << "  return 1;\n";
    stream << "}\n\n";
}

void CaplCodeGenerator::generateUtilityFunctions(QTextStream& stream) {
    // 유틸리티 함수들
    stream << "// 초기화 함수 - CAPL 프로그램 시작 시 실행\n";
    stream << "on start {\n";
    stream << "  write(\"CAPL 프로그램 시작됨\");\n";
    stream << "  write(\"시뮬레이션을 시작하려면 'S' 키를 누르세요\");\n";
    stream << "  write(\"시뮬레이션을 종료하려면 'Q' 키를 누르세요\");\n";
    stream << "}\n\n";

    stream << "// 종료 함수 - CAPL 프로그램 종료 시 실행\n";
    stream << "on end {\n";
    stream << "  write(\"CAPL 프로그램 종료됨\");\n";
    stream << "}\n\n";
}

// 블록과 연결 정보 수집
void CaplCodeGenerator::collectBlocksAndConnections(QMap<Block*, int>& blockIndices,
                                                    QList<Block*>& orderedBlocks,
                                                    QList<QPair<int, int>>& connections,
                                                    QList<InBlock*>& inBlocks,
                                                    QList<OutBlock*>& outBlocks) {
    // 모든 블록 수집 및 인덱스 할당
    for (auto item : m_scene->items()) {
        Block* block = dynamic_cast<Block*>(item);
        if (block) {
            blockIndices[block] = orderedBlocks.size();
            orderedBlocks.append(block);

            // In/Out 블록 식별
            if (InBlock* inBlock = dynamic_cast<InBlock*>(block)) {
                inBlocks.append(inBlock);
            }
            else if (OutBlock* outBlock = dynamic_cast<OutBlock*>(block)) {
                outBlocks.append(outBlock);
            }
        }
    }

    // 연결 수집
    for (auto connection : m_scene->getConnections()) {
        Block* sourceBlock = connection->getSourceBlock();
        Block* destBlock = connection->getDestBlock();

        if (blockIndices.contains(sourceBlock) && blockIndices.contains(destBlock)) {
            int sourceIndex = blockIndices[sourceBlock];
            int destIndex = blockIndices[destBlock];
            connections.append(qMakePair(sourceIndex, destIndex));
        }
    }
}
