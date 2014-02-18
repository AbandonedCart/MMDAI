#include "Common.h"

#include "vpvl2/vpvl2.h"
#include "vpvl2/IKeyframe.h"
#include "vpvl2/extensions/icu4c/Encoding.h"
#include "vpvl2/pmx/Model.h"
#include "vpvl2/mvd/BoneKeyframe.h"
#include "vpvl2/mvd/BoneSection.h"
#include "vpvl2/mvd/CameraKeyframe.h"
#include "vpvl2/mvd/CameraSection.h"
#include "vpvl2/mvd/EffectKeyframe.h"
#include "vpvl2/mvd/EffectSection.h"
#include "vpvl2/mvd/LightKeyframe.h"
#include "vpvl2/mvd/LightSection.h"
#include "vpvl2/mvd/ModelKeyframe.h"
#include "vpvl2/mvd/ModelSection.h"
#include "vpvl2/mvd/MorphKeyframe.h"
#include "vpvl2/mvd/MorphSection.h"
#include "vpvl2/mvd/Motion.h"
#include "vpvl2/mvd/NameListSection.h"
#include "vpvl2/mvd/ProjectKeyframe.h"
#include "vpvl2/mvd/ProjectSection.h"

#include "mock/Bone.h"
#include "mock/Model.h"
#include "mock/Morph.h"

using namespace ::testing;
using namespace vpvl2;
using namespace vpvl2::extensions::icu4c;
using namespace vpvl2::pmx;

namespace
{

class MockModelAdapter {
public:
    MockModelAdapter(MockIModel &model)
        : m_bones(new Array<IBone *>()),
          m_morphs(new Array<IMorph *>())
    {
        EXPECT_CALL(model, findBoneRef(_)).Times(AtLeast(1)).WillRepeatedly(Invoke(this, &MockModelAdapter::addBone));
        EXPECT_CALL(model, findMorphRef(_)).Times(AtLeast(1)).WillRepeatedly(Invoke(this, &MockModelAdapter::addMorph));
        EXPECT_CALL(model, getBoneRefs(_)).Times(AtLeast(1)).WillRepeatedly(Invoke(this, &MockModelAdapter::getBonesRef));
    }
private:
    IBone *addBone(const IString *name) {
        std::unique_ptr<MockIBone> bone(new MockIBone());
        EXPECT_CALL(*bone, name(IEncoding::kDefaultLanguage)).Times(AnyNumber()).WillRepeatedly(Return(name));
        EXPECT_CALL(*bone, isInverseKinematicsEnabled()).Times(AnyNumber()).WillRepeatedly(Return(false));
        EXPECT_CALL(*bone, hasInverseKinematics()).Times(AnyNumber()).WillRepeatedly(Return(false));
        m_bones->append(bone.get());
        return bone.release();
    }
    IMorph *addMorph(const IString *name) {
        std::unique_ptr<MockIMorph> morph(new MockIMorph());
        EXPECT_CALL(*morph, name(IEncoding::kDefaultLanguage)).Times(AnyNumber()).WillRepeatedly(Return(name));
        m_morphs->append(morph.get());
        return morph.release();
    }
    void getBonesRef(Array<IBone *> &a) {
        a.copy(*m_bones);
    }

    QScopedPointer<Array<IBone *>, ScopedPointerListDeleter> m_bones;
    QScopedPointer<Array<IMorph *>, ScopedPointerListDeleter> m_morphs;
};

static void CompareBoneInterpolationMatrix(const QuadWord p[], const mvd::BoneKeyframe &frame)
{
    QuadWord actual, expected = p[0];
    frame.getInterpolationParameter(mvd::BoneKeyframe::kBonePositionX, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
    expected = p[1];
    frame.getInterpolationParameter(mvd::BoneKeyframe::kBonePositionY, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
    expected = p[2];
    frame.getInterpolationParameter(mvd::BoneKeyframe::kBonePositionZ, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
    expected = p[3];
    frame.getInterpolationParameter(mvd::BoneKeyframe::kBoneRotation, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
}

static void CompareCameraInterpolationMatrix(const QuadWord p[], const mvd::CameraKeyframe &frame)
{
    QuadWord actual, expected = p[2];
    frame.getInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtX, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
    expected = p[3];
    frame.getInterpolationParameter(mvd::CameraKeyframe::kCameraAngle, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
    expected = p[4];
    frame.getInterpolationParameter(mvd::CameraKeyframe::kCameraDistance, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
    expected = p[5];
    frame.getInterpolationParameter(mvd::CameraKeyframe::kCameraFov, actual);
    ASSERT_TRUE(CompareVector(expected, actual));
}

}

TEST(MVDMotionTest, ParseEmpty)
{
    Encoding encoding(0);
    MockIModel model;
    mvd::Motion motion(&model, &encoding);
    mvd::Motion::DataInfo info;
    // parsing empty should be error
    ASSERT_FALSE(motion.preparse(reinterpret_cast<const uint8 *>(""), 0, info));
    ASSERT_EQ(mvd::Motion::kInvalidHeaderError, motion.error());
}

TEST(MVDMotionTest, ParseModelMotion)
{
    QFile file("motion.mvd");
    if (file.open(QFile::ReadOnly)) {
        const QByteArray &bytes = file.readAll();
        const uint8 *data = reinterpret_cast<const uint8 *>(bytes.constData());
        vsize size = bytes.size();
        Encoding encoding(0);
        MockIModel model;
        MockIBone bone;
        MockIMorph morph;
        EXPECT_CALL(model, findBoneRef(_)).Times(AnyNumber()).WillRepeatedly(Return(&bone));
        EXPECT_CALL(model, findMorphRef(_)).Times(AnyNumber()).WillRepeatedly(Return(&morph));
        mvd::Motion motion(&model, &encoding);
        mvd::Motion::DataInfo result;
        // valid model motion should be loaded successfully
        ASSERT_TRUE(motion.preparse(data, size, result));
        ASSERT_TRUE(motion.load(data, size));
        ASSERT_EQ(mvd::Motion::kNoError, motion.error());
    }
}

TEST(MVDMotionTest, ParseCameraMotion)
{
    QFile file("camera.mvd");
    if (file.open(QFile::ReadOnly)) {
        const QByteArray &bytes = file.readAll();
        const uint8 *data = reinterpret_cast<const uint8 *>(bytes.constData());
        vsize size = bytes.size();
        Encoding encoding(0);
        mvd::Motion motion(0, &encoding);
        mvd::Motion::DataInfo result;
        // valid camera motion should be loaded successfully
        ASSERT_TRUE(motion.preparse(data, size, result));
        ASSERT_TRUE(motion.load(data, size));
        ASSERT_EQ(mvd::Motion::kNoError, motion.error());
    }
}

TEST(MVDMotionTest, SaveBoneKeyframe)
{
    Encoding encoding(0);
    String str("This is test.");
    mvd::Motion motion(0, &encoding);
    mvd::BoneKeyframe frame(&motion), newFrame(&motion);
    Vector3 pos(1, 2, 3);
    Quaternion rot(4, 5, 6, 7);
    // initialize the bone frame to be copied
    frame.setLayerIndex(42);
    frame.setTimeIndex(42);
    frame.setName(&str);
    frame.setLocalTranslation(pos);
    frame.setLocalOrientation(rot);
    QuadWord px(8, 9, 10, 11),
            py(12, 13, 14, 15),
            pz(16, 17, 18, 19),
            pr(20, 21, 22, 23);
    QuadWord p[] = { px, py, pz, pr };
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBonePositionX, px);
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBonePositionY, py);
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBonePositionZ, pz);
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBoneRotation, pr);
    // write a bone frame to data and read it
    std::unique_ptr<uint8[]> ptr(new uint8[frame.estimateSize()]);
    frame.write(ptr.get());
    newFrame.read(ptr.get());
    // compare read bone frame
    ASSERT_EQ(0, motion.nameListSection()->key(&str));
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_EQ(frame.layerIndex(), newFrame.layerIndex());
    ASSERT_TRUE(CompareVector(pos, newFrame.localTranslation()));
    ASSERT_TRUE(CompareVector(rot, newFrame.localOrientation()));
    CompareBoneInterpolationMatrix(p, frame);
    // cloned bone frame shold be copied with deep
    std::unique_ptr<IBoneKeyframe> cloned(frame.clone());
    // ASSERT_TRUE(cloned->name()->equals(frame.name()));
    ASSERT_EQ(frame.layerIndex(), cloned->layerIndex());
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_TRUE(CompareVector(pos, cloned->localTranslation()));
    ASSERT_TRUE(CompareVector(rot, cloned->localOrientation()));
    CompareBoneInterpolationMatrix(p, *static_cast<mvd::BoneKeyframe *>(cloned.get()));
}

TEST(MVDMotionTest, SaveCameraKeyframe)
{
    Encoding encoding(0);
    mvd::Motion motion(0, &encoding);
    mvd::CameraKeyframe frame(&motion), newFrame(&motion);
    Vector3 pos(1, 2, 3), angle(4, 5, 6);
    // initialize the camera frame to be copied
    frame.setTimeIndex(42);
    frame.setLayerIndex(42);
    frame.setLookAt(pos);
    frame.setAngle(angle);
    frame.setDistance(7);
    frame.setFov(8);
    QuadWord px(9, 10, 11, 12),
            py(13, 14, 15, 16),
            pz(17, 18, 19, 20),
            pr(21, 22, 23, 24),
            pd(25, 26, 27, 28),
            pf(29, 30, 31, 32);
    QuadWord p[] = { px, py, pz, pr, pd, pf };
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtX, px);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtY, py);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtZ, pz);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraAngle, pr);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraDistance, pd);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraFov, pf);
    // write a camera frame to data and read it
    std::unique_ptr<uint8[]> ptr(new uint8[frame.estimateSize()]);
    frame.write(ptr.get());
    newFrame.read(ptr.get());
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_EQ(frame.layerIndex(), newFrame.layerIndex());
    ASSERT_TRUE(CompareVector(frame.lookAt(), newFrame.lookAt()));
    // compare read camera frame
    // for radian and degree calculation
    ASSERT_NEAR(newFrame.angle().x(), frame.angle().x(), 0.00001);
    ASSERT_NEAR(newFrame.angle().y(), frame.angle().y(), 0.00001);
    ASSERT_NEAR(newFrame.angle().z(), frame.angle().z(), 0.00001);
    ASSERT_FLOAT_EQ(newFrame.distance(), frame.distance());
    ASSERT_FLOAT_EQ(newFrame.fov(), frame.fov());
    CompareCameraInterpolationMatrix(p, frame);
    // cloned camera frame shold be copied with deep
    std::unique_ptr<ICameraKeyframe> cloned(frame.clone());
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_EQ(frame.layerIndex(), cloned->layerIndex());
    ASSERT_TRUE(CompareVector(frame.lookAt(), cloned->lookAt()));
    // for radian and degree calculation
    ASSERT_FLOAT_EQ(cloned->angle().x(), frame.angle().x());
    ASSERT_FLOAT_EQ(cloned->angle().y(), frame.angle().y());
    ASSERT_FLOAT_EQ(cloned->angle().z(), frame.angle().z());
    ASSERT_FLOAT_EQ(cloned->distance(), frame.distance());
    ASSERT_FLOAT_EQ(cloned->fov(), frame.fov());
    CompareCameraInterpolationMatrix(p, *static_cast<mvd::CameraKeyframe *>(cloned.get()));
}

TEST(MVDMotionTest, SaveMorphKeyframe)
{
    Encoding encoding(0);
    String str("This is test.");
    mvd::Motion motion(0, &encoding);
    mvd::MorphKeyframe frame(&motion), newFrame(&motion);
    // initialize the morph frame to be copied
    frame.setName(&str);
    frame.setTimeIndex(42);
    frame.setWeight(0.5);
    // write a morph frame to data and read it
    std::unique_ptr<uint8[]> ptr(new uint8[frame.estimateSize()]);
    frame.write(ptr.get());
    newFrame.read(ptr.get());
    // compare read morph frame
    ASSERT_EQ(0, motion.nameListSection()->key(&str));
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_EQ(frame.weight(), newFrame.weight());
    // cloned morph frame shold be copied with deep
    std::unique_ptr<IMorphKeyframe> cloned(frame.clone());
    // ASSERT_TRUE(cloned->name()->equals(frame.name()));
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_EQ(frame.weight(), cloned->weight());
}

struct MockGetBoneRefs {
    MockGetBoneRefs()
        : boneName("This is a test.")
    {
        EXPECT_CALL(bone, name(IEncoding::kDefaultLanguage)).Times(AnyNumber()).WillRepeatedly(Return(&boneName));
        EXPECT_CALL(bone, setInverseKinematicsEnable(false)).Times(AnyNumber());
        EXPECT_CALL(bone, isInverseKinematicsEnabled()).WillRepeatedly(Return(false));
        EXPECT_CALL(bone, hasInverseKinematics()).WillRepeatedly(Return(true));
        EXPECT_CALL(bone, index()).WillRepeatedly(Return(0));
    }
    void getBoneRefs(Array<IBone *> &value) {
        value.clear();
        value.append(&bone);
    }
    MockIBone bone;
    String boneName;
};

TEST(MVDMotionTest, SaveModelKeyframe)
{
    Encoding encoding(0);
    mvd::Motion motion(0, &encoding);
    MockIModel model;
    MockGetBoneRefs mocker;
    EXPECT_CALL(model, getBoneRefs(_)).WillRepeatedly(Invoke(&mocker, &MockGetBoneRefs::getBoneRefs));
    EXPECT_CALL(model, findBoneRef(_)).WillRepeatedly(Return(&mocker.bone));
    mvd::ModelSection section(&motion, &model, 0);
    std::unique_ptr<uint8[]> ptr(new uint8_t[section.estimateSize()]);
    section.write(ptr.get());
    section.read(ptr.get() + sizeof(mvd::Motion::SectionTag));
    mvd::ModelKeyframe frame(&section), newFrame(&section);
    // initialize the morph keyframe to be copied
    frame.setTimeIndex(42);
    frame.setAddBlendEnable(true);
    frame.setEdgeColor(Color(0.1, 0.2, 0.3, 0.4));
    frame.setEdgeWidth(0.5);
    frame.setInverseKinematicsEnable(0, false); // should not be crashed
    frame.setInverseKinematicsEnable(&mocker.bone, false);
    Hash<HashInt, IBone *> states;
    states.insert(0, &mocker.bone);
    frame.setInverseKinematicsState(states);
    frame.setLayerIndex(6);
    frame.setPhysicsEnable(false);
    frame.setPhysicsStillMode(7);
    frame.setShadowEnable(false);
    frame.setVisible(false);
    // write a morph keyframe to data and read it
    ptr.reset(new uint8[frame.estimateSize()]);
    frame.write(ptr.get());
    newFrame.read(ptr.get());
    // compare read model keyframe
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_EQ(frame.isAddBlendEnabled(), newFrame.isAddBlendEnabled());
    ASSERT_FLOAT_EQ(frame.edgeWidth(), newFrame.edgeWidth());
    ASSERT_TRUE(newFrame.isInverseKinematicsEnabled(0)); /* should not be crashed */
    ASSERT_FALSE(newFrame.isInverseKinematicsEnabled(&mocker.bone));
    ASSERT_EQ(frame.physicsStillMode(), newFrame.physicsStillMode());
    ASSERT_FALSE(newFrame.isPhysicsEnabled());
    ASSERT_FALSE(newFrame.isShadowEnabled());
    ASSERT_FALSE(newFrame.isVisible());
    // cloned model keyframe shold be copied with deep
    std::unique_ptr<IModelKeyframe> cloned(frame.clone());
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_EQ(frame.isAddBlendEnabled(), cloned->isAddBlendEnabled());
    ASSERT_FLOAT_EQ(frame.edgeWidth(), cloned->edgeWidth());
    ASSERT_TRUE(cloned->isInverseKinematicsEnabled(0)); /* should not be crashed */
    ASSERT_FALSE(cloned->isInverseKinematicsEnabled(&mocker.bone));
    ASSERT_EQ(frame.physicsStillMode(), cloned->physicsStillMode());
    ASSERT_FALSE(cloned->isPhysicsEnabled());
    ASSERT_FALSE(cloned->isShadowEnabled());
    ASSERT_FALSE(cloned->isVisible());
}

/*
TEST(MVDMotionTest, SaveLightKeyframe)
{
    mvd::LightKeyframe frame, newFrame;
    Vector3 color(0.1, 0.2, 0.3), direction(4, 5, 6);
    // initialize the light frame to be copied
    frame.setTimeIndex(42);
    frame.setColor(color);
    frame.setDirection(direction);
    // write a light frame to data and read it
    uint8_t data[mvd::LightKeyframe::strideSize()];
    frame.write(data);
    newFrame.read(data);
    // compare read light frame
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_TRUE(newFrame.color() == frame.color());
    ASSERT_TRUE(newFrame.direction() == frame.direction());
    // cloned morph frame shold be copied with deep
    std::unique_ptr<ILightKeyframe> cloned(frame.clone());
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_TRUE(cloned->color() == frame.color());
    ASSERT_TRUE(cloned->direction() == frame.direction());
}
*/

TEST(MVDMotionTest, SaveModelMotion)
{
    QFile file("motion.mvd");
    if (file.open(QFile::ReadOnly)) {
        const QByteArray &bytes = file.readAll();
        const uint8 *data = reinterpret_cast<const uint8 *>(bytes.constData());
        vsize size = bytes.size();
        Encoding encoding(0);
        MockIModel model;
        MockModelAdapter adapter(model); Q_UNUSED(adapter);
        mvd::Motion motion(&model, &encoding);
        ASSERT_TRUE(motion.load(data, size));
        vsize newSize = motion.estimateSize();
        //ASSERT_EQ(size, newSize);
        std::unique_ptr<uint8[]> newData(new uint8[newSize]);
        uint8 *ptr = newData.get();
        mvd::Motion motion2(&model, &encoding);
        motion.save(ptr);
        ASSERT_TRUE(motion2.load(ptr, newSize));
        ASSERT_EQ(motion.countKeyframes(IKeyframe::kBoneKeyframe), motion2.countKeyframes(IKeyframe::kBoneKeyframe));
        ASSERT_EQ(motion.countKeyframes(IKeyframe::kMorphKeyframe), motion2.countKeyframes(IKeyframe::kMorphKeyframe));
        ASSERT_EQ(motion.countKeyframes(IKeyframe::kModelKeyframe), motion2.countKeyframes(IKeyframe::kModelKeyframe));
    }
}

TEST(MVDMotionTest, SaveCameraMotion)
{
    QFile file("camera.mvd");
    if (file.open(QFile::ReadOnly)) {
        const QByteArray &bytes = file.readAll();
        const uint8 *data = reinterpret_cast<const uint8 *>(bytes.constData());
        vsize size = bytes.size();
        Encoding encoding(0);
        mvd::Motion motion(0, &encoding);
        ASSERT_TRUE(motion.load(data, size));
        vsize newSize = motion.estimateSize();
        std::unique_ptr<uint8[]> newData(new uint8[newSize]);
        uint8 *ptr = newData.get();
        mvd::Motion motion2(0, &encoding);
        motion.save(ptr);
        ASSERT_TRUE(motion2.load(ptr, newSize));
        ASSERT_EQ(motion.countKeyframes(IKeyframe::kCameraKeyframe), motion2.countKeyframes(IKeyframe::kCameraKeyframe));
    }
}

TEST(MVDMotionTest, BoneInterpolation)
{
    Encoding encoding(0);
    mvd::Motion motion(0, &encoding);
    mvd::BoneKeyframe frame(&motion);
    QuadWord n;
    frame.getInterpolationParameter(mvd::BoneKeyframe::kBonePositionX, n);
    ASSERT_TRUE(CompareVector(vpvl2::internal::InterpolationTable::defaultParameter(), n));
    QuadWord px(8, 9, 10, 11),
            py(12, 13, 14, 15),
            pz(16, 17, 18, 19),
            pr(20, 21, 22, 23);
    QuadWord p[] = { px, py, pz, pr };
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBonePositionX, px);
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBonePositionY, py);
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBonePositionZ, pz);
    frame.setInterpolationParameter(mvd::BoneKeyframe::kBoneRotation, pr);
    CompareBoneInterpolationMatrix(p, frame);
}

TEST(MVDMotionTest, CameraInterpolation)
{
    Encoding encoding(0);
    mvd::Motion motion(0, &encoding);
    mvd::CameraKeyframe frame(&motion);
    QuadWord n;
    frame.getInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtX, n);
    ASSERT_TRUE(CompareVector(vpvl2::internal::InterpolationTable::defaultParameter(), n));
    QuadWord px(9, 10, 11, 12),
            py(13, 14, 15, 16),
            pz(17, 18, 19, 20),
            pr(21, 22, 23, 24),
            pd(25, 26, 27, 28),
            pf(29, 30, 31, 32);
    QuadWord p[] = { px, py, pz, pr, pd, pf };
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtX, px);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtY, py);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraLookAtZ, pz);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraAngle, pr);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraDistance, pd);
    frame.setInterpolationParameter(mvd::CameraKeyframe::kCameraFov, pf);
    CompareCameraInterpolationMatrix(p, frame);
}

TEST(MVDMotionTest, AddAndRemoveBoneKeyframes)
{
    Encoding encoding(0);
    String name("bone");
    MockIModel model;
    MockIBone bone;
    mvd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kBoneKeyframe));
    // mock bone
    EXPECT_CALL(model, findBoneRef(_)).Times(AtLeast(1)).WillRepeatedly(Return(&bone));
    std::unique_ptr<IBoneKeyframe> keyframePtr(new mvd::BoneKeyframe(&motion));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // add a bone keyframe
        motion.addKeyframe(keyframePtr.get());
        IKeyframe *keyframe = keyframePtr.release();
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kBoneKeyframe));
        // boudary check of findBoneKeyframeAt
        ASSERT_EQ(static_cast<IBoneKeyframe *>(0), motion.findBoneKeyframeRefAt(-1));
        ASSERT_EQ(keyframe, motion.findBoneKeyframeRefAt(0));
        ASSERT_EQ(static_cast<IBoneKeyframe *>(0), motion.findBoneKeyframeRefAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(&name, IKeyframe::kBoneKeyframe));
        ASSERT_EQ(0, motion.findMorphKeyframeRef(42, &name, 1));
        // find a bone keyframe with timeIndex and name
        ASSERT_EQ(keyframe, motion.findBoneKeyframeRef(42, &name, 0));
    }
    keyframePtr.reset(new mvd::BoneKeyframe(&motion));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // replaced bone frame should be one keyframe
        motion.replaceKeyframe(keyframePtr.get(), true);
        IKeyframe *keyframeToDelete = keyframePtr.release();
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kBoneKeyframe));
        // no longer be find previous bone keyframe
        ASSERT_EQ(keyframeToDelete, motion.findBoneKeyframeRef(42, &name, 0));
        // delete bone keyframe and set it null
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kBoneKeyframe);
        // bone keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kBoneKeyframe));
        ASSERT_EQ(IKeyframe::TimeIndex(0), motion.durationTimeIndex());
        ASSERT_EQ(static_cast<IBoneKeyframe *>(0), motion.findBoneKeyframeRef(42, &name, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}

TEST(MVDMotionTest, AddAndRemoveCameraKeyframes)
{
    Encoding encoding(0);
    Model model(&encoding);
    mvd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kCameraKeyframe));
    std::unique_ptr<ICameraKeyframe> keyframePtr(new mvd::CameraKeyframe(&motion));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setDistance(42);
    {
        // add a camera keyframe
        motion.addKeyframe(keyframePtr.get());
        IKeyframe *keyframe = keyframePtr.release();
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kCameraKeyframe));
        // boudary check of findCameraKeyframeAt
        ASSERT_EQ(static_cast<ICameraKeyframe *>(0), motion.findCameraKeyframeRefAt(-1));
        ASSERT_EQ(keyframe, motion.findCameraKeyframeRefAt(0));
        ASSERT_EQ(static_cast<ICameraKeyframe *>(0), motion.findCameraKeyframeRefAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(0, IKeyframe::kCameraKeyframe));
        ASSERT_EQ(0, motion.findCameraKeyframeRef(42, 1));
        // find a camera keyframe with timeIndex
        ASSERT_EQ(keyframe, motion.findCameraKeyframeRef(42, 0));
    }
    keyframePtr.reset(new mvd::CameraKeyframe(&motion));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setDistance(84);
    {
        // replaced camera frame should be one keyframe
        motion.replaceKeyframe(keyframePtr.get(), true);
        IKeyframe *keyframeToDelete = keyframePtr.release();
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kCameraKeyframe));
        // no longer be find previous camera keyframe
        ASSERT_EQ(84.0f, motion.findCameraKeyframeRef(42, 0)->distance());
        // delete camera keyframe and set it null (don't forget updating motion!)
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kCameraKeyframe);
        // camera keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kCameraKeyframe));
        ASSERT_EQ(IKeyframe::TimeIndex(0), motion.durationTimeIndex());
        ASSERT_EQ(static_cast<ICameraKeyframe *>(0), motion.findCameraKeyframeRef(42, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}

/*
TEST(MVDMotionTest, AddAndRemoveLightKeyframes)
{
    Encoding encoding;
    Model model(&encoding);
    mvd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kLight));
    std::unique_ptr<ILightKeyframe> frame(new mvd::LightKeyframe());
    frame->setTimeIndex(42);
    frame->setColor(Vector3(1, 0, 0));
    {
        // add a light keyframe (don't forget updating motion!)
        motion.addKeyframe(frame.get());
        motion.update(IKeyframe::kLight);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kLight));
        // boudary check of findLightKeyframeAt
        ASSERT_EQ(static_cast<ILightKeyframe *>(0), motion.findLightKeyframeAt(-1));
        ASSERT_EQ(frame.get(), motion.findLightKeyframeAt(0));
        ASSERT_EQ(static_cast<ILightKeyframe *>(0), motion.findLightKeyframeAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(0, IKeyframe::kLight));
        ASSERT_EQ(0, motion.findLightKeyframe(42, 1));
        // find a light keyframe with timeIndex
        ASSERT_EQ(frame.release(), motion.findLightKeyframe(42, 0));
    }
    std::unique_ptr<ILightKeyframe> frame2(new mvd::LightKeyframe());
    frame2->setTimeIndex(42);
    frame2->setColor(Vector3(0, 0, 1));
    {
        // replaced light frame should be one keyframe (don't forget updating motion!)
        motion.replaceKeyframe(frame2.get());
        motion.update(IKeyframe::kLight);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kLight));
        // no longer be find previous light keyframe
        ASSERT_EQ(1.0f, motion.findLightKeyframe(42, 0)->color().z());
    }
    {
        IKeyframe *keyframeToDelete = frame2.release();
        // delete light keyframe and set it null (don't forget updating motion!)
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kLight);
        // light keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kLight));
        ASSERT_EQ(static_cast<ILightKeyframe *>(0), motion.findLightKeyframe(42, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}
*/

TEST(MVDMotionTest, AddAndRemoveMorphKeyframes)
{
    Encoding encoding(0);
    String name("morph");
    MockIModel model;
    MockIMorph morph;
    mvd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kMorphKeyframe));
    // mock morph
    EXPECT_CALL(model, findMorphRef(_)).Times(AtLeast(1)).WillRepeatedly(Return(&morph));
    std::unique_ptr<IMorphKeyframe> keyframePtr(new mvd::MorphKeyframe(&motion));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // add a morph keyframe
        motion.addKeyframe(keyframePtr.get());
        IKeyframe *keyframe = keyframePtr.release();
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kMorphKeyframe));
        // boudary check of findMorphKeyframeAt
        ASSERT_EQ(static_cast<IMorphKeyframe *>(0), motion.findMorphKeyframeRefAt(-1));
        ASSERT_EQ(keyframe, motion.findMorphKeyframeRefAt(0));
        ASSERT_EQ(static_cast<IMorphKeyframe *>(0), motion.findMorphKeyframeRefAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(&name, IKeyframe::kMorphKeyframe));
        ASSERT_EQ(0, motion.findMorphKeyframeRef(42, &name, 1));
        ASSERT_EQ(keyframe, motion.findMorphKeyframeRef(42, &name, 0));
    }
    keyframePtr.reset(new mvd::MorphKeyframe(&motion));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // replaced morph frame should be one keyframe
        motion.replaceKeyframe(keyframePtr.get(), true);
        IKeyframe *keyframeToDelete = keyframePtr.release();
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kMorphKeyframe));
        // no longer be find previous morph keyframe
        ASSERT_EQ(keyframeToDelete, motion.findMorphKeyframeRef(42, &name, 0));
        // delete light keyframe and set it null
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kMorphKeyframe);
        // morph keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kMorphKeyframe));
        ASSERT_EQ(IKeyframe::TimeIndex(0), motion.durationTimeIndex());
        ASSERT_EQ(static_cast<IMorphKeyframe *>(0), motion.findMorphKeyframeRef(42, &name, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}

TEST(MVDMotionTest, AddAndRemoveNullKeyframe)
{
    /* should happen nothing */
    Encoding encoding(0);
    MockIModel model;
    IKeyframe *nullKeyframe = 0;
    mvd::Motion motion(&model, &encoding);
    motion.addKeyframe(nullKeyframe);
    motion.replaceKeyframe(nullKeyframe, true);
    motion.deleteKeyframe(nullKeyframe);
}

class MVDMotionAllKeyframesTest : public TestWithParam<IKeyframe::Type> {};

TEST_P(MVDMotionAllKeyframesTest, SetAndGetAllKeyframes)
{
    Encoding::Dictionary dictionary;
    Encoding encoding(&dictionary);
    MockIModel model;
    mvd::Motion motion(&model, &encoding);
    Array<IKeyframe *> source, dest;
    IKeyframe::Type type = GetParam();
    std::unique_ptr<mvd::BoneKeyframe> boneKeyframe(new mvd::BoneKeyframe(&motion));
    boneKeyframe->setName(encoding.stringConstant(IEncoding::kMaxConstantType));
    source.append(boneKeyframe.get());
    std::unique_ptr<mvd::CameraKeyframe> cameraKeyframe(new mvd::CameraKeyframe(&motion));
    source.append(cameraKeyframe.get());
    std::unique_ptr<mvd::EffectKeyframe> effectKeyframe(new mvd::EffectKeyframe(&motion));
    source.append(effectKeyframe.get());
    std::unique_ptr<mvd::LightKeyframe> lightKeyframe(new mvd::LightKeyframe(&motion));
    source.append(lightKeyframe.get());
    std::unique_ptr<mvd::ModelKeyframe> modelKeyframe(new mvd::ModelKeyframe(0));
    source.append(modelKeyframe.get());
    std::unique_ptr<mvd::MorphKeyframe> morphKeyframe(new mvd::MorphKeyframe(&motion));
    morphKeyframe->setName(encoding.stringConstant(IEncoding::kMaxConstantType));
    source.append(morphKeyframe.get());
    std::unique_ptr<mvd::ProjectKeyframe> projectKeyframe(new mvd::ProjectKeyframe(&motion));
    source.append(projectKeyframe.get());
    motion.setAllKeyframes(source, type);
    boneKeyframe.release();
    cameraKeyframe.release();
    effectKeyframe.release();
    lightKeyframe.release();
    modelKeyframe.release();
    morphKeyframe.release();
    projectKeyframe.release();
    motion.getAllKeyframeRefs(dest, type);
    ASSERT_EQ(1, dest.count());
    ASSERT_EQ(type, dest[0]->type());
}

INSTANTIATE_TEST_CASE_P(MVDMotionInstance, MVDMotionAllKeyframesTest,
                        Values(IKeyframe::kBoneKeyframe,
                               IKeyframe::kCameraKeyframe, IKeyframe::kEffectKeyframe,
                               IKeyframe::kLightKeyframe, IKeyframe::kModelKeyframe,
                               IKeyframe::kMorphKeyframe, IKeyframe::kProjectKeyframe));
