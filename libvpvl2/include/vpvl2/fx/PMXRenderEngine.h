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

#pragma once
#ifndef VPVL2_FX_PMXRENDERENGINE_H_
#define VPVL2_FX_PMXRENDERENGINE_H_

#include "vpvl2/config.h"
#include "vpvl2/internal/util.h"
#include "vpvl2/IMaterial.h"
#include "vpvl2/IModel.h"
#include "vpvl2/IRenderEngine.h"
#include "vpvl2/fx/EffectEngine.h"

#ifdef VPVL2_ENABLE_OPENCL
#include "vpvl2/cl/PMXAccelerator.h"
#endif

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{

namespace cl {
class PMXAccelerator;
}
namespace gl {
class VertexBundle;
class VertexBundleLayout;
}

class Scene;

namespace fx
{

class VPVL2_API PMXRenderEngine VPVL2_DECL_FINAL : public IRenderEngine
{
public:
    PMXRenderEngine(IApplicationContext *applicationContextRef,
                    Scene *sceneRef,
                    cl::PMXAccelerator *accelerator,
                    IModel *modelRef);
    virtual ~PMXRenderEngine();

    IModel *parentModelRef() const;
    bool upload(void *userData);
    void release();
    void update();
    void setUpdateOptions(int options);
    void renderModel(IEffect::Pass *overridePass);
    void renderEdge(IEffect::Pass *overridePass);
    void renderShadow(IEffect::Pass *overridePass);
    void renderZPlot(IEffect::Pass *overridePass);
    bool hasPreProcess() const;
    bool hasPostProcess() const;
    void preparePostProcess();
    void performPreProcess();
    void performPostProcess(IEffect *nextPostEffect);
    IEffect *effectRef(IEffect::ScriptOrderType type) const;
    IEffect *defaultEffectRef() const;
    void setEffect(IEffect *effectRef, IEffect::ScriptOrderType type, void *userData);
    bool testVisible();

    void bindVertexBundle();
    void bindEdgeBundle();

    IApplicationContext *applicationContextRef() const { return m_applicationContextRef; }
    Scene *sceneRef() const { return m_sceneRef; }

private:
    class PrivateEffectEngine;
    class TransformFeedbackProgram;
    enum VertexBufferObjectType {
        kModelDynamicVertexBufferEven,
        kModelDynamicVertexBufferOdd,
        kModelStaticVertexBuffer,
        kModelIndexBuffer,
        kModelBindPoseVertexBuffer,
        kMaxVertexBufferObjectType
    };
    enum VertexArrayObjectType {
        kVertexArrayObjectEven,
        kVertexArrayObjectOdd,
        kEdgeVertexArrayObjectEven,
        kEdgeVertexArrayObjectOdd,
        kBindPoseVertexArrayObject,
        kMaxVertexArrayObjectType
    };
    struct MaterialContext {
        MaterialContext()
            : mainTextureRef(0),
              toonTextureRef(0),
              sphereTextureRef(0),
              toonTextureColor(1, 1, 1, 1)
        {
        }
        ~MaterialContext() {
            mainTextureRef = 0;
            toonTextureRef = 0;
            sphereTextureRef = 0;
        }
        ITexture *mainTextureRef;
        ITexture *toonTextureRef;
        ITexture *sphereTextureRef;
        Color toonTextureColor;
    };

    typedef void (GLAPIENTRY * PFNGLCULLFACEPROC) (gl::GLenum mode);
    typedef void (GLAPIENTRY * PFNGLENABLEPROC) (gl::GLenum cap);
    typedef void (GLAPIENTRY * PFNGLDISABLEPROC) (gl::GLenum cap);
    typedef void (GLAPIENTRY * PFNGLGENQUERIESPROC) (gl::GLsizei n, gl::GLuint* ids);
    typedef void (GLAPIENTRY * PFNGLBEGINQUERYPROC) (gl::GLenum target, gl::GLuint id);
    typedef void (GLAPIENTRY * PFNGLENDQUERYPROC) (gl::GLenum target);
    typedef void (GLAPIENTRY * PFNGLGETQUERYOBJECTIVPROC) (gl::GLuint id, gl::GLenum pname, gl::GLint* params);
    typedef void (GLAPIENTRY * PFNGLDELETEQUERIESPROC) (gl::GLsizei n, const gl::GLuint* ids);
    typedef void (GLAPIENTRY * PFNGLDRAWARRAYSPROC) (gl::GLenum mode, gl::GLint first, gl::GLsizei count);
    PFNGLCULLFACEPROC cullFace;
    PFNGLENABLEPROC enable;
    PFNGLDISABLEPROC disable;
    PFNGLGENQUERIESPROC genQueries;
    PFNGLBEGINQUERYPROC beginQuery;
    PFNGLENDQUERYPROC endQuery;
    PFNGLGETQUERYOBJECTIVPROC getQueryObjectiv;
    PFNGLDELETEQUERIESPROC deleteQueries;
    PFNGLDRAWARRAYSPROC drawArrays;

    bool uploadMaterials(void *userData);
    bool releaseUserData0(void *userData);
    void initializeEffectParameters(int extraCameraFlags);
    void refreshEffect();
    void createVertexBundle(gl::VertexBundleLayout *layout, IModel::Buffer::StrideType strideType, gl::GLuint dvbo);
    void unbindVertexBundle();
    void bindDynamicVertexAttributePointers(IModel::Buffer::StrideType type);
    void bindStaticVertexAttributePointers();
    void getVertexBundleType(VertexArrayObjectType &vao, VertexBufferObjectType &vbo) const;
    void getEdgeBundleType(VertexArrayObjectType &vao, VertexBufferObjectType &vbo) const;
    void getDrawPrimitivesCommand(EffectEngine::DrawPrimitiveCommand &command) const;
    void updateDrawPrimitivesCommand(const IMaterial *material, EffectEngine::DrawPrimitiveCommand &command) const;
    void updateBoneTransformMatrixPaletteData();
    void updateMaterialParameters(const IMaterial *material, const MaterialContext &context);
    void uploadToonTexture(const IMaterial *material,
                           const IString *toonTexturePath,
                           EffectEngine *engineRef,
                           MaterialContext &context,
                           int flags,
                           void *userData);
    void setupOffscreenEffect(IEffect *effectRef, void *userData);
    void executeOneTechniqueAllPasses(const char *name, Array<IEffect::Pass *> &passes);
    void labelVertexArray(const gl::VertexBundleLayout *layout, const char *name);
    void labelVertexBuffer(gl::GLenum key, const char *name);
    void annotateMaterial(const char *name, const IMaterial *material);
    __attribute__((format(printf, 2, 3)))
    void annotate(const char *const format, ...);

    PrivateEffectEngine *m_currentEffectEngineRef;
    cl::PMXAccelerator *m_accelerator;
#ifdef VPVL2_ENABLE_OPENCL
    cl::PMXAccelerator::VertexBufferBridgeArray m_accelerationBuffers;
#endif
    IApplicationContext *m_applicationContextRef;
    TransformFeedbackProgram *m_transformFeedbackProgram;
    Scene *m_sceneRef;
    IModel *m_modelRef;
    IModel::StaticVertexBuffer *m_staticBuffer;
    IModel::DynamicVertexBuffer *m_dynamicBuffer;
    IModel::IndexBuffer *m_indexBuffer;
    gl::VertexBundle *m_bundle;
    gl::VertexBundleLayout *m_layouts[kMaxVertexArrayObjectType];
    Array<MaterialContext> m_materialContexts;
    PointerHash<HashPtr, ITexture> m_allocatedTextures;
    PointerHash<HashInt, PrivateEffectEngine> m_effectEngines;
    PointerArray<PrivateEffectEngine> m_oseffects;
    IEffect *m_defaultEffectRef;
    gl::GLenum m_indexType;
    Vector3 m_aabbMin;
    Vector3 m_aabbMax;
    bool m_cullFaceState;
    bool m_updateEvenBuffer;

    VPVL2_DISABLE_COPY_AND_ASSIGN(PMXRenderEngine)
};

} /* namespace fx */
} /* namespace VPVL2_VERSION_NS */
} /* namespace vpvl2 */

#endif
