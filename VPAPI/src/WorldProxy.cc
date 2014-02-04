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

#include "WorldProxy.h"

#include <QtCore>
#include "BoneRefObject.h"
#include "ModelProxy.h"
#include "ProjectProxy.h"
#include "Util.h"

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

#include <vpvl2/vpvl2.h>
#include <vpvl2/extensions/World.h>

using namespace vpvl2;
using namespace vpvl2::extensions;

namespace {

class SynchronizedBoneMotionState : public btMotionState {
public:
    SynchronizedBoneMotionState(const IBone *boneRef)
        : m_boneRef(boneRef)
    {
    }
    ~SynchronizedBoneMotionState() {
        m_boneRef = 0;
    }

    void getWorldTransform(btTransform &worldTrans) const {
        worldTrans = m_boneRef->worldTransform();
    }
    void setWorldTransform(const btTransform & /* worldTrans */) {
    }

private:
    const IBone *m_boneRef;
};

}

WorldProxy::WorldProxy(ProjectProxy *parent)
    : QObject(parent),
      m_sceneWorld(new World()),
      m_modelWorld(new World()),
      m_parentProjectProxyRef(parent),
      m_simulationType(DisableSimulation),
      m_lastGravity(gravity()),
      m_lastTimeIndex(0.0),
      m_enableDebug(false),
      m_playing(false)
{
}

WorldProxy::~WorldProxy()
{
    joinWorld(0);
    m_parentProjectProxyRef = 0;
}

BoneRefObject *WorldProxy::ray(const Vector3 &from, const Vector3 &to)
{
    Q_ASSERT(m_modelWorld);
    btCollisionWorld::ClosestRayResultCallback callback(from, to);
    btDiscreteDynamicsWorld *worldRef = m_modelWorld->dynamicWorldRef();
    worldRef->stepSimulation(1);
    worldRef->rayTest(from, to, callback);
    if (callback.hasHit()) {
        BoneRefObject *value = static_cast<BoneRefObject *>(callback.m_collisionObject->getUserPointer());
        return value;
    }
    return 0;
}

void WorldProxy::joinWorld(ModelProxy *value)
{
    Q_ASSERT(m_modelWorld);
    btDiscreteDynamicsWorld *world = m_modelWorld->dynamicWorldRef();
    const int numCollidables = world->getNumCollisionObjects();
    for (int i = numCollidables - 1; i >= 0; i--) {
        btCollisionObject *object = world->getCollisionObjectArray().at(i);
        if (btRigidBody *body = btRigidBody::upcast(object)) {
            world->removeRigidBody(body);
            delete body->getMotionState();
        }
        else {
            world->removeCollisionObject(object);
        }
        delete object->getCollisionShape();
        delete object;
    }
    if (value) {
        foreach (BoneRefObject *bone, value->allBoneRefs()) {
            const IBone *boneRef = bone->data();
            if (boneRef->isInteractive()) {
                QScopedPointer<btSphereShape> shape(new btSphereShape(0.5));
                QScopedPointer<btMotionState> state(new SynchronizedBoneMotionState(boneRef));
                btRigidBody::btRigidBodyConstructionInfo info(0, state.take(), shape.take(), kZeroV3);
                QScopedPointer<btRigidBody> body(new btRigidBody(info));
                //body->setActivationState(DISABLE_DEACTIVATION);
                body->setCollisionFlags(body->getCollisionFlags() | btRigidBody::CF_KINEMATIC_OBJECT);
                body->setUserPointer(bone);
                world->addRigidBody(body.take());
            }
        }
    }
}

void WorldProxy::leaveWorld(ModelProxy *value)
{
    Q_ASSERT(m_sceneWorld);
    Q_ASSERT(value);
    IModel *modelRef = value->data();
    modelRef->leaveWorld(m_sceneWorld->dynamicWorldRef());
}

void WorldProxy::resetProjectInstance(ProjectProxy *value)
{
    Q_ASSERT(m_sceneWorld);
    Q_ASSERT(value);
    value->projectInstanceRef()->setWorldRef(m_sceneWorld->dynamicWorldRef());
}

void WorldProxy::stepSimulation(qreal timeIndex)
{
    Q_ASSERT(m_sceneWorld);
    Q_ASSERT(timeIndex >= 0);
    SimulationType type = simulationType();
    if (type == EnableSimulationAnytime || (type == EnableSimulationPlayOnly && m_playing)) {
        int delta = qRound(timeIndex - m_lastTimeIndex);
        if (delta > 0) {
            m_sceneWorld->stepSimulation(delta, Scene::defaultFPS());
        }
        m_lastTimeIndex = timeIndex;
    }
}

void WorldProxy::rewind()
{
    Q_ASSERT(m_sceneWorld);
    stepSimulation(0);
    XMLProject *project = m_parentProjectProxyRef->projectInstanceRef();
    Q_ASSERT(project);
    project->setWorldRef(0);
    if (simulationType() != DisableSimulation) {
        project->setWorldRef(m_sceneWorld->dynamicWorldRef());
    }
}

void WorldProxy::setDebugDrawer(btIDebugDraw *value)
{
    Q_ASSERT(m_sceneWorld);
    m_sceneWorld->dynamicWorldRef()->setDebugDrawer(value);
}

void WorldProxy::debugDraw()
{
    Q_ASSERT(m_sceneWorld);
    m_sceneWorld->dynamicWorldRef()->debugDrawWorld();
}

void WorldProxy::setPlaying(bool value)
{
    Q_ASSERT(m_parentProjectProxyRef);
    if (simulationType() == EnableSimulationPlayOnly) {
        applyAllModels(value);
    }
    m_playing = value;
}

WorldProxy::SimulationType WorldProxy::simulationType() const
{
    return m_simulationType;
}

void WorldProxy::setSimulationType(SimulationType value)
{
    Q_ASSERT(m_sceneWorld);
    if (value != simulationType()) {
        XMLProject *project = m_parentProjectProxyRef->projectInstanceRef();
        Q_ASSERT(project);
        bool enabled = (value == EnableSimulationAnytime);
        project->setWorldRef(0);
        if (enabled) {
            project->setWorldRef(m_sceneWorld->dynamicWorldRef());
            setGravity(m_lastGravity);
        }
        else {
            m_lastGravity = gravity();
        }
        applyAllModels(enabled);
        m_simulationType = value;
        simulationTypeChanged();
    }
}

QVector3D WorldProxy::gravity() const
{
    Q_ASSERT(m_sceneWorld);
    return Util::fromVector3(m_sceneWorld->gravity());
}

void WorldProxy::setGravity(const QVector3D &value)
{
    Q_ASSERT(m_sceneWorld);
    if (!qFuzzyCompare(gravity(), value)) {
        m_sceneWorld->setGravity(Util::toVector3(value));
        emit gravityChanged();
    }
}

qreal WorldProxy::baseFPS() const
{
    return m_sceneWorld->baseFPS();
}

void WorldProxy::setBaseFPS(const qreal &value)
{
    if (!qFuzzyCompare(baseFPS(), value)) {
        m_sceneWorld->setBaseFPS(value);
        emit baseFPSChanged();
    }
}

qreal WorldProxy::timeScale() const
{
    return m_sceneWorld->timeScale();
}

void WorldProxy::setTimeScale(const qreal &value)
{
    if (!qFuzzyCompare(timeScale(), value)) {
        m_sceneWorld->setTimeScale(value);
        emit timeScaleChanged();
    }
}

int WorldProxy::randSeed() const
{
    Q_ASSERT(m_sceneWorld);
    return m_sceneWorld->randSeed();
}

void WorldProxy::setRandSeed(int value)
{
    Q_ASSERT(m_sceneWorld);
    if (value != randSeed()) {
        m_sceneWorld->setRandSeed(value);
        emit randSeedChanged();
    }
}

bool WorldProxy::isDebugEnabled() const
{
    return m_enableDebug;
}

void WorldProxy::setDebugEnabled(bool value)
{
    if (value != m_enableDebug) {
        if (!value) {
            setDebugDrawer(0);
        }
        m_enableDebug = value;
    }
}

bool WorldProxy::isFloorEnabled() const
{
    Q_ASSERT(m_sceneWorld);
    return m_sceneWorld->isFloorEnabled();
}

void WorldProxy::setFloorEnabled(bool value)
{
    Q_ASSERT(m_sceneWorld);
    if (isFloorEnabled() != value) {
        m_sceneWorld->setFloorEnabled(value);
        emit enableFloorChanged();
    }
}

void WorldProxy::applyAllModels(bool value)
{
    foreach (ModelProxy *modelProxy, m_parentProjectProxyRef->modelProxies()) {
        modelProxy->data()->setPhysicsEnable(value);
    }
}
