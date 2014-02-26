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
#ifndef VPVL2_FX_ASSETRENDERENGINE_H_
#define VPVL2_FX_ASSETRENDERENGINE_H_

#include "vpvl2/Common.h"
#if defined(VPVL2_LINK_ASSIMP) || defined(VPVL2_LINK_ASSIMP3)

#include "vpvl2/fx/EffectEngine.h"
#include <map>

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

namespace gl
{
class VertexBundle;
class VertexBundleLayout;
}

class Scene;

namespace fx
{

/**
 * @file
 * @author Nagoya Institute of Technology Department of Computer Science
 * @author hkrn
 *
 * @section DESCRIPTION
 *
 * Bone class represents a bone of a Polygon Model Data object.
 */

class VPVL2_API AssetRenderEngine VPVL2_DECL_FINAL : public IRenderEngine
{
public:
    AssetRenderEngine(IApplicationContext *applicationContextRef, Scene *scene, asset::Model *parentModelRef);
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
    void setOverridePass(IEffect::Pass *pass);
    bool testVisible();

    void bindVertexBundle(const aiMesh *mesh);

    IApplicationContext *applicationContextRef() const { return m_applicationContextRef; }
    Scene *sceneRef() const { return m_sceneRef; }

private:
    typedef void (GLAPIENTRY * PFNGLCULLFACEPROC) (gl::GLenum mode);
    typedef void (GLAPIENTRY * PFNGLENABLEPROC) (gl::GLenum cap);
    typedef void (GLAPIENTRY * PFNGLDISABLEPROC) (gl::GLenum cap);
    typedef void (GLAPIENTRY * PFNGLGENQUERIESPROC) (gl::GLsizei n, gl::GLuint* ids);
    typedef void (GLAPIENTRY * PFNGLBEGINQUERYPROC) (gl::GLenum target, gl::GLuint id);
    typedef void (GLAPIENTRY * PFNGLENDQUERYPROC) (gl::GLenum target);
    typedef void (GLAPIENTRY * PFNGLGETQUERYOBJECTIVPROC) (gl::GLuint id, gl::GLenum pname, gl::GLint* params);
    typedef void (GLAPIENTRY * PFNGLDELETEQUERIESPROC) (gl::GLsizei n, const gl::GLuint* ids);
    PFNGLCULLFACEPROC cullFace;
    PFNGLENABLEPROC enable;
    PFNGLDISABLEPROC disable;
    PFNGLGENQUERIESPROC genQueries;
    PFNGLBEGINQUERYPROC beginQuery;
    PFNGLENDQUERYPROC endQuery;
    PFNGLGETQUERYOBJECTIVPROC getQueryObjectiv;
    PFNGLDELETEQUERIESPROC deleteQueries;

    class PrivateEffectEngine;
    typedef std::map<std::string, ITexture *> Textures;
    struct Vertex {
        Vertex()
            : position(kZeroV3),
              normal(kZeroV3),
              texcoord(kZeroV3),
              tangent(kZeroV3),
              bitangent(kZeroV3),
              uva1(kZeroV4),
              uva2(kZeroV4),
              uva3(kZeroV4),
              uva4(kZeroV4)
        {
        }
        Vector3 position;
        Vector3 normal;
        Vector3 texcoord;
        Vector3 tangent;
        Vector3 bitangent;
        Vector4 uva1;
        Vector4 uva2;
        Vector4 uva3;
        Vector4 uva4;
    };
    typedef Array<Vertex> Vertices;
    typedef Array<int> Indices;

    bool uploadRecurse(const aiScene *scene, const aiNode *node, void *userData);
    void initializeEffectParameters();
    void refreshEffect();
    void renderRecurse(const aiScene *scene, const aiNode *node, IEffect::Pass *overridePass, const bool hasShadowMap);
    void renderZPlotRecurse(const aiScene *scene, const aiNode *node, IEffect::Pass *overridePass);
    void setAssetMaterial(const aiMaterial *material, bool &hasTexture, bool &hasSphereMap);
    void createVertexBundle(const aiMesh *mesh, const Vertices &vertices, const Indices &indices);
    void unbindVertexBundle(const aiMesh *mesh);
    void bindStaticVertexAttributePointers();
    void setDrawCommandMode(EffectEngine::DrawPrimitiveCommand &command, const aiMesh *mesh);
    __attribute__((format(printf, 2, 3)))
    void annotate(const char *const format, ...);

    PrivateEffectEngine *m_currentEffectEngineRef;
    IApplicationContext *m_applicationContextRef;
    Scene *m_sceneRef;
    asset::Model *m_modelRef;
    PointerHash<HashInt, PrivateEffectEngine> m_effectEngines;
    PointerArray<PrivateEffectEngine> m_oseffects;
    PointerHash<HashPtr, ITexture> m_allocatedTextures;
    Textures m_textureMap;
    Hash<HashPtr, int> m_numIndices;
    PointerHash<HashPtr, gl::VertexBundle> m_vbo;
    PointerHash<HashPtr, gl::VertexBundleLayout> m_vao;
    IEffect *m_defaultEffectRef;
    int m_nvertices;
    int m_nmeshes;
    bool m_cullFaceState;

    VPVL2_DISABLE_COPY_AND_ASSIGN(AssetRenderEngine)
};

} /* namespace fx */
} /* namespace VPVL2_VERSION_NS */
} /* namespace vpvl2 */

#endif /* VPVL2_LINK_ASSIMP */
#endif
