#ifndef BLOCKFACTORY_H
#define BLOCKFACTORY_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QIcon>
#include <QVariant>
#include <functional>

#include "block.h"

// 블록 생성자 함수 타입
typedef std::function<Block*(const QString&, QGraphicsItem*)> BlockCreatorFunc;

// 코드 생성 함수 타입 (각 언어별)
typedef std::function<QString(const Block*, const QMap<QString, QVariant>&)> CodeGeneratorFunc;

// 블록 정보를 저장하는 클래스
class BlockInfo {
public:
    BlockInfo();
    BlockInfo(Block::BlockType type,
              const QString& name,
              const QString& category,
              const QString& description,
              const BlockCreatorFunc&,
              const QIcon& icon = QIcon());

    Block::BlockType type() const;
    QString name() const;
    QString category() const;
    QString description() const;
    QIcon icon() const;

    // 블록 인스턴스 생성
    Block* createInstance(const QString& instanceName = QString(), QGraphicsItem* parent = nullptr) const;

    // 코드 생성 함수 등록 (각 언어별)
    void registerCodeGenerator(const QString& language, CodeGeneratorFunc func);

    // 코드 생성
    QString generateCode(const QString& language, const Block* block) const;

private:
    Block::BlockType m_type;
    QString m_name;
    QString m_category;
    QString m_description;
    BlockCreatorFunc m_creator;
    QIcon m_icon;
    QMap<QString, CodeGeneratorFunc> m_codeGenerators;
};

// 블록 팩토리 - 싱글톤 패턴
class BlockFactory : public QObject {
    Q_OBJECT

public:
    // 싱글톤 인스턴스 얻기
    static BlockFactory* instance();

    // 블록 정보 등록
    void registerBlockType(const BlockInfo& blockInfo);

    // 등록된 모든 블록 타입 목록 가져오기
    QList<BlockInfo> getAllBlockTypes() const;

    // 카테고리별 블록 타입 목록 가져오기
    QList<BlockInfo> getBlockTypesByCategory(const QString& category) const;

    // 특정 타입의 블록 정보 가져오기
    BlockInfo getBlockInfo(Block::BlockType type) const;
    BlockInfo getBlockInfoByName(const QString& name) const;

    // 블록 인스턴스 생성
    Block* createBlock(Block::BlockType type, const QString& name = QString(), QGraphicsItem* parent = nullptr) const;
    Block* createBlockByName(const QString& typeName, const QString& name = QString(), QGraphicsItem* parent = nullptr) const;

    // 모든 카테고리 목록 얻기
    QStringList getAllCategories() const;

    // 타입에 따른 코드 생성
    QString generateCode(const QString& language, const Block* block) const;

private:
    BlockFactory(QObject* parent = nullptr);

    // 모든 블록 타입 정보 등록
    void registerDefaultBlockTypes();

    QMap<Block::BlockType, BlockInfo> m_blockInfoMap;
    QMap<QString, Block::BlockType> m_nameToTypeMap;

    static BlockFactory* s_instance;
};

#endif // BLOCKFACTORY_H
