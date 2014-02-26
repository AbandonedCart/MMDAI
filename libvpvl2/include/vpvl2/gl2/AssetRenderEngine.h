/**

 Copyright (c) 2009-2011  Nagoya Institute of Technology
                          Department of Computer Science
               2010-2014  hkrn

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
#ifndef VPVL2_GL2_ASSETRENDERENGINE_H_
#define VPVL2_GL2_ASSETRENDERENGINE_H_

#include "vpvl2/Common.h"
#if defined(VPVL2_LINK_ASSIMP) || defined(VPVL2_LINK_ASSIMP3)

#include "vpvl2/IApplicationContext.h"
#include "vpvl2/IRenderEngine.h"
#include "vpvl2/gl/VertexBundleLayout.h"

struct aiMaterial;
struct aiMesh;
struct aiNode;
struct aiScene;

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{

namespace asset
{
class Model;
}
namespace extensions
{
namespace gl
{
}
}

class Scene;

namespace gl2
{

class BaseShaderProgram;

/**
 * @file
 * @author Nagoya Institute of Technology Department of Computer Science
 * @author hkrn
 *
 * @section DESCRIPTION
 *
 * Bone class represents a bone of a Polygon Model Data object.
 */

class VPVL2_API AssetRenderEngine : public IRenderEngine
{
public:
    class Program;

    AssetRenderEngine(IApplicationContext *applicationContext, Scene *scene, asset::Model *parentModelRef);
    virtual ~AssetRenderEngine();

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

private:
    typedef void (GLAPIENTRY * PFNGLCULLFACEPROC) (gl::GLenum mode);
    typedef void (GLAPIENTRY * PFNGLENABLEPROC) (gl::GLenum cap);
    typedef void (GLAPIENTRY * PFNGLDISABLEPROC) (gl::GLenum cap);
    typedef void (GLAPIENTRY * PFNGLDRAWELEMENTS) (gl::GLenum mode, gl::GLsizei count, gl::GLenum type, const gl::GLvoid *indices);
    typedef void (GLAPIENTRY * PFNGLENABLEVERTEXATTRIBARRAYPROC) (gl::GLuint);
    typedef void (GLAPIENTRY * PFNGLVERTEXATTRIBPOINTERPROC) (gl::GLuint index, gl::GLint size, gl::GLenum type, gl::GLboolean normalized, gl::GLsizei stride, const gl::GLvoid* pointer);
    PFNGLCULLFACEPROC cullFace;
    PFNGLENABLEPROC enable;
    PFNGLDISABLEPROC disable;
    PFNGLDRAWELEMENTS drawElements;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC vertexAttribPointer;

    struct Vertex {
        Vertex() {}
        Vector4 position;
        Vector3 normal;
        Vector3 texcoord;
    };
    typedef Array<Vertex> Vertices;
    typedef Array<int> Indices;
    class PrivateContext;
    bool uploadRecurse(const aiScene *scene, const aiNode *node, void *userData);
    void deleteRecurse(const aiScene *scene, const aiNode *node);
    void renderRecurse(const aiScene *scene, const aiNode *node);
    void renderZPlotRecurse(const aiScene *scene, const aiNode *node);
    void setAssetMaterial(const aiMaterial *material, Program *program);
    bool createProgram(BaseShaderProgram *program,
                       IApplicationContext::ShaderType vertexShaderType,
                       IApplicationContext::ShaderType fragmentShaderType,
                       void *userData);
    void createVertexBundle(const aiMesh *mesh,
                            const Vertices &vertices,
                            const Indices &indices);
    void bindVertexBundle(const aiMesh *mesh);
    void unbindVertexBundle(const aiMesh *mesh);
    void bindStaticVertexAttributePointers();

    IApplicationContext *m_applicationContextRef;
    Scene *m_sceneRef;
    asset::Model *m_modelRef;
    PrivateContext *m_context;
    gl::VertexBundleLayout m_bundle;

    VPVL2_DISABLE_COPY_AND_ASSIGN(AssetRenderEngine)
};

} /* namespace gl2 */
} /* namespace VPVL2_VERSION_NS */
} /* namespace vpvl2 */

#endif /* VPVL2_LINK_ASSIMP */
#endif

