/**

 Copyright (c) 2010-2014  hkrn

 All rights reserved.

 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:

 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
 - Neither the name of the MMDAI project team nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef MODELPROXY_H
#define MODELPROXY_H

#include <QHash>
#include <QJsonValue>
#include <QObject>
#include <QQmlListProperty>
#include <QQuaternion>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUuid>
#include <QVector3D>

#include <vpvl2/IString.h>
#include "ProjectProxy.h"

class QAbstractItemModel;
class QStringListModel;
class BoneRefObject;
class JointRefObject;
class LabelRefObject;
class MaterialRefObject;
class MorphRefObject;
class RigidBodyRefObject;
class VertexRefObject;

namespace vpvl2 {
class IBone;
class ILabel;
class IModel;
class IMorph;
}

class ModelProxy : public QObject
{
    Q_OBJECT
    Q_ENUMS(AxisType)
    Q_ENUMS(EncodingType)
    Q_ENUMS(ObjectType)
    Q_ENUMS(TransformType)
    Q_ENUMS(VersionType)
    Q_PROPERTY(ProjectProxy *parentProject READ parentProject CONSTANT FINAL)
    Q_PROPERTY(ModelProxy *parentBindingModel READ parentBindingModel WRITE setParentBindingModel NOTIFY parentBindingModelChanged)
    Q_PROPERTY(BoneRefObject *parentBindingBone READ parentBindingBone WRITE setParentBindingBone NOTIFY parentBindingBoneChanged)
    Q_PROPERTY(MotionProxy *childMotion READ childMotion NOTIFY childMotionChanged)
    Q_PROPERTY(QUuid uuid READ uuid CONSTANT FINAL)
    Q_PROPERTY(QUrl fileUrl READ fileUrl CONSTANT FINAL)
    Q_PROPERTY(QUrl faviconUrl READ faviconUrl CONSTANT FINAL)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString comment READ comment WRITE setComment NOTIFY commentChanged FINAL)
    Q_PROPERTY(VersionType version READ version WRITE setVersion NOTIFY versionChanged FINAL)
    Q_PROPERTY(EncodingType encodingType READ encodingType WRITE setEncodingType NOTIFY encodingTypeChanged FINAL)
    Q_PROPERTY(QQmlListProperty<LabelRefObject> allLabels READ allLabels NOTIFY allLabelsChanged FINAL)
    Q_PROPERTY(QQmlListProperty<BoneRefObject> allBones READ allBones NOTIFY allBonesChanged FINAL)
    Q_PROPERTY(QQmlListProperty<MorphRefObject> allMorphs READ allMorphs NOTIFY allMorphsChanged FINAL)
    Q_PROPERTY(QQmlListProperty<MaterialRefObject> allMaterials READ allMaterials NOTIFY allMaterialsChanged FINAL)
    Q_PROPERTY(QQmlListProperty<VertexRefObject> allVertices READ allVertices NOTIFY allVerticesChanged FINAL)
    Q_PROPERTY(QQmlListProperty<RigidBodyRefObject> allRigidBodies READ allRigidBodies NOTIFY allRigidBodiesChanged FINAL)
    Q_PROPERTY(QQmlListProperty<JointRefObject> allJoints READ allJoints NOTIFY allJointsChanged FINAL)
    Q_PROPERTY(QQmlListProperty<BoneRefObject> targetBones READ targetBones NOTIFY targetBonesChanged FINAL)
    Q_PROPERTY(BoneRefObject *firstTargetBone READ firstTargetBone NOTIFY firstTargetBoneChanged FINAL)
    Q_PROPERTY(MorphRefObject *firstTargetMorph READ firstTargetMorph WRITE setFirstTargetMorph NOTIFY firstTargetMorphChanged FINAL)
    Q_PROPERTY(AxisType axisType READ axisType WRITE setAxisType NOTIFY axisTypeChanged FINAL)
    Q_PROPERTY(TransformType transformType READ transformType WRITE setTransformType NOTIFY transformTypeChanged FINAL)
    Q_PROPERTY(QVector3D translation READ translation WRITE setTranslation NOTIFY translationChanged FINAL)
    Q_PROPERTY(QQuaternion orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(QVector3D eulerOrientation READ eulerOrientation WRITE setEulerOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(ProjectProxy::LanguageType language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(qreal scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged FINAL)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged FINAL)
    Q_PROPERTY(qreal edgeWidth READ edgeWidth WRITE setEdgeWidth NOTIFY edgeWidthChanged FINAL)
    Q_PROPERTY(int maxUVCount READ maxUVCount WRITE setMaxUVCount NOTIFY numUVAChanged FINAL)
    Q_PROPERTY(int orderIndex READ orderIndex WRITE setOrderIndex NOTIFY orderIndexChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool moving READ isMoving NOTIFY movingChanged FINAL)
    Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged FINAL)

public:
    enum AxisType {
        AxisX,
        AxisY,
        AxisZ
    };
    enum EncodingType {
        ShiftJIS = vpvl2::IString::kShiftJIS,
        UTF8     = vpvl2::IString::kUTF8,
        UTF16    = vpvl2::IString::kUTF16
    };
    enum ObjectType {
        Vertex,
        Material,
        Bone,
        Morph,
        Label,
        RigidBody,
        Joint,
        SoftBody
    };
    enum TransformType {
        LocalTransform,
        GlobalTransform,
        ViewTransform
    };
    enum VersionType {
        PMD_1_0,
        PMX_2_0,
        PMX_2_1
    };

    ModelProxy(ProjectProxy *project,
               vpvl2::IModel *model,
               const QUuid &uuid,
               const QUrl &fileUrl,
               const QUrl &faviconUrl,
               QUndoStack *undoStackRef);
    ~ModelProxy();

    Q_INVOKABLE bool saveJson(const QUrl &fileUrl) const;

    void initialize(bool all);
    void addBindingModel(ModelProxy *value);
    void removeBindingModel(ModelProxy *value);
    void releaseBindings();
    QJsonValue toJson() const;

    vpvl2::IModel *data() const;
    ProjectProxy *parentProject() const;
    QUndoStack *undoStack() const;
    ModelProxy *parentBindingModel() const;
    void setParentBindingModel(ModelProxy *value);
    BoneRefObject *parentBindingBone() const;
    void setParentBindingBone(BoneRefObject *value);
    MotionProxy *childMotion() const;
    void setChildMotion(MotionProxy *value, bool emitSignal);
    QUuid uuid() const;
    QUrl fileUrl() const;
    QUrl faviconUrl() const;
    QQmlListProperty<LabelRefObject> allLabels();
    QQmlListProperty<BoneRefObject> allBones();
    QQmlListProperty<MorphRefObject> allMorphs();
    QQmlListProperty<MaterialRefObject> allMaterials();
    QQmlListProperty<VertexRefObject> allVertices();
    QQmlListProperty<RigidBodyRefObject> allRigidBodies();
    QQmlListProperty<JointRefObject> allJoints();
    QQmlListProperty<BoneRefObject> targetBones();
    VersionType version() const;
    void setVersion(const VersionType &value);
    QString name() const;
    void setName(const QString &value);
    QString comment() const;
    void setComment(const QString &value);
    EncodingType encodingType() const;
    void setEncodingType(EncodingType value);
    QList<BoneRefObject *> allTargetBones() const;
    BoneRefObject *firstTargetBone() const;
    MorphRefObject *firstTargetMorph() const;
    void setFirstTargetMorph(MorphRefObject *value);
    AxisType axisType() const;
    void setAxisType(AxisType value);
    TransformType transformType() const;
    void setTransformType(TransformType value);
    ProjectProxy::LanguageType language() const;
    void setLanguage(ProjectProxy::LanguageType value);
    QVector3D translation() const;
    void setTranslation(const QVector3D &value);
    QQuaternion orientation() const;
    void setOrientation(const QQuaternion &value);
    QVector3D eulerOrientation() const;
    void setEulerOrientation(const QVector3D &value);
    qreal scaleFactor() const;
    void setScaleFactor(qreal value);
    qreal edgeWidth() const;
    void setEdgeWidth(qreal value);
    qreal opacity() const;
    void setOpacity(qreal value);
    QList<vpvl2::ILabel *> allLabels() const;
    int maxUVCount() const;
    void setMaxUVCount(int value);
    int orderIndex() const;
    void setOrderIndex(int value);
    bool isVisible() const;
    void setVisible(bool value);
    bool isMoving() const;
    bool isDirty() const;
    void setDirty(bool value);
    void markDirty();

    QList<LabelRefObject *> allLabelRefs() const;
    QList<BoneRefObject *> allBoneRefs() const;
    QList<MorphRefObject *> allMorphRefs() const;
    QList<MaterialRefObject *> allMaterialRefs() const;
    QList<VertexRefObject *> allVertexRefs() const;
    QList<RigidBodyRefObject *> allRigidBodyRefs() const;
    QList<JointRefObject *> allJointRefs() const;

signals:
    void parentBindingModelChanged();
    void parentBindingBoneChanged();
    void childMotionChanged();
    void versionChanged();
    void nameChanged();
    void commentChanged();
    void encodingTypeChanged();
    void allLabelsChanged();
    void allBonesChanged();
    void allMorphsChanged();
    void allMaterialsChanged();
    void allVerticesChanged();
    void allRigidBodiesChanged();
    void allJointsChanged();
    void targetBonesChanged();
    void firstTargetBoneChanged();
    void firstTargetMorphChanged();
    void axisTypeChanged();
    void transformTypeChanged();
    void languageChanged();
    void translationChanged();
    void orientationChanged();
    void scaleFactorChanged();
    void opacityChanged();
    void edgeWidthChanged();
    void numUVAChanged();
    void orderIndexChanged();
    void visibleChanged();
    void movingChanged();
    void dirtyChanged();
    void transformDidBegin();
    void targetBonesDidTranslate();
    void targetBonesDidRotate();
    void transformDidDiscard();
    void transformDidCommit();
    void boneDidSelect(BoneRefObject *bone);
    void morphDidSelect(MorphRefObject *morph);
    void modelDidRefresh();

public slots:
    Q_INVOKABLE void selectOpaqueObject(QObject *value);
    Q_INVOKABLE void selectBone(BoneRefObject *value);
    Q_INVOKABLE void beginTransform(qreal startY);
    Q_INVOKABLE void translate(qreal value);
    Q_INVOKABLE void rotate(qreal angle);
    Q_INVOKABLE void discardTransform();
    Q_INVOKABLE void commitTransform();
    Q_INVOKABLE void resetTargets();
    Q_INVOKABLE void release();
    Q_INVOKABLE void refresh();

    Q_INVOKABLE BoneRefObject *resolveBoneRef(const vpvl2::IBone *value) const;
    Q_INVOKABLE BoneRefObject *findBoneByName(const QString &name) const;
    Q_INVOKABLE BoneRefObject *findBoneByUuid(const QUuid &uuid) const;
    Q_INVOKABLE MorphRefObject *resolveMorphRef(const vpvl2::IMorph *value) const;
    Q_INVOKABLE MorphRefObject *findMorphByName(const QString &name) const;
    Q_INVOKABLE MorphRefObject *findMorphByUuid(const QUuid &uuid) const;
    Q_INVOKABLE QList<QObject *> findMorphsByCategory(int type) const;
    Q_INVOKABLE MaterialRefObject *resolveMaterialRef(const vpvl2::IMaterial *value) const;
    Q_INVOKABLE MaterialRefObject *findMaterialByName(const QString &name) const;
    Q_INVOKABLE MaterialRefObject *findMaterialByUuid(const QUuid &uuid) const;
    Q_INVOKABLE VertexRefObject *resolveVertexRef(const vpvl2::IVertex *value) const;
    Q_INVOKABLE VertexRefObject *findVertexByUuid(const QUuid &uuid) const;
    Q_INVOKABLE RigidBodyRefObject *resolveRigidBodyRef(const vpvl2::IRigidBody *value) const;
    Q_INVOKABLE RigidBodyRefObject *findRigidBodyByName(const QString &name) const;
    Q_INVOKABLE RigidBodyRefObject *findRigidBodyByUuid(const QUuid &uuid) const;
    Q_INVOKABLE JointRefObject *resolveJointRef(const vpvl2::IJoint *value) const;
    Q_INVOKABLE JointRefObject *findJointByName(const QString &name) const;
    Q_INVOKABLE JointRefObject *findJointByUuid(const QUuid &uuid) const;

    Q_INVOKABLE VertexRefObject *createVertex();
    Q_INVOKABLE MaterialRefObject *createMaterial();
    Q_INVOKABLE BoneRefObject *createBone();
    Q_INVOKABLE MorphRefObject *createMorph();
    Q_INVOKABLE LabelRefObject *createLabel();
    Q_INVOKABLE RigidBodyRefObject *createRigidBody();
    Q_INVOKABLE JointRefObject *createJoint();
    Q_INVOKABLE QObject *createObject(ObjectType type);
    Q_INVOKABLE bool removeVertex(VertexRefObject *value);
    Q_INVOKABLE bool removeMaterial(MaterialRefObject *value);
    Q_INVOKABLE bool removeBone(BoneRefObject *value);
    Q_INVOKABLE bool removeMorph(MorphRefObject *value);
    Q_INVOKABLE bool removeLabel(LabelRefObject *value);
    Q_INVOKABLE bool removeRigidBody(RigidBodyRefObject *value);
    Q_INVOKABLE bool removeJoint(JointRefObject *value);
    Q_INVOKABLE bool removeObject(QObject *value);
    Q_INVOKABLE bool deleteObject(QObject *value);

private slots:
    void resetLanguage();

private:
    void initializeBones(const vpvl2::Array<vpvl2::ILabel *> &labelRefs);
    void initializeMorphs(const vpvl2::Array<vpvl2::ILabel *> &labelRefs, const vpvl2::IModel *model);
    void initializeAllVertices();
    void initializeAllMaterials();
    void initializeAllBones();
    void initializeAllMorphs();
    void initializeAllRigidBodies();
    void initializeAllJoints();
    void saveTransformState();
    void clearTransformState();

    ProjectProxy *m_parentProjectRef;
    MotionProxy *m_childMotionRef;
    QScopedPointer<vpvl2::IModel> m_model;
    typedef QPair<vpvl2::Vector3, vpvl2::Quaternion> InternalTransform;
    QHash<BoneRefObject *, InternalTransform> m_transformState;
    const QUuid m_uuid;
    const QUrl m_fileUrl;
    const QUrl m_faviconUrl;
    QHash<const vpvl2::IBone *, BoneRefObject *> m_bone2Refs;
    QHash<const vpvl2::IMorph *, MorphRefObject *> m_morph2Refs;
    QHash<const vpvl2::IMaterial *, MaterialRefObject *> m_material2Refs;
    QHash<const vpvl2::IVertex *, VertexRefObject *> m_vertex2Refs;
    QHash<const vpvl2::IRigidBody *, RigidBodyRefObject *> m_rigidBody2Refs;
    QHash<const vpvl2::IJoint *, JointRefObject *> m_joint2Refs;
    QHash<const QString, BoneRefObject *> m_name2BoneRefs;
    QHash<const QUuid, BoneRefObject *> m_uuid2BoneRefs;
    QHash<const QString, MorphRefObject *> m_name2MorphRefs;
    QHash<const QUuid, MorphRefObject *> m_uuid2MorphRefs;
    QHash<const QString, MaterialRefObject *> m_name2MaterialRefs;
    QHash<const QUuid, MaterialRefObject *> m_uuid2MaterialRefs;
    QHash<const QUuid, VertexRefObject *> m_uuid2VertexRefs;
    QHash<const QString, RigidBodyRefObject *> m_name2RigidBodyRefs;
    QHash<const QUuid, RigidBodyRefObject *> m_uuid2RigidBodyRefs;
    QHash<const QString, JointRefObject *> m_name2JointRefs;
    QHash<const QUuid, JointRefObject *> m_uuid2JointRefs;
    QList<LabelRefObject *> m_allLabels;
    QList<BoneRefObject *> m_allBones;
    QList<BoneRefObject *> m_targetBoneRefs;
    QList<MorphRefObject *> m_allMorphs;
    QList<MaterialRefObject *> m_allMaterials;
    QList<VertexRefObject *> m_allVertices;
    QList<RigidBodyRefObject *> m_allRigidBodies;
    QList<JointRefObject *> m_allJoints;
    QList<ModelProxy *> m_bindingModels;
    QUndoStack *m_undoStackRef;
    MorphRefObject * m_targetMorphRef;
    AxisType m_boneAxisType;
    TransformType m_boneTransformType;
    ProjectProxy::LanguageType m_language;
    qreal m_baseY;
    bool m_moving;
    bool m_dirty;
};

#endif // MODELPROXY_H
