/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2013  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#include "vpvl2/vpvl2.h"
#include "vpvl2/cg/EffectEngine.h"
#include "vpvl2/cg/EffectContext.h"
#include "vpvl2/internal/util.h"

#include "vpvl2/extensions/cg/Util.h"
#include "vpvl2/extensions/gl/FrameBufferObject.h"
#include "vpvl2/extensions/gl/Texture2D.h"
#include "vpvl2/extensions/gl/Texture3D.h"
#include "vpvl2/extensions/gl/TexturePtrRef.h"
#include "vpvl2/extensions/gl/VertexBundleLayout.h"

#include <string.h> /* for Linux */

#include <string>
#include <sstream>

namespace
{

using namespace vpvl2;
using namespace vpvl2::cg;
using namespace vpvl2::extensions::cg;

static const Scalar kWidth = 1, kHeight = 1;
static const Vector4 kVertices[] = {
    btVector4(-kWidth,  kHeight,  0,  1),
    btVector4(-kWidth, -kHeight,  0,  0),
    btVector4( kWidth, -kHeight,  1,  0),
    btVector4( kWidth,  kHeight,  1,  1)
};
static const size_t kVertexStride = sizeof(kVertices[0]);
static const int kIndices[] = { 0, 1, 2, 3 };
static const uint8_t *kBaseAddress = reinterpret_cast<const uint8_t *>(&kVertices[0]);
static const size_t kTextureOffset = reinterpret_cast<const uint8_t *>(&kVertices[0].z()) - kBaseAddress;
static const size_t kIndicesSize = sizeof(kIndices) / sizeof(kIndices[0]);
static const EffectEngine::DrawPrimitiveCommand kQuadDrawCommand = EffectEngine::DrawPrimitiveCommand(GL_QUADS, kIndicesSize, GL_UNSIGNED_INT, 0, 0, sizeof(int));
static const char kWorldSemantic[] = "WORLD";
static const char kViewSemantic[] = "VIEW";
static const char kProjectionSemantic[] = "PROJECTION";
static const char kWorldViewSemantic[] = "WORLDVIEW";
static const char kViewProjectionSemantic[] = "VIEWPROJECTION";
static const char kWorldViewProjectionSemantic[] = "WORLDVIEWPROJECTION";
static const char kInverseTransposeSemanticsSuffix[] = "INVERSETRANSPOSE";
static const char kTransposeSemanticsSuffix[] = "TRANSPOSE";
static const char kInverseSemanticsSuffix[] = "INVERSE";
static const char kMultipleTechniquesPrefix[] = "Technique=Technique?";
static const char kSingleTechniquePrefix[] = "Technique=";

}

namespace vpvl2
{
namespace cg
{

/* BasicParameter */

BaseParameter::BaseParameter()
    : m_parameterRef(0)
{
}

BaseParameter::~BaseParameter()
{
    invalidateParameter();
    m_parameterRef = 0;
}

void BaseParameter::addParameter(IEffect::IParameter *parameter)
{
    connectParameter(parameter, m_parameterRef);
}

void BaseParameter::invalidateParameter()
{
    if (m_parameterRef) {
        m_parameterRef->reset();
    }
}

void BaseParameter::connectParameter(IEffect::IParameter *sourceParameter, IEffect::IParameter *&destinationParameter)
{
    /* prevent infinite reference loop */
    if (sourceParameter && sourceParameter != destinationParameter) {
        sourceParameter->connect(destinationParameter);
        destinationParameter = sourceParameter;
    }
}

/* BoolParameter */

BooleanParameter::BooleanParameter()
    : BaseParameter()
{
}

BooleanParameter::~BooleanParameter()
{
}

void BooleanParameter::setValue(bool value)
{
    if (m_parameterRef) {
        m_parameterRef->setValue(value);
    }
}

/* IntegerParameter */

IntegerParameter::IntegerParameter()
    : BaseParameter()
{
}

IntegerParameter::~IntegerParameter()
{
}

void IntegerParameter::setValue(int value)
{
    if (m_parameterRef) {
        m_parameterRef->setValue(value);
    }
}

/* FloatParameter */

FloatParameter::FloatParameter()
{
}

FloatParameter::~FloatParameter()
{
}

void FloatParameter::setValue(float value)
{
    if (m_parameterRef) {
        m_parameterRef->setValue(value);
    }
}

/* Float2Parameter */

Float2Parameter::Float2Parameter()
    : BaseParameter()
{
}

Float2Parameter::~Float2Parameter()
{
}

void Float2Parameter::setValue(const Vector3 &value)
{
    if (m_parameterRef) {
        m_parameterRef->setValue(value);
    }
}

/* Float4Parameter */

Float4Parameter::Float4Parameter()
    : BaseParameter()
{
}

Float4Parameter::~Float4Parameter()
{
}

void Float4Parameter::setValue(const Vector4 &value)
{
    if (m_parameterRef) {
        m_parameterRef->setValue(value);
    }
}

/* MatrixSemantic */

MatrixSemantic::MatrixSemantic(const IRenderContext *renderContextRef, int flags)
    : BaseParameter(),
      m_renderContextRef(renderContextRef),
      m_camera(0),
      m_cameraInversed(0),
      m_cameraTransposed(0),
      m_cameraInverseTransposed(0),
      m_light(0),
      m_lightInversed(0),
      m_lightTransposed(0),
      m_lightInverseTransposed(0),
      m_flags(flags)
{
}

MatrixSemantic::~MatrixSemantic()
{
    invalidateParameter();
    m_camera = 0;
    m_cameraInversed = 0;
    m_cameraTransposed = 0;
    m_cameraInverseTransposed = 0;
    m_light = 0;
    m_lightInversed = 0;
    m_lightTransposed = 0;
    m_lightInverseTransposed = 0;
    m_flags = 0;
    m_renderContextRef = 0;
}

void MatrixSemantic::addParameter(IEffect::IParameter *parameterRef, const char *suffix)
{
    if (const IEffect::IAnnotation *annotationRef = parameterRef->annotationRef("Object")) {
        const char *name = annotationRef->stringValue();
        const size_t len = strlen(name);
        if (VPVL2_CG_STREQ_CONST(name, len, "Camera")) {
            setParameter(suffix, parameterRef, m_cameraInversed, m_cameraTransposed, m_cameraInverseTransposed, m_camera);
        }
        else if (VPVL2_CG_STREQ_CONST(name, len, "Light")) {
            setParameter(suffix, parameterRef, m_lightInversed, m_lightTransposed, m_lightInverseTransposed, m_light);
        }
    }
    else {
        setParameter(suffix, parameterRef, m_cameraInversed, m_cameraTransposed, m_cameraInverseTransposed, m_camera);
    }
}

void MatrixSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_camera->reset();
    m_cameraInversed->reset();
    m_cameraTransposed->reset();
    m_cameraInverseTransposed->reset();
    m_light->reset();
    m_lightInversed->reset();
    m_lightTransposed->reset();
    m_lightInverseTransposed->reset();
}

void MatrixSemantic::setMatrices(const IModel *model, int extraCameraFlags, int extraLightFlags)
{
    setMatrix(model, m_camera,
              extraCameraFlags | IRenderContext::kCameraMatrix);
    setMatrix(model, m_cameraInversed,
              extraCameraFlags | IRenderContext::kCameraMatrix | IRenderContext::kInverseMatrix);
    setMatrix(model, m_cameraTransposed,
              extraCameraFlags | IRenderContext::kCameraMatrix | IRenderContext::kTransposeMatrix);
    setMatrix(model, m_cameraInverseTransposed,
              extraCameraFlags | IRenderContext::kCameraMatrix | IRenderContext::kInverseMatrix | IRenderContext::kTransposeMatrix);
    setMatrix(model, m_light,
              extraLightFlags | IRenderContext::kLightMatrix);
    setMatrix(model, m_lightInversed,
              extraLightFlags | IRenderContext::kLightMatrix | IRenderContext::kInverseMatrix);
    setMatrix(model, m_lightTransposed,
              extraLightFlags | IRenderContext::kLightMatrix | IRenderContext::kTransposeMatrix);
    setMatrix(model, m_lightInverseTransposed,
              extraLightFlags | IRenderContext::kLightMatrix | IRenderContext::kInverseMatrix | IRenderContext::kTransposeMatrix);
}

void MatrixSemantic::setParameter(const char *suffix,
                                  IEffect::IParameter *sourceParameterRef,
                                  IEffect::IParameter *&inverse,
                                  IEffect::IParameter *&transposedRef,
                                  IEffect::IParameter *&inversetransposedRef,
                                  IEffect::IParameter *&baseParameterRef)
{
    const size_t len = strlen(suffix);
    if (VPVL2_CG_STREQ_CONST(suffix, len, kInverseTransposeSemanticsSuffix)) {
        BaseParameter::connectParameter(sourceParameterRef, inversetransposedRef);
    }
    else if (VPVL2_CG_STREQ_CONST(suffix, len, kTransposeSemanticsSuffix)) {
        BaseParameter::connectParameter(sourceParameterRef, transposedRef);
    }
    else if (VPVL2_CG_STREQ_CONST(suffix, len, kInverseSemanticsSuffix)) {
        BaseParameter::connectParameter(sourceParameterRef, inverse);
    }
    else {
        BaseParameter::connectParameter(sourceParameterRef, baseParameterRef);
    }
}

void MatrixSemantic::setMatrix(const IModel *model, IEffect::IParameter *parameterRef, int flags)
{
    if (parameterRef) {
        float matrix[16];
        m_renderContextRef->getMatrix(matrix, model, m_flags | flags);
        parameterRef->setMatrix(matrix);
    }
}

/* Materialemantic */

MaterialSemantic::MaterialSemantic()
    : BaseParameter(),
      m_geometry(0),
      m_light(0)
{
}

MaterialSemantic::~MaterialSemantic()
{
    invalidateParameter();
    m_geometry = 0;
    m_light = 0;
}

void MaterialSemantic::addParameter(IEffect::IParameter *parameterRef)
{
    const char *name = parameterRef->semantic();
    const size_t nlen = strlen(name);
    if (VPVL2_CG_STREQ_CONST(name, nlen, "SPECULARPOWER")
            || VPVL2_CG_STREQ_CONST(name, nlen, "EDGECOLOR")
            || VPVL2_CG_STREQ_CONST(name, nlen, "EMISSIVE")
            || VPVL2_CG_STREQ_CONST(name, nlen, "TOONCOLOR")) {
        BaseParameter::connectParameter(parameterRef, m_geometry);
    }
    else if (const IEffect::IAnnotation *annotationRef = parameterRef->annotationRef("Object")) {
        const char *aname = annotationRef->stringValue();
        const size_t alen = strlen(aname);
        if (VPVL2_CG_STREQ_CONST(aname, alen,  "Geometry")) {
            BaseParameter::connectParameter(parameterRef, m_geometry);
        }
        else if (VPVL2_CG_STREQ_CONST(aname, alen, "Light")) {
            BaseParameter::connectParameter(parameterRef, m_light);
        }
    }
}

void MaterialSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    if (m_geometry) {
        m_geometry->reset();
    }
    if (m_light) {
        m_light->reset();
    }
}

void MaterialSemantic::setGeometryColor(const Vector3 &value)
{
    if (m_geometry) {
        m_geometry->setValue(value);
    }
}

void MaterialSemantic::setGeometryValue(const Scalar &value)
{
    if (m_geometry) {
        m_geometry->setValue(value);
    }
}

void MaterialSemantic::setLightColor(const Vector3 &value)
{
    if (m_light) {
        m_light->setValue(value);
    }
}

void MaterialSemantic::setLightValue(const Scalar &value)
{
    if (m_light) {
        m_light->setValue(value);
    }
}

/* MaterialTextureSemantic */

MaterialTextureSemantic::MaterialTextureSemantic()
    : BaseParameter(),
      m_mipmap(false)
{
}

MaterialTextureSemantic::~MaterialTextureSemantic()
{
    invalidateParameter();
}

bool MaterialTextureSemantic::hasMipmap(const IEffect::IParameter *textureParameterRef, const IEffect::IParameter *samplerParameterRef)
{
    bool hasMipmap = false;
    if (const IEffect::IAnnotation *annotation = textureParameterRef->annotationRef("MipLevels")) {
        hasMipmap = annotation->integerValue() != 1;
    }
    if (const IEffect::IAnnotation *annotaiton = textureParameterRef->annotationRef("Level")) {
        hasMipmap = annotaiton->integerValue() != 1;
    }
    Array<IEffect::ISamplerState *> states;
    samplerParameterRef->getSamplerStateRefs(states);
    const int nstates = states.count();
    for (int i = 0; i < nstates; i++) {
        const IEffect::ISamplerState *state = states[i];
        if (state->type() == IEffect::IParameter::kInteger) {
            const char *name = state->name();
            const size_t len = strlen(name);
            if (VPVL2_CG_STREQ_CASE_CONST(name, len, "MINFILTER")) {
                int value = 0;
                state->getValue(value);
                switch (value) {
                case GL_NEAREST_MIPMAP_NEAREST:
                case GL_NEAREST_MIPMAP_LINEAR:
                case GL_LINEAR_MIPMAP_NEAREST:
                case GL_LINEAR_MIPMAP_LINEAR:
                    hasMipmap = true;
                    break;
                default:
                    break;
                }
                if (hasMipmap) {
                    break;
                }
            }
        }
    }
    return hasMipmap;
}

void MaterialTextureSemantic::addParameter(const IEffect::IParameter *textureParameterRef, IEffect::IParameter *samplerParameterRef)
{
    if (hasMipmap(textureParameterRef, samplerParameterRef)) {
        m_mipmap = true;
    }
    BaseParameter::addParameter(samplerParameterRef);
}

void MaterialTextureSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_textures.clear();
    m_mipmap = false;
}

void MaterialTextureSemantic::setTexture(const HashPtr &key, const ITexture *value)
{
    if (m_parameterRef && value) {
        m_textures.insert(key, value);
        m_parameterRef->setSampler(value);
    }
}

void MaterialTextureSemantic::updateParameter(const HashPtr &key)
{
    if (const ITexture *const *value = m_textures.find(key)) {
        const ITexture *textureRef = *value;
        m_parameterRef->setTexture(textureRef);
    }
}

/* TextureUnit */

TextureUnit::TextureUnit()
    : BaseParameter()
{
}

TextureUnit::~TextureUnit()
{
}

void TextureUnit::setTexture(GLuint value)
{
    m_parameterRef->setTexture(static_cast<intptr_t>(value));
}

/* GeometrySemantic */

GeometrySemantic::GeometrySemantic()
    : BaseParameter(),
      m_camera(0),
      m_light(0)
{
}

GeometrySemantic::~GeometrySemantic()
{
    invalidateParameter();
    m_camera = 0;
    m_light = 0;
}

void GeometrySemantic::addParameter(IEffect::IParameter *parameterRef)
{
    if (const IEffect::IAnnotation *annotationRef = parameterRef->annotationRef("Object")) {
        const char *name = annotationRef->stringValue();
        const size_t len = strlen(name);
        if (VPVL2_CG_STREQ_CONST(name, len, "Camera")) {
            BaseParameter::connectParameter(parameterRef, m_camera);
        }
        else if (VPVL2_CG_STREQ_CONST(name, len, "Light")) {
            BaseParameter::connectParameter(parameterRef, m_light);
        }
    }
}

void GeometrySemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    if (m_camera) {
        m_camera->reset();
    }
    if (m_light) {
        m_light->reset();
    }
}

void GeometrySemantic::setCameraValue(const Vector3 &value)
{
    if (m_camera) {
        m_camera->setValue(value);
    }
}

void GeometrySemantic::setLightValue(const Vector3 &value)
{
    if (m_light) {
        m_light->setValue(value);
    }
}

/* TimeSemantic */

TimeSemantic::TimeSemantic(const IRenderContext *renderContextRef)
    : BaseParameter(),
      m_renderContextRef(renderContextRef),
      m_syncEnabled(0),
      m_syncDisabled(0)
{
}

TimeSemantic::~TimeSemantic()
{
    invalidateParameter();
    m_syncEnabled = 0;
    m_syncDisabled = 0;
}

void TimeSemantic::addParameter(IEffect::IParameter *parameterRef)
{
    if (const IEffect::IAnnotation *annotationRef = parameterRef->annotationRef("SyncInEditMode")) {
        if (annotationRef->stringValue()) {
            BaseParameter::connectParameter(parameterRef, m_syncEnabled);
            return;
        }
    }
    BaseParameter::connectParameter(parameterRef, m_syncDisabled);
}

void TimeSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    if (m_syncEnabled) {
        m_syncEnabled->reset();
    }
    if (m_syncDisabled) {
        m_syncDisabled->reset();
    }
}

void TimeSemantic::update()
{
    float value = 0;
    if (m_syncEnabled) {
        m_renderContextRef->getTime(value, true);
        m_syncEnabled->setValue(value);
    }
    if (m_syncDisabled) {
        m_renderContextRef->getTime(value, false);
        m_syncDisabled->setValue(value);
    }
}

/* ControlObjectSemantic */

ControlObjectSemantic::ControlObjectSemantic(const Scene *sceneRef, const IRenderContext *renderContextRef)
    : BaseParameter(),
      m_sceneRef(sceneRef),
      m_renderContextRef(renderContextRef)
{
}

ControlObjectSemantic::~ControlObjectSemantic()
{
    m_sceneRef = 0;
    m_renderContextRef = 0;
}

void ControlObjectSemantic::addParameter(IEffect::IParameter *parameterRef)
{
    if (parameterRef->annotationRef("name")) {
        m_parameterRefs.append(parameterRef);
    }
}

void ControlObjectSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_parameterRefs.clear();
}

void ControlObjectSemantic::update(const IModel *self)
{
    const int nparameters = m_parameterRefs.count();
    for (int i = 0; i < nparameters; i++) {
        IEffect::IParameter *parameterRef = m_parameterRefs[i];
        if (const IEffect::IAnnotation *annotationRef = parameterRef->annotationRef("name")) {
            const char *name = annotationRef->stringValue();
            const size_t len = strlen(name);
            if (VPVL2_CG_STREQ_CONST(name, len, "(self)")) {
                setParameter(self, parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, len, "(OffscreenOwner)")) {
                if (IEffect *parent = m_parameterRef->parentEffectRef()->parentEffectRef()) {
                    const IModel *model = m_renderContextRef->effectOwner(parent);
                    setParameter(model, parameterRef);
                }
            }
            else {
                IString *s = m_renderContextRef->toUnicode(reinterpret_cast<const uint8_t *>(name));
                const IModel *model = m_renderContextRef->findModel(s);
                delete s;
                setParameter(model, parameterRef);
            }
        }
    }
}

void ControlObjectSemantic::setParameter(const IModel *model, IEffect::IParameter *parameterRef)
{
    const IEffect::IParameter::Type parameterType = parameterRef->type();
    if (model) {
        if (const IEffect::IAnnotation *annotation = parameterRef->annotationRef("item")) {
            const char *item = annotation->stringValue();
            const size_t len = strlen(item);
            const IModel::Type type = model->type();
            if (type == IModel::kPMDModel || type == IModel::kPMXModel) {
                const IString *s = m_renderContextRef->toUnicode(reinterpret_cast<const uint8_t *>(item));
                IBone *bone = model->findBone(s);
                IMorph *morph = model->findMorph(s);
                delete s;
                if (bone) {
                    float matrix4x4[16] = { 0 };
                    switch (parameterType) {
                    case IEffect::IParameter::kFloat3:
                    case IEffect::IParameter::kFloat4:
                        parameterRef->setValue(bone->worldTransform().getOrigin());
                        break;
                    case IEffect::IParameter::kFloat4x4:
                        bone->worldTransform().getOpenGLMatrix(matrix4x4);
                        parameterRef->setMatrix(matrix4x4);
                        break;
                    default:
                        break;
                    }
                }
                else if (morph) {
                    parameterRef->setValue(float(morph->weight()));
                }
            }
            else {
                const Vector3 &position = model->worldPosition();
                const Quaternion &rotation = model->worldRotation();
                if (VPVL2_CG_STREQ_CONST(item, len, "X")) {
                    parameterRef->setValue(position.x());
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Y")) {
                    parameterRef->setValue(position.y());
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Z")) {
                    parameterRef->setValue(position.z());
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "XYZ")) {
                    parameterRef->setValue(position);
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Rx")) {
                    parameterRef->setValue(btDegrees(rotation.x()));
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Ry")) {
                    parameterRef->setValue(btDegrees(rotation.y()));
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Rz")) {
                    parameterRef->setValue(btDegrees(rotation.z()));
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Rxyz")) {
                    const Vector3 rotationDegree(btDegrees(rotation.x()), btDegrees(rotation.y()), btDegrees(rotation.z()));
                    parameterRef->setValue(rotationDegree);
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Si")) {
                    parameterRef->setValue(model->scaleFactor());
                }
                else if (VPVL2_CG_STREQ_CONST(item, len, "Tr")) {
                    parameterRef->setValue(model->opacity());
                }
            }
        }
        else {
            float matrix4x4[16] = { 0 };
            switch (parameterType) {
            case IEffect::IParameter::kBoolean:
                parameterRef->setValue(model->isVisible());
                break;
            case IEffect::IParameter::kFloat:
                parameterRef->setValue(model->scaleFactor());
                break;
            case IEffect::IParameter::kFloat3:
            case IEffect::IParameter::kFloat4:
                parameterRef->setValue(model->worldPosition());
                break;
            case IEffect::IParameter::kFloat4x4:
                m_renderContextRef->getMatrix(matrix4x4, model, IRenderContext::kWorldMatrix | IRenderContext::kCameraMatrix);
                parameterRef->setMatrix(matrix4x4);
                break;
            default:
                break;
            }
        }
    }
    else {
        const IEffect::IParameter::Type type = parameterRef->type();
        float matrix4x4[16] = { 0 };
        switch (type) {
        case IEffect::IParameter::kBoolean:
            parameterRef->setValue(false);
            break;
        case IEffect::IParameter::kFloat:
            parameterRef->setValue(0.0f);
            break;
        case IEffect::IParameter::kFloat3:
        case IEffect::IParameter::kFloat4:
            parameterRef->setValue(kZeroV4);
            break;
        case IEffect::IParameter::kFloat4x4:
            Transform::getIdentity().getOpenGLMatrix(matrix4x4);
            parameterRef->setValue(matrix4x4);
            break;
        default:
            break;
        }
    }
}

/* RenderColorTargetSemantic */

RenderColorTargetSemantic::RenderColorTargetSemantic(IRenderContext *renderContextRef)
    : BaseParameter(),
      m_renderContextRef(renderContextRef)
{
}

RenderColorTargetSemantic::~RenderColorTargetSemantic()
{
    m_textures.releaseAll();
    m_renderContextRef = 0;
}

bool RenderColorTargetSemantic::tryGetTextureFlags(const IEffect::IParameter *textureParameterRef,
                                                   const IEffect::IParameter *samplerParameterRef,
                                                   bool enableAllTextureTypes,
                                                   int &flags)
{
    const IEffect::IAnnotation *annotationRef = textureParameterRef->annotationRef("ResourceType");
    if (enableAllTextureTypes && annotationRef) {
        const char *typeName = annotationRef->stringValue();
        const size_t len = strlen(typeName);
        const IEffect::IParameter::Type samplerType = samplerParameterRef->type();
        if (VPVL2_CG_STREQ_CONST(typeName, len, "CUBE") && samplerType == IEffect::IParameter::kSamplerCube) {
            flags = IRenderContext::kTextureCube;
        }
        else if (VPVL2_CG_STREQ_CONST(typeName, len, "3D") && samplerType == IEffect::IParameter::kSampler3D) {
            flags = IRenderContext::kTexture3D;
        }
        else if (VPVL2_CG_STREQ_CONST(typeName, len, "2D") && samplerType == IEffect::IParameter::kSampler2D) {
            flags = IRenderContext::kTexture2D;
        }
        else {
            return false;
        }
    }
    else {
        flags = IRenderContext::kTexture2D;
    }
    if (MaterialTextureSemantic::hasMipmap(textureParameterRef, samplerParameterRef)) {
        flags |= IRenderContext::kGenerateTextureMipmap;
    }
    return true;
}

void RenderColorTargetSemantic::addParameter(IEffect::IParameter *textureParameterRef,
                                             IEffect::IParameter *samplerParameterRef,
                                             FrameBufferObject *frameBufferObjectRef,
                                             const IString *dir,
                                             bool enableResourceName,
                                             bool enableAllTextureTypes)
{
    int flags;
    if (!tryGetTextureFlags(textureParameterRef, samplerParameterRef, enableAllTextureTypes, flags)) {
        return;
    }
    const IEffect::IAnnotation *annotationRef = textureParameterRef->annotationRef("ResourceName");
    const char *textureParameterName = textureParameterRef->name();
    IRenderContext::SharedTextureParameter sharedTextureParameter(textureParameterRef);
    const ITexture *textureRef = 0;
    if (enableResourceName && annotationRef) {
        const char *name = annotationRef->stringValue();
        IString *s = m_renderContextRef->toUnicode(reinterpret_cast<const uint8_t*>(name));
        IRenderContext::Texture texture(flags);
        texture.async = false;
        if (m_renderContextRef->uploadTexture(s, dir, texture, 0)) {
            textureRef = texture.texturePtrRef;
            if (textureRef) {
                samplerParameterRef->setSampler(textureRef);
                m_path2parameterRefs.insert(name, textureParameterRef);
                ITexture *tex = m_textures.append(new TexturePtrRef(textureRef));
                m_name2textures.insert(textureParameterName, Texture(frameBufferObjectRef, tex, textureParameterRef, samplerParameterRef));
            }
        }
        delete s;
    }
    else if (m_renderContextRef->tryGetSharedTextureParameter(textureParameterName, sharedTextureParameter)) {
        IEffect::IParameter *parameterRef = sharedTextureParameter.parameterRef;
        if (strcmp(parameterRef->semantic(), textureParameterRef->semantic()) == 0) {
            textureParameterRef = parameterRef;
            textureRef = sharedTextureParameter.textureRef;
        }
    }
    else if ((flags & IRenderContext::kTexture3D) != 0) {
        generateTexture3D0(textureParameterRef, samplerParameterRef, frameBufferObjectRef);
        textureRef = m_textures[m_textures.count() - 1];
    }
    else if ((flags & IRenderContext::kTexture2D) != 0) {
        generateTexture2D0(textureParameterRef, samplerParameterRef, frameBufferObjectRef);
        textureRef = m_textures[m_textures.count() - 1];
    }
    m_parameters.append(textureParameterRef);
    if (samplerParameterRef && textureParameterRef) {
        samplerParameterRef->setTexture(textureRef);
    }
}

void RenderColorTargetSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_name2textures.clear();
    m_parameters.clear();
}

const RenderColorTargetSemantic::Texture *RenderColorTargetSemantic::findTexture(const char *name) const
{
    return m_name2textures.find(name);
}

IEffect::IParameter *RenderColorTargetSemantic::findParameter(const char *name) const
{
    IEffect::IParameter *const *ref = m_path2parameterRefs.find(name);
    return ref ? *ref : 0;
}

int RenderColorTargetSemantic::countParameters() const
{
    return m_parameters.count();
}

void RenderColorTargetSemantic::generateTexture2D(IEffect::IParameter *textureParameterRef,
                                                  IEffect::IParameter *samplerParameterRef,
                                                  const Vector3 &size,
                                                  FrameBufferObject *frameBufferObjectRef,
                                                  BaseSurface::Format &format)
{
    Util::getTextureFormat(textureParameterRef, format);
    ITexture *texture = m_textures.append(new Texture2D(format, size, 0));
    texture->create();
    m_name2textures.insert(textureParameterRef->name(), Texture(frameBufferObjectRef, texture, textureParameterRef, samplerParameterRef));
    texture->bind();
    if (MaterialTextureSemantic::hasMipmap(textureParameterRef, samplerParameterRef)) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    texture->unbind();
}

void RenderColorTargetSemantic::generateTexture3D(IEffect::IParameter *textureParamaterRef,
                                                  IEffect::IParameter *samplerParameterRef,
                                                  const Vector3 &size,
                                                  FrameBufferObject *frameBufferObjectRef)
{
    BaseSurface::Format format;
    Util::getTextureFormat(textureParamaterRef, format);
    ITexture *texture = m_textures.append(new Texture3D(format, size, 0));
    texture->create();
    m_name2textures.insert(textureParamaterRef->name(), Texture(frameBufferObjectRef, texture, textureParamaterRef, samplerParameterRef));
    texture->bind();
    if (MaterialTextureSemantic::hasMipmap(textureParamaterRef, samplerParameterRef)) {
        glGenerateMipmap(GL_TEXTURE_3D);
    }
    texture->unbind();
}

void RenderColorTargetSemantic::generateTexture2D0(IEffect::IParameter *textureRef,
                                                   IEffect::IParameter *samplerRef,
                                                   FrameBufferObject *frameBufferObjectRef)
{
    size_t width, height;
    BaseSurface::Format format;
    getSize2(textureRef, width, height);
    generateTexture2D(textureRef, samplerRef, Vector3(Scalar(width), Scalar(height), 0), frameBufferObjectRef, format);
}

void RenderColorTargetSemantic::generateTexture3D0(IEffect::IParameter *textureRef,
                                                   IEffect::IParameter *samplerRef,
                                                   FrameBufferObject *frameBufferObjectRef)
{
    size_t width, height, depth;
    getSize3(textureRef, width, height, depth);
    generateTexture3D(textureRef, samplerRef, Vector3(Scalar(width), Scalar(height), Scalar(depth)), frameBufferObjectRef);
}

void RenderColorTargetSemantic::getSize2(const IEffect::IParameter *parameterRef, size_t &width, size_t &height) const
{
    Vector3 size;
    if (Util::getSize2(parameterRef, size)) {
        width = size_t(size.x());
        height = size_t(size.y());
    }
    else {
        Vector3 viewport;
        m_renderContextRef->getViewport(viewport);
        width = btMax(size_t(1), size_t(viewport.x() * size.x()));
        height = btMax(size_t(1), size_t(viewport.y() * size.y()));
    }
}

void RenderColorTargetSemantic::getSize3(const IEffect::IParameter *parameterRef, size_t &width, size_t &height, size_t &depth) const
{
    Vector3 size;
    if (Util::getSize3(parameterRef, size)) {
        width = size_t(size.x());
        height = size_t(size.y());
        depth = size_t(size.z());
    }
    else {
        Vector3 viewport;
        m_renderContextRef->getViewport(viewport);
        width = btMax(size_t(1), size_t(viewport.x()));
        height = btMax(size_t(1), size_t(viewport.y()));
        depth = 24;
    }
}

ITexture *RenderColorTargetSemantic::lastTextureRef() const
{
    return m_textures[m_textures.count() - 1];
}

/* RenderDepthStencilSemantic */

RenderDepthStencilTargetSemantic::RenderDepthStencilTargetSemantic(IRenderContext *renderContextRef)
    : RenderColorTargetSemantic(renderContextRef)
{
}

RenderDepthStencilTargetSemantic::~RenderDepthStencilTargetSemantic()
{
    invalidateParameter();
}

void RenderDepthStencilTargetSemantic::addParameter(IEffect::IParameter *parameterRef, FrameBufferObject *frameBufferObjectRef)
{
    size_t width, height;
    getSize2(parameterRef, width, height);
    m_parameters.append(parameterRef);
    BaseSurface::Format format(GL_DEPTH_COMPONENT, GL_DEPTH24_STENCIL8, GL_UNSIGNED_BYTE, GL_TEXTURE_2D);
    m_renderBuffers.append(new FrameBufferObject::StandardRenderBuffer(format, Vector3(Scalar(width), Scalar(height), 0)));
    FrameBufferObject::BaseRenderBuffer *renderBuffer = m_renderBuffers[m_renderBuffers.count() - 1];
    renderBuffer->create();
    m_buffers.insert(parameterRef->name(), Buffer(frameBufferObjectRef, renderBuffer, parameterRef));
}

void RenderDepthStencilTargetSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_renderBuffers.clear();
    m_parameters.clear();
    m_buffers.clear();
}

const RenderDepthStencilTargetSemantic::Buffer *RenderDepthStencilTargetSemantic::findDepthStencilBuffer(const char *name) const
{
    return m_buffers.find(name);
}

/* OffscreenRenderTargetSemantic */

OffscreenRenderTargetSemantic::OffscreenRenderTargetSemantic(IRenderContext *renderContextRef)
    : RenderColorTargetSemantic(renderContextRef)
{
}

OffscreenRenderTargetSemantic::~OffscreenRenderTargetSemantic()
{
}

void OffscreenRenderTargetSemantic::generateTexture2D(IEffect::IParameter *textureParameterRef,
                                                      IEffect::IParameter *samplerParameterRef,
                                                      const Vector3 &size,
                                                      FrameBufferObject *frameBufferObjectRef,
                                                      BaseSurface::Format &format)
{
    RenderColorTargetSemantic::generateTexture2D(textureParameterRef, samplerParameterRef, size, frameBufferObjectRef, format);
    ITexture *texture = lastTextureRef();
    if (Effect *effectRef = static_cast<Effect *>(textureParameterRef->parentEffectRef())) {
        effectRef->addOffscreenRenderTarget(texture, textureParameterRef, samplerParameterRef);
    }
}

/* AnimatedTextureSemantic */

AnimatedTextureSemantic::AnimatedTextureSemantic(IRenderContext *renderContextRef)
    : m_renderContextRef(renderContextRef)
{
}

AnimatedTextureSemantic::~AnimatedTextureSemantic()
{
    m_renderContextRef = 0;
}

void AnimatedTextureSemantic::addParameter(IEffect::IParameter *parameterRef)
{
    const IEffect::IAnnotation *annotationRef = parameterRef->annotationRef("ResourceName");
    if (annotationRef && parameterRef->type() == IEffect::IParameter::kTexture) {
        m_parameterRefs.append(parameterRef);
    }
}

void AnimatedTextureSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_parameterRefs.clear();
}

void AnimatedTextureSemantic::update(const RenderColorTargetSemantic &renderColorTarget)
{
    const int nparameters = m_parameterRefs.count();
    for (int i = 0; i < nparameters; i++) {
        IEffect::IParameter *parameter = m_parameterRefs[i];
        const IEffect::IAnnotation *resourceNameAnnotation = parameter->annotationRef("ResourceName");
        float offset = 0, speed = 1, seek = 0;
        if (const IEffect::IAnnotation *annotation = parameter->annotationRef("Offset")) {
            offset = annotation->floatValue();
        }
        if (const IEffect::IAnnotation *annotation = parameter->annotationRef("Speed")) {
            speed = annotation->floatValue();
        }
        if (const IEffect::IAnnotation *annotation = parameter->annotationRef("SeekVariable")) {
            const IEffect *effect = parameter->parentEffectRef();
            IEffect::IParameter *seekParameter = effect->findParameter(annotation->stringValue());
            seekParameter->getValue(seek);
        }
        else {
            m_renderContextRef->getTime(seek, true);
        }
        const char *resourceName = resourceNameAnnotation->stringValue();
        const IEffect::IParameter *textureParameterRef = renderColorTarget.findParameter(resourceName);
        if (const RenderColorTargetSemantic::Texture *t = renderColorTarget.findTexture(textureParameterRef->name())) {
            GLuint textureID = static_cast<GLuint>(t->textureRef->data());
            m_renderContextRef->uploadAnimatedTexture(offset, speed, seek, &textureID);
        }
    }
}

/* TextureValueSemantic */

TextureValueSemantic::TextureValueSemantic()
{
}

TextureValueSemantic::~TextureValueSemantic()
{
}

void TextureValueSemantic::addParameter(IEffect::IParameter *parameterRef)
{
    if (const IEffect::IAnnotation *annotation = parameterRef->annotationRef("TextureName")) {
        int ndimensions = 0;
        parameterRef->getArrayDimension(ndimensions);
        bool isFloat4 = parameterRef->type() == IEffect::IParameter::kFloat4;
        bool isValidDimension = ndimensions == 1 || ndimensions == 2;
        if (isFloat4 && isValidDimension) {
            const char *name = annotation->stringValue();
            const IEffect *effect = parameterRef->parentEffectRef();
            IEffect::IParameter *textureParameterRef = effect->findParameter(name);
            if (textureParameterRef->type() == IEffect::IParameter::kTexture) {
                m_parameterRefs.append(textureParameterRef);
            }
        }
    }
}

void TextureValueSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_parameterRefs.clear();
}

void TextureValueSemantic::update()
{
    const int nparameters = m_parameterRefs.count();
    for (int i = 0; i < nparameters; i++) {
        int size = 0;
        intptr_t texture;
        IEffect::IParameter *parameterRef = m_parameterRefs[i];
        parameterRef->getTextureRef(texture);
        parameterRef->getArrayTotalSize(size);
        if (Vector4 *pixels = new (std::nothrow) Vector4[size]) {
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            parameterRef->setValue(pixels);
            delete[] pixels;
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* SelfShadowObjectSemantic */

SelfShadowSemantic::SelfShadowSemantic()
    : BaseParameter(),
      m_center(0),
      m_size(0),
      m_distance(0),
      m_rate(0)
{
}

SelfShadowSemantic::~SelfShadowSemantic()
{
}

void SelfShadowSemantic::addParameter(IEffect::IParameter *parameterRef)
{
    if (const IEffect::IAnnotation *annotation = parameterRef->annotationRef("name")) {
        const char *name = annotation->stringValue();
        size_t len = strlen(name);
        if (VPVL2_CG_STREQ_CASE_CONST(name, len, "rate")) {
            m_rate = parameterRef;
        }
        if (VPVL2_CG_STREQ_CASE_CONST(name, len, "size")) {
            m_size = parameterRef;
        }
        else if (VPVL2_CG_STREQ_CASE_CONST(name, len, "center")) {
            m_center = parameterRef;
        }
        else if (VPVL2_CG_STREQ_CASE_CONST(name, len, "distance")) {
            m_distance = parameterRef;
        }
    }
}

void SelfShadowSemantic::invalidateParameter()
{
    BaseParameter::invalidateParameter();
    m_rate = 0;
    m_size = 0;
    m_center = 0;
    m_distance = 0;
}

void SelfShadowSemantic::updateParameter(const IShadowMap *shadowMapRef)
{
    if (m_center) {
        m_center->setValue(shadowMapRef->position());
    }
    if (m_distance) {
        m_distance->setValue(shadowMapRef->distance());
    }
    if (m_rate) {
        m_rate->setValue(1); //shadowMapRef->rate());
    }
    if (m_size) {
        m_size->setValue(shadowMapRef->size());
    }
}

/* Effect::RectRenderEngine */
class EffectEngine::RectangleRenderEngine
{
public:
    RectangleRenderEngine()
        : m_vertexBundle(0),
          m_verticesBuffer(0),
          m_indicesBuffer(0)
    {
    }
    ~RectangleRenderEngine() {
        m_bundle.releaseVertexArrayObjects(&m_vertexBundle, 1);
        glDeleteBuffers(1, &m_verticesBuffer);
        glDeleteBuffers(1, &m_indicesBuffer);
    }

    void initializeVertexBundle() {
        glGenBuffers(1, &m_verticesBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_verticesBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
        glGenBuffers(1, &m_indicesBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        m_bundle.allocateVertexArrayObjects(&m_vertexBundle, 1);
        m_bundle.bindVertexArrayObject(m_vertexBundle);
        bindVertexBundle(false);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        m_bundle.unbindVertexArrayObject();
        unbindVertexBundle(false);
    }
    void bindVertexBundle(bool bundle) {
        if (!bundle || !m_bundle.bindVertexArrayObject(m_vertexBundle)) {
            glBindBuffer(GL_ARRAY_BUFFER, m_verticesBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesBuffer);
            glVertexPointer(2, GL_FLOAT, kVertexStride, reinterpret_cast<const GLvoid *>(0));
            glTexCoordPointer(2, GL_FLOAT, kVertexStride, reinterpret_cast<const GLvoid *>(kTextureOffset));
        }
        glDisable(GL_DEPTH_TEST);
    }
    void unbindVertexBundle(bool bundle) {
        if (!bundle || !m_bundle.unbindVertexArrayObject()) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        glEnable(GL_DEPTH_TEST);
    }

private:
    VertexBundleLayout m_bundle;
    GLuint m_vertexBundle;
    GLuint m_verticesBuffer;
    GLuint m_indicesBuffer;
};

/* EffectEngine */
EffectEngine::EffectEngine(Scene *sceneRef, IRenderContext *renderContextRef)
    : world(renderContextRef, IRenderContext::kWorldMatrix),
      view(renderContextRef, IRenderContext::kViewMatrix),
      projection(renderContextRef, IRenderContext::kProjectionMatrix),
      worldView(renderContextRef, IRenderContext::kWorldMatrix | IRenderContext::kViewMatrix),
      viewProjection(renderContextRef, IRenderContext::kViewMatrix | IRenderContext::kProjectionMatrix),
      worldViewProjection(renderContextRef, IRenderContext::kWorldMatrix | IRenderContext::kViewMatrix | IRenderContext::kProjectionMatrix),
      time(renderContextRef),
      elapsedTime(renderContextRef),
      controlObject(sceneRef, renderContextRef),
      renderColorTarget(renderContextRef),
      renderDepthStencilTarget(renderContextRef),
      animatedTexture(renderContextRef),
      offscreenRenderTarget(renderContextRef),
      index(0),
      m_effectRef(0),
      m_defaultStandardEffectRef(0),
      m_renderContextRef(renderContextRef),
      m_rectangleRenderEngine(0),
      m_frameBufferObjectRef(0),
      m_scriptOutput(kColor),
      m_scriptClass(kObject)
{
    /* prepare pre/post effect that uses rectangle (quad) rendering */
    m_rectangleRenderEngine = new RectangleRenderEngine();
    m_rectangleRenderEngine->initializeVertexBundle();
}

EffectEngine::~EffectEngine()
{
    invalidateEffect();
    delete m_rectangleRenderEngine;
    m_rectangleRenderEngine = 0;
    m_defaultTechniques.clear();
    m_defaultStandardEffectRef = 0;
    m_renderContextRef = 0;
}

bool EffectEngine::setEffect(IEffect *effectRef, const IString *dir, bool isDefaultStandardEffect)
{
    VPVL2_LOG(CHECK(effectRef));
    m_effectRef = static_cast<Effect *>(effectRef);
    IEffect::IParameter *standardsGlobal = 0;
    FrameBufferObject *frameBufferObjectRef = m_frameBufferObjectRef = m_effectRef->parentFrameBufferObject();
    Array<IEffect::IParameter *> parameters;
    m_effectRef->getParameterRefs(parameters);
    const int nparameters = parameters.count();
    for (int i = 0; i < nparameters; i++) {
        IEffect::IParameter *parameterRef = parameters[i];
        const char *semantic = parameterRef->semantic();
        const size_t slen = strlen(semantic);
        if (VPVL2_CG_STREQ_CONST(semantic, slen, "VIEWPORTPIXELSIZE")) {
            viewportPixelSize.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_SUFFIX(semantic, slen, kWorldViewProjectionSemantic)) {
            worldViewProjection.addParameter(parameterRef, VPVL2_CG_GET_SUFFIX(semantic, kWorldViewProjectionSemantic));
        }
        else if (VPVL2_CG_STREQ_SUFFIX(semantic, slen, kWorldViewSemantic)) {
            worldView.addParameter(parameterRef, VPVL2_CG_GET_SUFFIX(semantic, kWorldViewSemantic));
        }
        else if (VPVL2_CG_STREQ_SUFFIX(semantic, slen, kViewProjectionSemantic)) {
            viewProjection.addParameter(parameterRef, VPVL2_CG_GET_SUFFIX(semantic, kViewProjectionSemantic));
        }
        else if (VPVL2_CG_STREQ_SUFFIX(semantic, slen, kWorldSemantic)) {
            world.addParameter(parameterRef, VPVL2_CG_GET_SUFFIX(semantic, kWorldSemantic));
        }
        else if (VPVL2_CG_STREQ_SUFFIX(semantic, slen, kViewSemantic)) {
            view.addParameter(parameterRef, VPVL2_CG_GET_SUFFIX(semantic, kViewSemantic));
        }
        else if (VPVL2_CG_STREQ_SUFFIX(semantic, slen, kProjectionSemantic)) {
            projection.addParameter(parameterRef, VPVL2_CG_GET_SUFFIX(semantic, kProjectionSemantic));
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "DIFFUSE")) {
            diffuse.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "AMBIENT")) {
            ambient.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "EMISSIVE")) {
            emissive.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "SPECULARPOWER")) {
            specularPower.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "SPECULAR")) {
            specular.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "TOONCOLOR")) {
            toonColor.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "EDGECOLOR")) {
            edgeColor.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "EDGEWIDTH")) {
            edgeWidth.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "ADDINGTEXTURE")) {
            addingTexture.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "ADDINGSPHERE")) {
            addingSphere.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "MULTIPLYINGTEXTURE")) {
            multiplyTexture.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "MULTIPLYINGSPHERE")) {
            multiplySphere.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "_POSITION")) {
            position.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "_DIRECTION")) {
            direction.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "TIME")) {
            time.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "ELAPSEDTIME")) {
            elapsedTime.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "MOUSEPOSITION")) {
            mousePosition.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "LEFTMOUSEDOWN")) {
            leftMouseDown.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "MIDDLEMOUSEDOWN")) {
            middleMouseDown.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "RIGHTMOUSEDOWN")) {
            rightMouseDown.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "CONTROLOBJECT")) {
            controlObject.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "ANIMATEDTEXTURE")) {
            animatedTexture.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "TEXTUREVALUE")) {
            textureValue.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "RENDERDEPTHSTENCILTARGET")) {
            renderDepthStencilTarget.addParameter(parameterRef, frameBufferObjectRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "SELFSHADOWVPVM")) {
            selfShadow.addParameter(parameterRef);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "SHAREDRENDERCOLORTARGETVPVM")) {
            addSharedTextureParameter(parameterRef, frameBufferObjectRef, renderColorTarget);
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "SHAREDOFFSCREENRENDERTARGETVPVM")) {
            addSharedTextureParameter(parameterRef, frameBufferObjectRef, offscreenRenderTarget);
        }
        else if (!standardsGlobal && VPVL2_CG_STREQ_CONST(semantic, slen, "STANDARDSGLOBAL")) {
            standardsGlobal = parameterRef;
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "_INDEX")) {
        }
        else if (VPVL2_CG_STREQ_CONST(semantic, slen, "TEXUNIT0")) {
            depthTexture.addParameter(parameterRef);
        }
        else {
            const char *name = parameterRef->name();
            const size_t nlen = strlen(name);
            if (VPVL2_CG_STREQ_CONST(name, nlen, "parthf")) {
                parthf.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "spadd")) {
                spadd.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "transp")) {
                transp.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "use_texture")) {
                useTexture.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "use_spheremap")) {
                useSpheremap.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "use_toon")) {
                useToon.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "opadd")) {
                opadd.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "VertexCount")) {
                vertexCount.addParameter(parameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(name, nlen, "SubsetCount")) {
                subsetCount.addParameter(parameterRef);
            }
            else {
                const IEffect::IParameter::Type parameterType = parameterRef->type();
                if (parameterType == IEffect::IParameter::kSampler2D ||
                        parameterType == IEffect::IParameter::kSampler3D ||
                        parameterType == IEffect::IParameter::kSamplerCube) {
                    parseSamplerStateParameter(parameterRef, frameBufferObjectRef, dir);
                }
            }
        }
        m_effectRef->addInteractiveParameter(parameterRef);
    }
    /*
     * parse STANDARDSGLOBAL semantic parameter at last to resolve parameters in
     * script process dependencies correctly
     */
    bool ownTechniques = false;
    if (standardsGlobal) {
        setStandardsGlobal(standardsGlobal, ownTechniques);
    }
    if (isDefaultStandardEffect) {
        setDefaultStandardEffectRef(effectRef);
    }
    else if (!ownTechniques) {
        Array<IEffect::ITechnique *> techniques;
        effectRef->getTechniqueRefs(techniques);
        const int ntechniques = techniques.count();
        for (int i = 0; i < ntechniques; i++) {
            IEffect::ITechnique *technique = techniques[i];
            addTechniquePasses(technique);
        }
    }
    return true;
}

void EffectEngine::invalidateEffect()
{
    viewportPixelSize.invalidateParameter();
    worldViewProjection.invalidateParameter();
    worldView.invalidateParameter();
    viewProjection.invalidateParameter();
    world.invalidateParameter();
    view.invalidateParameter();
    projection.invalidateParameter();
    diffuse.invalidateParameter();
    ambient.invalidateParameter();
    emissive.invalidateParameter();
    specularPower.invalidateParameter();
    specular.invalidateParameter();
    toonColor.invalidateParameter();
    edgeColor.invalidateParameter();
    edgeWidth.invalidateParameter();
    addingTexture.invalidateParameter();
    addingSphere.invalidateParameter();
    multiplyTexture.invalidateParameter();
    multiplySphere.invalidateParameter();
    position.invalidateParameter();
    direction.invalidateParameter();
    time.invalidateParameter();
    elapsedTime.invalidateParameter();
    mousePosition.invalidateParameter();
    leftMouseDown.invalidateParameter();
    middleMouseDown.invalidateParameter();
    rightMouseDown.invalidateParameter();
    controlObject.invalidateParameter();
    animatedTexture.invalidateParameter();
    textureValue.invalidateParameter();
    renderDepthStencilTarget.invalidateParameter();
    selfShadow.invalidateParameter();
    parthf.invalidateParameter();
    spadd.invalidateParameter();
    transp.invalidateParameter();
    useTexture.invalidateParameter();
    useSpheremap.invalidateParameter();
    useToon.invalidateParameter();
    opadd.invalidateParameter();
    vertexCount.invalidateParameter();
    subsetCount.invalidateParameter();
    m_target2BufferRefs.clear();
    m_target2TextureRefs.clear();
    m_passScripts.clear();
    m_techniquePasses.clear();
    m_techniques.clear();
    m_techniqueScripts.clear();
    m_frameBufferObjectRef = 0;
    m_effectRef = 0;
}

IEffect::ITechnique *EffectEngine::findTechnique(const char *pass,
                                                 int offset,
                                                 int nmaterials,
                                                 bool hasTexture,
                                                 bool hasSphereMap,
                                                 bool useToon) const
{
    IEffect::ITechnique *technique = 0;
    const int ntechniques = m_techniques.count();
    for (int i = 0; i < ntechniques; i++) {
        IEffect::ITechnique *t = m_techniques[i];
        if (testTechnique(t, pass, offset, nmaterials, hasTexture, hasSphereMap, useToon)) {
            technique = t;
            break;
        }
    }
    if (!technique) {
        const int ntechniques2 = m_defaultTechniques.count();
        for (int i = 0; i < ntechniques2; i++) {
            IEffect::ITechnique *t = m_defaultTechniques[i];
            if (testTechnique(t, pass, offset, nmaterials, hasTexture, hasSphereMap, useToon)) {
                technique = t;
                break;
            }
        }
    }
    return technique;
}

void EffectEngine::executeScriptExternal()
{
    if (scriptOrder() == IEffect::kPostProcess) {
        bool isPassExecuted; /* unused and ignored */
        DrawPrimitiveCommand command;
        executeScript(&m_externalScript, command, 0, isPassExecuted);
    }
}

bool EffectEngine::hasTechniques(IEffect::ScriptOrderType order) const
{
    return scriptOrder() == order ? m_techniqueScripts.size() > 0 : false;
}

void EffectEngine::executeProcess(const IModel *model,
                                  IEffect *nextPostEffectRef,
                                  IEffect::ScriptOrderType order)
{
    if (!m_effectRef || scriptOrder() != order) {
        return;
    }
    setZeroGeometryParameters(model);
    diffuse.setGeometryColor(Color(0, 0, 0, model ? model->opacity() : 0)); /* for asset opacity */
    const IEffect::ITechnique *technique = findTechnique("object", 0, 0, false, false, false);
    executeTechniquePasses(technique, kQuadDrawCommand, nextPostEffectRef);
}

void EffectEngine::executeTechniquePasses(const IEffect::ITechnique *technique,
                                          const DrawPrimitiveCommand &command,
                                          IEffect *nextPostEffectRef)
{
    if (technique) {
        const Script *tss = m_techniqueScripts.find(technique);
        bool isPassExecuted;
        executeScript(tss, command, nextPostEffectRef, isPassExecuted);
        if (!isPassExecuted) {
            if (const Passes *passes = m_techniquePasses.find(technique)) {
                const int npasses = passes->size();
                for (int i = 0; i < npasses; i++) {
                    IEffect::IPass *pass = passes->at(i);
                    const Script *pss = m_passScripts.find(pass);
                    executeScript(pss, command, nextPostEffectRef, isPassExecuted);
                }
            }
        }
    }
}

void EffectEngine::setModelMatrixParameters(const IModel *model,
                                            int extraCameraFlags,
                                            int extraLightFlags)
{
    world.setMatrices(model, extraCameraFlags, extraLightFlags);
    view.setMatrices(model, extraCameraFlags, extraLightFlags);
    projection.setMatrices(model, extraCameraFlags, extraLightFlags);
    worldView.setMatrices(model, extraCameraFlags, extraLightFlags);
    viewProjection.setMatrices(model, extraCameraFlags, extraLightFlags);
    worldViewProjection.setMatrices(model, extraCameraFlags, extraLightFlags);
}

void EffectEngine::setDefaultStandardEffectRef(IEffect *effectRef)
{
    if (!m_defaultStandardEffectRef) {
        Array<IEffect::ITechnique *> techniques;
        effectRef->getTechniqueRefs(techniques);
        const int ntechniques = techniques.count();
        Passes passes;
        for (int i = 0; i < ntechniques; i++) {
            IEffect::ITechnique *technique = techniques[i];
            if (parseTechniqueScript(technique, passes)) {
                m_techniquePasses.insert(technique, passes);
                m_defaultTechniques.append(technique);
            }
            passes.clear();
        }
        m_defaultStandardEffectRef = effectRef;
    }
}

void EffectEngine::setZeroGeometryParameters(const IModel *model)
{
    edgeColor.setGeometryColor(model ? model->edgeColor() : kZeroV3);
    toonColor.setGeometryColor(kZeroC);
    ambient.setGeometryColor(kZeroC);
    diffuse.setGeometryColor(kZeroC);
    emissive.setGeometryColor(kZeroC);
    specular.setGeometryColor(kZeroC);
    specularPower.setGeometryValue(0);
    spadd.setValue(false);
    useTexture.setValue(false);
    useSpheremap.setValue(false);
}

void EffectEngine::updateModelGeometryParameters(const Scene *scene, const IModel *model)
{
    const ILight *light = scene->light();
    const Vector3 &lightColor = light->color();
    if (model && model->type() == IModel::kAssetModel) {
        const Vector3 &ac = Vector3(0.7f, 0.7f, 0.7f) - lightColor;
        ambient.setLightColor(Color(ac.x(), ac.y(), ac.z(), 1));
        diffuse.setLightColor(Color(1, 1, 1, 1));
        specular.setLightColor(lightColor);
    }
    else {
        ambient.setLightColor(lightColor);
        diffuse.setLightColor(kZeroC);
        specular.setLightColor(lightColor);
    }
    emissive.setLightColor(kZeroC);
    edgeColor.setLightColor(kZeroC);
    const Vector3 &lightDirection = light->direction();
    position.setLightValue(-lightDirection);
    direction.setLightValue(lightDirection.normalized());
    const ICamera *camera = scene->camera();
    const Vector3 &cameraPosition = camera->modelViewTransform().getOrigin();
    position.setCameraValue(cameraPosition);
    direction.setCameraValue((cameraPosition - camera->lookAt()).normalized());
    controlObject.update(model);
}

void EffectEngine::updateSceneParameters()
{
    Vector3 viewport;
    m_renderContextRef->getViewport(viewport);
    viewportPixelSize.setValue(viewport);
    Vector4 position;
    m_renderContextRef->getMousePosition(position, IRenderContext::kMouseCursorPosition);
    mousePosition.setValue(position);
    m_renderContextRef->getMousePosition(position, IRenderContext::kMouseLeftPressPosition);
    leftMouseDown.setValue(position);
    m_renderContextRef->getMousePosition(position, IRenderContext::kMouseMiddlePressPosition);
    middleMouseDown.setValue(position);
    m_renderContextRef->getMousePosition(position, IRenderContext::kMouseRightPressPosition);
    rightMouseDown.setValue(position);
    time.update();
    elapsedTime.update();
    textureValue.update();
    animatedTexture.update(renderColorTarget);
}

bool EffectEngine::isStandardEffect() const
{
    return scriptOrder() == IEffect::kStandard;
}

const EffectEngine::Script *EffectEngine::findTechniqueScript(const CGtechnique technique) const
{
    return m_techniqueScripts.find(technique);
}

const EffectEngine::Script *EffectEngine::findPassScript(const CGpass pass) const
{
    return m_passScripts.find(pass);
}

bool EffectEngine::testTechnique(const IEffect::ITechnique *technique,
                                 const char *pass,
                                 int offset,
                                 int nmaterials,
                                 bool hasTexture,
                                 bool hasSphereMap,
                                 bool useToon)
{
    if (technique) {
        int ok = 1;
        ok &= Util::isPassEquals(technique->annotationRef("MMDPass"), pass) ? 1 : 0;
        ok &= containsSubset(technique->annotationRef("Subset"), offset, nmaterials) ? 1 : 0;
        if (const IEffect::IAnnotation *annotation = technique->annotationRef("UseTexture")) {
            ok &= annotation->booleanValue() == hasTexture ? 1 : 0;
        }
        if (const IEffect::IAnnotation *annotation = technique->annotationRef("UseSphereMap")) {
            ok &= annotation->booleanValue() == hasSphereMap ? 1 : 0;
        }
        if (const IEffect::IAnnotation *annotation = technique->annotationRef("UseToon")) {
            ok &= annotation->booleanValue() == useToon ? 1 : 0;
        }
        return ok == 1;
    }
    return false;
}

bool EffectEngine::containsSubset(const IEffect::IAnnotation *annotation, int subset, int nmaterials)
{
    if (annotation) {
        const std::string s(annotation->stringValue());
        std::istringstream stream(s);
        std::string segment;
        while (std::getline(stream, segment, ',')) {
            if (strtol(segment.c_str(), 0, 10) == subset) {
                return true;
            }
            std::string::size_type offset = segment.find("-");
            if (offset != std::string::npos) {
                int from = strtol(segment.substr(0, offset).c_str(), 0, 10);
                int to = strtol(segment.substr(offset + 1).c_str(), 0, 10);
                if (to == 0) {
                    to = nmaterials;
                }
                if (from > to) {
                    std::swap(from, to);
                }
                if (from <= subset && subset <= to) {
                    return true;
                }
            }
        }
        return false;
    }
    return true;
}

void EffectEngine::setScriptStateFromRenderColorTargetSemantic(const RenderColorTargetSemantic &semantic,
                                                               const std::string &value,
                                                               ScriptState::Type type,
                                                               ScriptState &state)
{
    bool bound = false;
    state.type = type;
    if (!value.empty()) {
        if (const RenderColorTargetSemantic::Texture *texture = semantic.findTexture(value.c_str())) {
            state.renderColorTargetTextureRef = texture;
            m_target2TextureRefs.insert(type, texture);
            bound = true;
        }
    }
    else if (const RenderColorTargetSemantic::Texture *const *texturePtr = m_target2TextureRefs.find(type)) {
        const RenderColorTargetSemantic::Texture *texture = *texturePtr;
        state.renderColorTargetTextureRef = texture;
        m_target2TextureRefs.remove(type);
    }
    state.isRenderTargetBound = bound;
}

void EffectEngine::setScriptStateFromRenderDepthStencilTargetSemantic(const RenderDepthStencilTargetSemantic &semantic,
                                                                      const std::string &value,
                                                                      ScriptState::Type type,
                                                                      ScriptState &state)
{
    bool bound = false;
    state.type = type;
    if (!value.empty()) {
        if (const RenderDepthStencilTargetSemantic::Buffer *buffer = semantic.findDepthStencilBuffer(value.c_str())) {
            state.renderDepthStencilBufferRef = buffer;
            m_target2BufferRefs.insert(type, buffer);
            bound = true;
        }
    }
    else if (const RenderDepthStencilTargetSemantic::Buffer *const *bufferPtr = m_target2BufferRefs.find(type)) {
        const RenderDepthStencilTargetSemantic::Buffer *buffer = *bufferPtr;
        state.renderDepthStencilBufferRef = buffer;
        m_target2BufferRefs.remove(type);
    }
    state.isRenderTargetBound = bound;
}

void EffectEngine::setScriptStateFromParameter(const IEffect *effectRef,
                                               const std::string &value,
                                               IEffect::IParameter::Type testType,
                                               ScriptState::Type type,
                                               ScriptState &state)
{
    IEffect::IParameter *parameter = effectRef->findParameter(value.c_str());
    if (parameter && parameter->type() == testType) {
        state.type = type;
        state.parameter = parameter;
    }
}

void EffectEngine::executePass(IEffect::IPass *pass, const DrawPrimitiveCommand &command) const
{
    if (pass) {
        pass->setState();
        drawPrimitives(command);
        pass->resetState();
    }
}

void EffectEngine::setRenderColorTargetFromScriptState(const ScriptState &state, IEffect *nextPostEffectRef)
{
    if (const RenderColorTargetSemantic::Texture *textureRef = state.renderColorTargetTextureRef) {
        const int index = state.type - ScriptState::kRenderColorTarget0, targetIndex = GL_COLOR_ATTACHMENT0 + index;
        if (FrameBufferObject *fbo = textureRef->frameBufferObjectRef) {
            Vector3 viewport;
            m_renderContextRef->getViewport(viewport);
            if (state.isRenderTargetBound) {
                ITexture *tref = textureRef->textureRef;
                if (m_effectRef->hasRenderColorTargetIndex(targetIndex)) {
                    /* The render color target is not bound yet  */
                    fbo->resize(viewport, index);
                    fbo->bindTexture(tref, index);
                    m_effectRef->addRenderColorTargetIndex(targetIndex);
                }
                else {
                    /* change current color attachment to the specified texture */
                    fbo->readMSAABuffer(index);
                    fbo->resize(viewport, index);
                    fbo->bindTexture(tref, index);
                }
                const Vector3 &size = tref->size();
                glViewport(0, 0, GLsizei(size.x()), GLsizei(size.y()));
            }
            else if (Effect *nextPostEffect = static_cast<Effect *>(nextPostEffectRef)) {
                FrameBufferObject *nextFrameBufferObject = nextPostEffect->parentFrameBufferObject();
                m_frameBufferObjectRef->transferTo(nextFrameBufferObject, m_effectRef->renderColorTargetIndices());
                nextPostEffect->inheritRenderColorTargetIndices(m_effectRef);
            }
            else  {
                /* final color output */
                if (index > 0) {
                    fbo->readMSAABuffer(index);
                    m_effectRef->removeRenderColorTargetIndex(targetIndex);
                }
                else {
                    /* reset to the default window framebuffer */
                    m_frameBufferObjectRef->transferToWindow(m_effectRef->renderColorTargetIndices(), viewport);
                    m_effectRef->clearRenderColorTargetIndices();
                    glViewport(0, 0, GLsizei(viewport.x()), GLsizei(viewport.y()));
                }
            }
        }
    }
}

void EffectEngine::setRenderDepthStencilTargetFromScriptState(const ScriptState &state, const IEffect *nextPostEffectRef)
{
    if (const RenderDepthStencilTargetSemantic::Buffer *bufferRef = state.renderDepthStencilBufferRef) {
        if (FrameBufferObject *fbo = bufferRef->frameBufferObjectRef) {
            if (state.isRenderTargetBound) {
                Vector3 viewport;
                m_renderContextRef->getViewport(viewport);
                FrameBufferObject::BaseRenderBuffer *renderBuffer = bufferRef->renderBufferRef;
                renderBuffer->resize(viewport);
                fbo->bindDepthStencilBuffer(renderBuffer);
            }
            else if (!nextPostEffectRef && m_effectRef->renderColorTargetIndices().size() > 0) {
                fbo->unbindDepthStencilBuffer();
            }
        }
    }
}

void EffectEngine::executeScript(const Script *script,
                                 const DrawPrimitiveCommand &command,
                                 IEffect *nextPostEffectRef,
                                 bool &isPassExecuted)
{
    isPassExecuted = scriptOrder() == IEffect::kPostProcess;
    if (script) {
        const int nstates = script->size();
        int stateIndex = 0, nloop = 0, currentIndex = 0, backStateIndex = 0;
        Vector4 v4;
        Vector3 viewport;
        m_renderContextRef->getViewport(viewport);
        while (stateIndex < nstates) {
            const ScriptState &state = script->at(stateIndex);
            switch (state.type) {
            case ScriptState::kClearColor:
                glClear(GL_COLOR_BUFFER_BIT);
                break;
            case ScriptState::kClearDepth:
                glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                break;
            case ScriptState::kClearSetColor:
                if (IEffect::IParameter *parameter = state.parameter) {
                    parameter->setValue(v4);
                    glClearColor(v4.x(), v4.y(), v4.z(), v4.w());
                }
                break;
            case ScriptState::kClearSetDepth:
                if (IEffect::IParameter *parameter = state.parameter) {
                    parameter->setValue(v4);
                    glClearDepth(v4.x());
                }
                break;
            case ScriptState::kLoopByCount:
                if (IEffect::IParameter *parameter = state.parameter) {
                    float value;
                    parameter->getValue(value);
                    backStateIndex = stateIndex + 1;
                    currentIndex = 0;
                    nloop = int(value);
                }
                break;
            case ScriptState::kLoopEnd:
                if (--nloop >= 0) {
                    stateIndex = backStateIndex;
                    ++currentIndex;
                    continue;
                }
                break;
            case ScriptState::kLoopGetIndex:
                if (IEffect::IParameter *parameter = state.parameter) {
                    parameter->setValue(currentIndex);
                }
                break;
            case ScriptState::kRenderColorTarget0:
            case ScriptState::kRenderColorTarget1:
            case ScriptState::kRenderColorTarget2:
            case ScriptState::kRenderColorTarget3:
                setRenderColorTargetFromScriptState(state, nextPostEffectRef);
                break;
            case ScriptState::kRenderDepthStencilTarget:
                setRenderDepthStencilTargetFromScriptState(state, nextPostEffectRef);
                break;
            case ScriptState::kDrawBuffer:
                if (m_scriptClass != kObject) {
                    m_rectangleRenderEngine->bindVertexBundle(true);
                    executePass(state.pass, kQuadDrawCommand);
                    m_rectangleRenderEngine->unbindVertexBundle(true);
                    rebindVertexBundle();
                }
                break;
            case ScriptState::kDrawGeometry:
                if (m_scriptClass != kScene) {
                    executePass(state.pass, command);
                }
                break;
            case ScriptState::kPass:
                executeScript(m_passScripts.find(state.pass), command, nextPostEffectRef, isPassExecuted);
                isPassExecuted = true;
                break;
            case ScriptState::kScriptExternal:
            case ScriptState::kUnknown:
            default:
                break;
            }
            stateIndex++;
        }
    }
}

void EffectEngine::addTechniquePasses(IEffect::ITechnique *technique)
{
    Passes passes;
    if (parseTechniqueScript(technique, passes)) {
        m_techniquePasses.insert(technique, passes);
        m_techniques.append(technique);
    }
}

void EffectEngine::setStandardsGlobal(const IEffect::IParameter *parameterRef, bool &ownTechniques)
{
    float version;
    parameterRef->getValue(version);
    if (!btFuzzyZero(version - 0.8f)) {
        return;
    }
    if (const IEffect::IAnnotation *annotation = parameterRef->annotationRef("ScriptClass")) {
        const char *value = annotation->stringValue();
        const size_t len = strlen(value);
        if (VPVL2_CG_STREQ_CONST(value, len, "object")) {
            m_scriptClass = kObject;
        }
        else if (VPVL2_CG_STREQ_CONST(value, len, "scene")) {
            m_scriptClass = kScene;
        }
        else if (VPVL2_CG_STREQ_CONST(value, len, "sceneorobject")) {
            m_scriptClass = kSceneOrObject;
        }
    }
    if (const IEffect::IAnnotation *annotation = parameterRef->annotationRef("ScriptOrder")) {
        const char *value = annotation->stringValue();
        const size_t len = strlen(value);
        if (VPVL2_CG_STREQ_CONST(value, len, "standard")) {
            m_effectRef->setScriptOrderType(IEffect::kStandard);
        }
        else if (VPVL2_CG_STREQ_CONST(value, len, "preprocess")) {
            m_effectRef->setScriptOrderType(IEffect::kPreProcess);
        }
        else if (VPVL2_CG_STREQ_CONST(value, len, "postprocess")) {
            m_effectRef->setScriptOrderType(IEffect::kPostProcess);
        }
    }
    if (const IEffect::IAnnotation *annotation = parameterRef->annotationRef("Script")) {
        const char *value = annotation->stringValue();
        const size_t len = strlen(value);
        m_techniques.clear();
        if (VPVL2_CG_STREQ_SUFFIX(value, len, kMultipleTechniquesPrefix)) {
            const std::string &s = Util::trimLastSemicolon(VPVL2_CG_GET_SUFFIX(value, kMultipleTechniquesPrefix));
            std::istringstream stream(s);
            std::string segment;
            while (std::getline(stream, segment, ':')) {
                IEffect::ITechnique *technique = m_effectRef->findTechnique(segment.c_str());
                addTechniquePasses(technique);
            }
            ownTechniques = true;
        }
        else if (VPVL2_CG_STREQ_SUFFIX(value, len, kSingleTechniquePrefix)) {
            const std::string &s = Util::trimLastSemicolon(VPVL2_CG_GET_SUFFIX(value, kSingleTechniquePrefix));
            IEffect::ITechnique *technique = parameterRef->parentEffectRef()->findTechnique(s.c_str());
            addTechniquePasses(technique);
            ownTechniques = true;
        }
    }
}

void EffectEngine::parseSamplerStateParameter(IEffect::IParameter *samplerParameterRef,
                                              FrameBufferObject *frameBufferObjectRef,
                                              const IString *dir)
{
    Array<IEffect::ISamplerState *> states;
    samplerParameterRef->getSamplerStateRefs(states);
    const int nstates = states.count();
    for (int i = 0; i < nstates; i++) {
        const IEffect::ISamplerState *samplerState = states[i];
        if (samplerState->type() == IEffect::IParameter::kTexture) {
            IEffect::IParameter *textureParameterRef = samplerState->parameterRef();
            const char *semantic = textureParameterRef->semantic();
            const size_t len = strlen(semantic);
            if (VPVL2_CG_STREQ_CONST(semantic, len, "MATERIALTEXTURE")) {
                materialTexture.addParameter(textureParameterRef, samplerParameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(semantic, len, "MATERIALSPHEREMAP")) {
                materialSphereMap.addParameter(textureParameterRef, samplerParameterRef);
            }
            else if (VPVL2_CG_STREQ_CONST(semantic, len, "RENDERCOLORTARGET")) {
                renderColorTarget.addParameter(textureParameterRef,
                                               samplerParameterRef,
                                               frameBufferObjectRef,
                                               0, false, false);
            }
            else if (VPVL2_CG_STREQ_CONST(semantic, len, "OFFSCREENRENDERTARGET")) {
                offscreenRenderTarget.addParameter(textureParameterRef,
                                                   samplerParameterRef,
                                                   frameBufferObjectRef,
                                                   0, false, false);
            }
            else {
                renderColorTarget.addParameter(textureParameterRef,
                                               samplerParameterRef,
                                               frameBufferObjectRef,
                                               dir, true, true);
            }
            break;
        }
    }
}

void EffectEngine::addSharedTextureParameter(IEffect::IParameter *textureParameterRef,
                                             FrameBufferObject *frameBufferObjectRef,
                                             RenderColorTargetSemantic &semantic)
{
    if (textureParameterRef) {
        const char *name = textureParameterRef->name();
        IRenderContext::SharedTextureParameter sharedTextureParameter(textureParameterRef);
        if (!m_renderContextRef->tryGetSharedTextureParameter(name, sharedTextureParameter)) {
            sharedTextureParameter.parameterRef = textureParameterRef;
            semantic.addParameter(textureParameterRef, 0, frameBufferObjectRef, 0, false, false);
            if (const RenderColorTargetSemantic::Texture *texture = semantic.findTexture(name)) {
                /* parse semantic first and add shared parameter not to fetch unparsed semantic parameter at RenderColorTarget#addParameter */
                sharedTextureParameter.textureRef = texture->textureRef;
                m_renderContextRef->addSharedTextureParameter(name, sharedTextureParameter);
            }
        }
    }
}

bool EffectEngine::parsePassScript(IEffect::IPass *pass)
{
    if (m_passScripts[pass]) {
        return true;
    }
    if (pass) {
        Script passScriptStates;
        if (const IEffect::IAnnotation *annotation = pass->annotationRef("Script")) {
            const std::string s(annotation->stringValue());
            ScriptState lastState, newState;
            std::istringstream stream(s);
            std::string segment;
            bool renderColorTarget0DidSet = false,
                    useRenderBuffer = false,
                    useDepthStencilBuffer = false;
            while (std::getline(stream, segment, ';')) {
                std::string::size_type offset = segment.find("=");
                if (offset != std::string::npos) {
                    const std::string &command = Util::trim(segment.substr(0, offset));
                    const std::string &value = Util::trim(segment.substr(offset + 1));
                    newState.setFromState(lastState);
                    if (command == "RenderColorTarget" || command == "RenderColorTarget0") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget0,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                        renderColorTarget0DidSet = true;
                    }
                    else if (renderColorTarget0DidSet && command == "RenderColorTarget1") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget1,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                    }
                    else if (renderColorTarget0DidSet && command == "RenderColorTarget2") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget2,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                    }
                    else if (renderColorTarget0DidSet && command == "RenderColorTarget3") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget3,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                    }
                    else if (command == "RenderDepthStencilTarget") {
                        setScriptStateFromRenderDepthStencilTargetSemantic(renderDepthStencilTarget,
                                                                           value,
                                                                           ScriptState::kRenderDepthStencilTarget,
                                                                           newState);
                        useDepthStencilBuffer = newState.isRenderTargetBound;
                    }
                    else if (command == "ClearSetColor") {
                        setScriptStateFromParameter(m_effectRef, value, IEffect::IParameter::kFloat4, ScriptState::kClearSetColor, newState);
                    }
                    else if (command == "ClearSetDepth") {
                        setScriptStateFromParameter(m_effectRef, value, IEffect::IParameter::kFloat, ScriptState::kClearSetDepth, newState);
                    }
                    else if (command == "Clear") {
                        if (value == "Color" && useRenderBuffer) {
                            newState.type = ScriptState::kClearColor;
                        }
                        else if (value == "Depth" && useDepthStencilBuffer) {
                            newState.type = ScriptState::kClearDepth;
                        }
                    }
                    else if (command == "Draw") {
                        if (value == "Buffer") {
                            if (m_scriptClass == kObject)
                                return false;
                            newState.type = ScriptState::kDrawBuffer;
                        }
                        if (value == "Geometry") {
                            if (m_scriptClass == kScene)
                                return false;
                            newState.type = ScriptState::kDrawGeometry;
                        }
                        newState.pass = pass;
                    }
                    if (newState.type != ScriptState::kUnknown) {
                        lastState = newState;
                        passScriptStates.push_back(newState);
                        newState.reset();
                    }
                }
            }
        }
        else {
            /* just draw geometry primitives */
            if (m_scriptClass == kScene) {
                return false;
            }
            ScriptState state;
            state.pass = pass;
            state.type = ScriptState::kDrawGeometry;
            passScriptStates.push_back(state);
        }
        m_passScripts.insert(pass, passScriptStates);
        return true;
    }
    return false;
}

bool EffectEngine::parseTechniqueScript(const IEffect::ITechnique *technique, Passes &passes)
{
    /* just check only it's technique object for technique without pass */
    if (technique) {
        Script techniqueScriptStates;
        if (const IEffect::IAnnotation *annotation = technique->annotationRef("Script")) {
            const std::string s(annotation->stringValue());
            std::istringstream stream(s);
            std::string segment;
            Script scriptExternalStates;
            ScriptState lastState, newState;
            bool useScriptExternal = scriptOrder() == IEffect::kPostProcess,
                    renderColorTarget0DidSet = false,
                    renderDepthStencilTargetDidSet = false,
                    useRenderBuffer = false,
                    useDepthStencilBuffer = false;
            while (std::getline(stream, segment, ';')) {
                std::string::size_type offset = segment.find("=");
                if (offset != std::string::npos) {
                    const std::string &command = Util::trim(segment.substr(0, offset));
                    const std::string &value = Util::trim(segment.substr(offset + 1));
                    newState.setFromState(lastState);
                    if (command == "RenderColorTarget" || command == "RenderColorTarget0") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget0,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                        renderColorTarget0DidSet = true;
                    }
                    else if (renderColorTarget0DidSet && command == "RenderColorTarget1") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget1,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                    }
                    else if (renderColorTarget0DidSet && command == "RenderColorTarget2") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget2,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                    }
                    else if (renderColorTarget0DidSet && command == "RenderColorTarget3") {
                        setScriptStateFromRenderColorTargetSemantic(renderColorTarget,
                                                                    value,
                                                                    ScriptState::kRenderColorTarget3,
                                                                    newState);
                        useRenderBuffer = newState.isRenderTargetBound;
                    }
                    else if (command == "RenderDepthStencilTarget") {
                        setScriptStateFromRenderDepthStencilTargetSemantic(renderDepthStencilTarget,
                                                                           value,
                                                                           ScriptState::kRenderDepthStencilTarget,
                                                                           newState);
                        useDepthStencilBuffer = newState.isRenderTargetBound;
                        renderDepthStencilTargetDidSet = true;
                    }
                    else if (command == "ClearSetColor") {
                        setScriptStateFromParameter(m_effectRef, value, IEffect::IParameter::kFloat4, ScriptState::kClearSetColor, newState);
                    }
                    else if (command == "ClearSetDepth") {
                        setScriptStateFromParameter(m_effectRef, value, IEffect::IParameter::kFloat, ScriptState::kClearSetDepth, newState);
                    }
                    else if (command == "Clear") {
                        if (value == "Color" && useRenderBuffer) {
                            newState.type = ScriptState::kClearColor;
                        }
                        else if (value == "Depth" && useDepthStencilBuffer) {
                            newState.type = ScriptState::kClearDepth;
                        }
                    }
                    else if (command == "Pass") {
                        IEffect::IPass *pass = technique->findPass(value.c_str());
                        if (parsePassScript(pass)) {
                            newState.type = ScriptState::kPass;
                            newState.pass = pass;
                            passes.push_back(pass);
                        }
                    }
                    else if (!lastState.enterLoop && command == "LoopByCount") {
                        IEffect::IParameter *parameter = m_effectRef->findParameter(value.c_str());
                        if (Util::isIntegerParameter(parameter)) {
                            newState.type = ScriptState::kLoopByCount;
                            newState.enterLoop = true;
                            newState.parameter = parameter;
                        }
                    }
                    else if (lastState.enterLoop && command == "LoopEnd") {
                        newState.type = ScriptState::kLoopEnd;
                        newState.enterLoop = false;
                    }
                    else if (lastState.enterLoop && command == "LoopGetIndex") {
                        IEffect::IParameter *parameter = m_effectRef->findParameter(value.c_str());
                        if (Util::isIntegerParameter(parameter)) {
                            newState.type = ScriptState::kLoopGetIndex;
                            newState.enterLoop = true;
                            newState.parameter = parameter;
                        }
                    }
                    else if (useScriptExternal && command == "ScriptExternal") {
                        newState.type = ScriptState::kScriptExternal;
                        useScriptExternal = false;
                        if (lastState.enterLoop)
                            return false;
                    }
                    if (newState.type != ScriptState::kUnknown) {
                        lastState = newState;
                        if (useScriptExternal)
                            scriptExternalStates.push_back(newState);
                        else
                            techniqueScriptStates.push_back(newState);
                        newState.reset();
                    }
                }
            }
            m_techniqueScripts.insert(technique, techniqueScriptStates);
            if (m_externalScript.size() == 0) {
                m_externalScript.copyFromArray(scriptExternalStates);
            }
            return !lastState.enterLoop;
        }
        else {
            ScriptState state;
            Array<IEffect::IPass *> passesToParse;
            technique->getPasses(passesToParse);
            const int npasses = passesToParse.count();
            for (int i = 0; i < npasses; i++) {
                IEffect::IPass *pass = passesToParse[i];
                if (parsePassScript(pass)) {
                    state.type = ScriptState::kPass;
                    state.pass = pass;
                    passes.push_back(pass);
                }
            }
        }
        return true;
    }
    return false;
}

/* EffectEngine::ScriptState */

EffectEngine::ScriptState::ScriptState()
    : type(kUnknown),
      renderColorTargetTextureRef(0),
      renderDepthStencilBufferRef(0),
      parameter(0),
      pass(0),
      enterLoop(false),
      isRenderTargetBound(false)
{
}

EffectEngine::ScriptState::~ScriptState()
{
    reset();
}

void EffectEngine::ScriptState::reset()
{
    type = kUnknown;
    renderColorTargetTextureRef = 0;
    renderDepthStencilBufferRef = 0;
    parameter = 0;
    pass = 0;
    enterLoop = false;
    isRenderTargetBound = false;
}

void EffectEngine::ScriptState::setFromState(const ScriptState &other)
{
    enterLoop = other.enterLoop;
    isRenderTargetBound = other.isRenderTargetBound;
}

}
}
