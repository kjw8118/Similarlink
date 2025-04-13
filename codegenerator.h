#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <QObject>

#include <sstream>

#include "simulation.h"
#include "block.h"

#include "blockfactory.h"

class CppCodeGenerator : public QObject {
public:
    CppCodeGenerator(SimulationScene* scene, QObject* parent = nullptr);

    // 현재 다이어그램 상태를 C++ 코드로 생성
    QString generateCode();
    QString generateFunction();
    QString generateClass();
private:
    void generateBlockClass(QTextStream& stream);

    void generateSimulationClass(QTextStream& stream);

    void generateSimulationInstance(QTextStream& stream);

    // 블록과 연결 정보 수집
    void collectBlocksAndConnections(QMap<Block*, int>& blockIndices,
                                     QList<Block*>& orderedBlocks,
                                     QList<QPair<int, int>>& connections,
                                     QList<InBlock*>& inBlocks,
                                     QList<OutBlock*>& outBlocks);

    SimulationScene* m_scene;


};




class PythonCodeGenerator : public QObject {
    Q_OBJECT
public:
    PythonCodeGenerator(SimulationScene* scene, QObject* parent = nullptr);

    // 현재 다이어그램 상태를 Python 코드로 생성
    QString generateCode();

private:
    void generateBlockClass(QTextStream& stream);

    void generateSimulationClass(QTextStream& stream);

    void generateSimulationInstance(QTextStream& stream);

    // 블록과 연결 정보 수집
    void collectBlocksAndConnections(QMap<Block*, int>& blockIndices,
                                     QList<Block*>& orderedBlocks,
                                     QList<QPair<int, int>>& connections,
                                     QList<InBlock*>& inBlocks,
                                     QList<OutBlock*>& outBlocks);

    SimulationScene* m_scene;
};



class CaplCodeGenerator : public QObject {
    Q_OBJECT
public:
    CaplCodeGenerator(SimulationScene* scene, QObject* parent = nullptr);

    // 현재 다이어그램 상태를 CAPL 코드로 생성
    QString generateCode() ;

private:
    void generateVariables(QTextStream& stream);

    void generateSignalHandlers(QTextStream& stream);

    void generateMainFunctions(QTextStream& stream) ;

    void generateBlockFunctions(QTextStream& stream);

    void generateUtilityFunctions(QTextStream& stream);

    // 블록과 연결 정보 수집
    void collectBlocksAndConnections(QMap<Block*, int>& blockIndices,
                                     QList<Block*>& orderedBlocks,
                                     QList<QPair<int, int>>& connections,
                                     QList<InBlock*>& inBlocks,
                                     QList<OutBlock*>& outBlocks);

    SimulationScene* m_scene;
};

#endif // CODEGENERATOR_H
