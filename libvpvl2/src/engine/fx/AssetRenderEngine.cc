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

#include "vpvl2/fx/AssetRenderEngine.h"
#include "vpvl2/internal/util.h"

#if defined(VPVL2_LINK_ASSIMP) || defined(VPVL2_LINK_ASSIMP3)

#include "vpvl2/vpvl2.h"
#include "vpvl2/asset/Model.h"
#include "vpvl2/gl/VertexBundle.h"
#include "vpvl2/gl/VertexBundleLayout.h"

#if defined(VPVL2_LINK_ASSIMP3)
#include <assimp/scene.h>
#elif defined(VPVL2_LINK_ASSIMP)
#include <assimp/aiScene.h>
#endif

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{
namespace fx
{
using namespace gl;

class AssetRenderEngine::PrivateEffectEngine : public EffectEngine {
public:
    static const std::string canonicalizePath(const std::string &path) {
        const std::string from("\\"), to("/");
        std::string ret(path);
        std::string::size_type pos(path.find(from));
        while (pos != std::string::npos) {
            ret.replace(pos, from.length(), to);
            pos = ret.find(from, pos + to.length());
        }
        return ret;
    }
    static bool splitTexturePath(const std::string &path, std::string &mainTexture, std::string &subTexture) {
        std::string::size_type pos = path.find_first_of("*");
        if (pos != std::string::npos) {
            mainTexture.assign(canonicalizePath(path.substr(0, pos)));
            subTexture.assign(canonicalizePath(path.substr(pos + 1)));
            return true;
        }
        else {
            mainTexture.assign(canonicalizePath(path));
            subTexture.assign(std::string());
            return false;
        }
    }

    PrivateEffectEngine(AssetRenderEngine *renderEngineRef, const IApplicationContext::FunctionResolver *resolver)
        : EffectEngine(renderEngineRef->sceneRef(), renderEngineRef->applicationContextRef()),
          drawElementsBaseVertex(reinterpret_cast<PFNGLDRAWELEMENTSBASEVERTEXPROC>(resolver->resolveSymbol("glDrawElementsBaseVertex"))),
          drawElements(reinterpret_cast<PFNGLDRAWELEMENTSPROC>(resolver->resolveSymbol("glDrawElements"))),
          m_parentRenderEngineRef(renderEngineRef)
    {
    }
    ~PrivateEffectEngine() {
        m_parentRenderEngineRef = 0;
    }

    void setMesh(const aiMesh *value) {
        m_mesh = value;
    }

protected:
    typedef void (GLAPIENTRY * PFNGLDRAWELEMENTSBASEVERTEXPROC) (GLenum mode, GLsizei count, GLenum type, void* indices, GLint basevertex);
    typedef void (GLAPIENTRY * PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
    PFNGLDRAWELEMENTSBASEVERTEXPROC drawElementsBaseVertex;
    PFNGLDRAWELEMENTSPROC drawElements;

    void drawPrimitives(const DrawPrimitiveCommand &command) const {
        if (drawElementsBaseVertex) {
            drawElementsBaseVertex(command.mode, command.count, command.type,
                                   const_cast<uint8 *>(command.ptr) + command.offset, 0);
        }
        else {
            drawElements(command.mode, command.count, command.type, command.ptr + command.offset);
        }
    }
    void rebindVertexBundle() {
        m_parentRenderEngineRef->bindVertexBundle(m_mesh);
    }

private:
    AssetRenderEngine *m_parentRenderEngineRef;
    const aiMesh *m_mesh;

    VPVL2_DISABLE_COPY_AND_ASSIGN(PrivateEffectEngine)
};

AssetRenderEngine::AssetRenderEngine(IApplicationContext *applicationContextRef, Scene *scene, asset::Model *model)
    : cullFace(reinterpret_cast<PFNGLCULLFACEPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glCullFace"))),
      enable(reinterpret_cast<PFNGLENABLEPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glEnable"))),
      disable(reinterpret_cast<PFNGLDISABLEPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glDisable"))),
      genQueries(reinterpret_cast<PFNGLGENQUERIESPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glGenQueries"))),
      beginQuery(reinterpret_cast<PFNGLBEGINQUERYPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glBeginQuery"))),
      endQuery(reinterpret_cast<PFNGLENDQUERYPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glEndQuery"))),
      getQueryObjectiv(reinterpret_cast<PFNGLGETQUERYOBJECTIVPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glGetQueryObjectiv"))),
      deleteQueries(reinterpret_cast<PFNGLDELETEQUERIESPROC>(applicationContextRef->sharedFunctionResolverInstance()->resolveSymbol("glDeleteQueries"))),
      m_currentEffectEngineRef(0),
      m_applicationContextRef(applicationContextRef),
      m_sceneRef(scene),
      m_modelRef(model),
      m_defaultEffectRef(0),
      m_nvertices(0),
      m_nmeshes(0),
      m_cullFaceState(true)
{
}

AssetRenderEngine::~AssetRenderEngine()
{
    if (m_modelRef) {
        VPVL2_LOG(WARNING, "destroyed AssetRenderEngine without calling AssetRenderEngine#release explicitly: " << this);
        release();
    }
    m_applicationContextRef = 0;
    m_sceneRef = 0;
}

IModel *AssetRenderEngine::parentModelRef() const
{
    return m_modelRef && m_modelRef->parentSceneRef() ? m_modelRef : 0;
}

bool AssetRenderEngine::upload(void *userData)
{
    bool ret = true;
    const aiScene *scene = m_modelRef->aiScenePtr();
    if (!scene) {
        return true;
    }
    pushAnnotationGroup(std::string("AssetRenderEngine#upload name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), m_applicationContextRef);
    const unsigned int nmaterials = scene->mNumMaterials;
    aiString texturePath;
    std::string path, mainTexture, subTexture;
    PrivateEffectEngine *engine = 0;
    int flags = 0;
    if (PrivateEffectEngine *const *enginePtr = m_effectEngines.find(IEffect::kStandard)) {
        engine = *enginePtr;
        if (engine->materialTexture.isMipmapEnabled()) {
            flags |= IApplicationContext::kGenerateTextureMipmap;
        }
    }
    for (unsigned int i = 0; i < nmaterials; i++) {
        aiMaterial *material = scene->mMaterials[i];
        aiReturn found = AI_SUCCESS;
        int textureIndex = 0;
        while (found == AI_SUCCESS) {
            found = material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath);
            path = texturePath.data;
            if (PrivateEffectEngine::splitTexturePath(path, mainTexture, subTexture)) {
                if (m_textureMap[mainTexture] == 0) {
                    IString *mainTexturePath = m_applicationContextRef->toUnicode(reinterpret_cast<const uint8 *>(mainTexture.c_str()));
                    if (ITexture *texturePtr = m_applicationContextRef->uploadModelTexture(mainTexturePath, flags, userData)) {
                        m_textureMap[mainTexture] = m_allocatedTextures.insert(texturePtr, texturePtr);
                        if (engine) {
                            engine->materialTexture.setTexture(material, texturePtr);
                        }
                        VPVL2_VLOG(2, "Loaded a main texture: name=" << internal::cstr(mainTexturePath, "(null)") << " ID=" << texturePtr);
                    }
                    internal::deleteObject(mainTexturePath);
                }
                if (m_textureMap[subTexture] == 0) {
                    IString *subTexturePath = m_applicationContextRef->toUnicode(reinterpret_cast<const uint8 *>(subTexture.c_str()));
                    if (ITexture *texturePtr = m_applicationContextRef->uploadModelTexture(subTexturePath, flags, userData)) {
                        m_textureMap[subTexture] = m_allocatedTextures.insert(texturePtr, texturePtr);
                        if (engine) {
                            engine->materialSphereMap.setTexture(material, texturePtr);
                        }
                        VPVL2_VLOG(2, "Loaded a sub texture: name=" << internal::cstr(subTexturePath, "(null)") << " ID=" << texturePtr);
                    }
                    internal::deleteObject(subTexturePath);
                }
            }
            else if (m_textureMap[mainTexture] == 0) {
                IString *mainTexturePath = m_applicationContextRef->toUnicode(reinterpret_cast<const uint8 *>(mainTexture.c_str()));
                if (ITexture *texturePtr = m_applicationContextRef->uploadModelTexture(mainTexturePath, flags, userData)) {
                    m_textureMap[mainTexture] = m_allocatedTextures.insert(texturePtr, texturePtr);
                    if (engine) {
                        engine->materialTexture.setTexture(material, texturePtr);
                    }
                    VPVL2_VLOG(2, "Loaded a main texture: name=" << internal::cstr(mainTexturePath, "(null)") << " ID=" << texturePtr);
                }
                internal::deleteObject(mainTexturePath);
            }
            textureIndex++;
        }
    }
    ret = uploadRecurse(scene, scene->mRootNode, userData);
    m_modelRef->setVisible(ret);
    popAnnotationGroup(m_applicationContextRef);
    return ret;
}

void AssetRenderEngine::release()
{
    pushAnnotationGroup(std::string("AssetRenderEngine#release name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), m_applicationContextRef);
    m_vao.releaseAll();
    m_vbo.releaseAll();
    m_allocatedTextures.releaseAll();
    m_effectEngines.releaseAll();
    m_oseffects.releaseAll();
    m_defaultEffectRef = 0;
    m_currentEffectEngineRef = 0;
    m_modelRef = 0;
    m_nvertices = 0;
    m_nmeshes = 0;
    m_cullFaceState = false;
    popAnnotationGroup(m_applicationContextRef);
}

void AssetRenderEngine::update()
{
    m_currentEffectEngineRef->updateSceneParameters();
}

void AssetRenderEngine::setUpdateOptions(int /* options */)
{
    /* do nothing */
}

void AssetRenderEngine::renderModel(IEffect::Pass *overridePass)
{
    if (!m_modelRef || !m_modelRef->isVisible() || btFuzzyZero(m_modelRef->opacity()) ||
            !m_currentEffectEngineRef || !m_currentEffectEngineRef->isStandardEffect()) {
        return;
    }
    pushAnnotationGroup(std::string("AssetRenderEngine#renderModel name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), m_applicationContextRef);
    bool hasShadowMap = false;
    if (const IShadowMap *shadowMap = m_sceneRef->shadowMapRef()) {
        m_currentEffectEngineRef->depthTexture.setTexture(shadowMap->textureRef());
        m_currentEffectEngineRef->selfShadow.updateParameter(shadowMap);
        hasShadowMap = true;
    }
    initializeEffectParameters();
    refreshEffect();
    const aiScene *a = m_modelRef->aiScenePtr();
    renderRecurse(a, a->mRootNode, overridePass, hasShadowMap);
    if (!m_cullFaceState) {
        enable(kGL_CULL_FACE);
        m_cullFaceState = true;
    }
    popAnnotationGroup(m_applicationContextRef);
}

void AssetRenderEngine::renderEdge(IEffect::Pass * /* overridePass */)
{
    /* do nothing */
}

void AssetRenderEngine::renderShadow(IEffect::Pass * /* overridePass */)
{
    /* do nothing */
}

void AssetRenderEngine::renderZPlot(IEffect::Pass *overridePass)
{
    if (!m_modelRef || !m_modelRef->isVisible() || btFuzzyZero(m_modelRef->opacity()) ||
            !m_currentEffectEngineRef || !m_currentEffectEngineRef->isStandardEffect()) {
        return;
    }
    pushAnnotationGroup(std::string("AssetRenderEngine#renderZPlot name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), m_applicationContextRef);
    initializeEffectParameters();
    refreshEffect();
    const aiScene *a = m_modelRef->aiScenePtr();
    disable(kGL_CULL_FACE);
    renderZPlotRecurse(a, a->mRootNode, overridePass);
    enable(kGL_CULL_FACE);
    popAnnotationGroup(m_applicationContextRef);
}

bool AssetRenderEngine::hasPreProcess() const
{
    return m_currentEffectEngineRef ? m_currentEffectEngineRef->hasTechniques(IEffect::kPreProcess) : false;
}

bool AssetRenderEngine::hasPostProcess() const
{
    return m_currentEffectEngineRef ? m_currentEffectEngineRef->hasTechniques(IEffect::kPostProcess) : false;
}

void AssetRenderEngine::preparePostProcess()
{
    if (m_currentEffectEngineRef) {
        pushAnnotationGroup(std::string("AssetRenderEngine#preparePostProcess name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), m_applicationContextRef);
        m_currentEffectEngineRef->executeScriptExternal();
        popAnnotationGroup(m_applicationContextRef);
    }
}

void AssetRenderEngine::performPreProcess()
{
    if (hasPreProcess()) {
        pushAnnotationGroup(std::string("AssetRenderEngine#performPreProcess name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), m_applicationContextRef);
        m_currentEffectEngineRef->executeProcess(m_modelRef, 0, IEffect::kPreProcess);
        popAnnotationGroup(m_applicationContextRef);
    }
}

void AssetRenderEngine::performPostProcess(IEffect *nextPostEffect)
{
    if (hasPostProcess()) {
        pushAnnotationGroup(std::string("AssetRenderEngine#performPostProcess name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), m_applicationContextRef);
        m_currentEffectEngineRef->executeProcess(m_modelRef, nextPostEffect, IEffect::kPostProcess);
        popAnnotationGroup(m_applicationContextRef);
    }
}

IEffect *AssetRenderEngine::effectRef(IEffect::ScriptOrderType type) const
{
    if (type == IEffect::kDefault) {
        return m_defaultEffectRef;
    }
    else {
        const PrivateEffectEngine *const *ee = m_effectEngines.find(type);
        return ee ? (*ee)->effect() : 0;
    }
}

void AssetRenderEngine::setEffect(IEffect *effectRef, IEffect::ScriptOrderType type, void *userData)
{
    const IApplicationContext::FunctionResolver *resolver = m_applicationContextRef->sharedFunctionResolverInstance();
    pushAnnotationGroup(std::string("AssetRenderEngine#setEffect name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), resolver);
    if (type == IEffect::kStandardOffscreen) {
        const int neffects = m_oseffects.count();
        bool found = false;
        PrivateEffectEngine *ee = 0;
        for (int i = 0; i < neffects; i++) {
            ee = m_oseffects[i];
            if (ee->effect() == effectRef) {
                found = true;
                break;
            }
        }
        if (found) {
            m_currentEffectEngineRef = ee;
        }
        else if (effectRef) {
            PrivateEffectEngine *previous = m_currentEffectEngineRef;
            m_currentEffectEngineRef = new PrivateEffectEngine(this, resolver);
            m_currentEffectEngineRef->setEffect(effectRef, userData, false);
            const aiScene *scene = m_modelRef->aiScenePtr();
            if (scene && m_currentEffectEngineRef->scriptOrder() == IEffect::kStandard) {
                const unsigned int nmaterials = scene->mNumMaterials;
                std::string texture, mainTexture, subTexture;
                aiString texturePath;
                /* copy current material textures/spheres parameters to offscreen effect */
                for (unsigned int i = 0; i < nmaterials; i++) {
                    aiMaterial *material = scene->mMaterials[i];
                    aiReturn found = AI_SUCCESS;
                    int textureIndex = 0;
                    while (found == AI_SUCCESS) {
                        found = material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath);
                        if (found != AI_SUCCESS) {
                            break;
                        }
                        texture = texturePath.data;
                        if (PrivateEffectEngine::splitTexturePath(texture, mainTexture, subTexture)) {
                            Textures::const_iterator sub = m_textureMap.find(subTexture);
                            if (sub != m_textureMap.end()) {
                                m_currentEffectEngineRef->materialSphereMap.setTexture(material, sub->second);
                            }
                        }
                        Textures::const_iterator main = m_textureMap.find(mainTexture);
                        if (main != m_textureMap.end()) {
                            m_currentEffectEngineRef->materialTexture.setTexture(material, main->second);
                        }
                        textureIndex++;
                    }
                }
                m_oseffects.append(m_currentEffectEngineRef);
            }
            else {
                internal::deleteObject(m_currentEffectEngineRef);
                m_currentEffectEngineRef = previous;
            }
        }
    }
    else {
        IEffect::ScriptOrderType findType = (type == IEffect::kAutoDetection && effectRef) ? effectRef->scriptOrderType() : type;
        if (PrivateEffectEngine *const *ee = m_effectEngines.find(findType)) {
            m_currentEffectEngineRef = *ee;
        }
        else {
            /* set default standard effect if effect is null */
            bool wasEffectNull = false;
            if (!effectRef) {
                m_defaultEffectRef = m_sceneRef->createDefaultStandardEffectRef(m_applicationContextRef);
                effectRef = m_defaultEffectRef;
                wasEffectNull = true;
            }
            m_currentEffectEngineRef = new PrivateEffectEngine(this, resolver);
            m_currentEffectEngineRef->setEffect(effectRef, userData, wasEffectNull);
            m_effectEngines.insert(type == IEffect::kAutoDetection ? m_currentEffectEngineRef->scriptOrder() : type, m_currentEffectEngineRef);
            /* set default standard effect as secondary effect */
            if (!wasEffectNull && m_currentEffectEngineRef->scriptOrder() == IEffect::kStandard) {
                m_defaultEffectRef = m_sceneRef->createDefaultStandardEffectRef(m_applicationContextRef);
                m_currentEffectEngineRef->setDefaultStandardEffectRef(m_defaultEffectRef);
            }
        }
    }
    m_currentEffectEngineRef->parthf.setValue(false);
    m_currentEffectEngineRef->transp.setValue(false);
    m_currentEffectEngineRef->opadd.setValue(false);
    popAnnotationGroup(resolver);
}

bool AssetRenderEngine::testVisible()
{
    const IApplicationContext::FunctionResolver *resolver = m_applicationContextRef->sharedFunctionResolverInstance();
    pushAnnotationGroup(std::string("AssetRenderEngine#testVisible name=").append(internal::cstr(m_modelRef->name(IEncoding::kDefaultLanguage), "")).c_str(), resolver);
    GLenum target = kGL_NONE;
    int version = resolver->query(IApplicationContext::FunctionResolver::kQueryVersion);
    bool visible = true;
    if (version >= makeVersion(3, 3) || resolver->hasExtension("ARB_occlusion_query2")) {
        target = kGL_ANY_SAMPLES_PASSED;
    }
    else if (resolver->hasExtension("ARB_occlusion_query")) {
        target = kGL_SAMPLES_PASSED;
    }
    if (target != kGL_NONE) {
        GLuint query = 0;
        genQueries(1, &query);
        beginQuery(target, query);
        renderEdge(0);
        endQuery(target);
        GLint result = 0;
        getQueryObjectiv(query, kGL_QUERY_RESULT, &result);
        visible = result != 0;
        deleteQueries(1, &query);
    }
    popAnnotationGroup(resolver);
    return visible;
}

void AssetRenderEngine::bindVertexBundle(const aiMesh *mesh)
{
    m_currentEffectEngineRef->setMesh(mesh);
    if (mesh) {
        VertexBundleLayout *const *layout = m_vao.find(mesh);
        if (layout && !(*layout)->bind()) {
            VertexBundle *bundle = *m_vbo.find(mesh);
            bundle->bind(VertexBundle::kVertexBuffer, 0);
            bindStaticVertexAttributePointers();
            bundle->bind(VertexBundle::kIndexBuffer, 0);
        }
    }
}

bool AssetRenderEngine::uploadRecurse(const aiScene *scene, const aiNode *node, void *userData)
{
    pushAnnotationGroup("AssetRenderEngine#uploadRecurse", m_applicationContextRef);
    const unsigned int nmeshes = node->mNumMeshes;
    bool ret = true;
    m_nmeshes = nmeshes;
    Vertices assetVertices;
    Vertex assetVertex;
    Array<int> vertexIndices;
    assetVertex.position.setW(1);
    for (unsigned int i = 0; i < nmeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        const unsigned int nfaces = mesh->mNumFaces;
        unsigned int numIndices = 0;
        switch (mesh->mPrimitiveTypes) {
        case aiPrimitiveType_LINE:
            numIndices = 1;
            break;
        case aiPrimitiveType_POINT:
            numIndices = 2;
            break;
        case aiPrimitiveType_TRIANGLE:
            numIndices = 3;
            break;
        default:
            break;
        }
        for (unsigned int j = 0; j < nfaces; j++) {
            const struct aiFace &face = mesh->mFaces[j];
            for (unsigned int k = 0; k < numIndices; k++) {
                int vertexIndex = face.mIndices[k];
                vertexIndices.append(vertexIndex);
            }
        }
        const bool hasNormals = mesh->HasNormals();
        const bool hasTexCoords = mesh->HasTextureCoords(0);
        const bool hasTangent = mesh->HasTangentsAndBitangents();
        const aiVector3D *vertices = mesh->mVertices;
        const aiVector3D *normals = hasNormals ? mesh->mNormals : 0;
        const aiVector3D *texcoords = hasTexCoords ? mesh->mTextureCoords[0] : 0;
        const aiVector3D *tangents = hasTangent ? mesh->mTangents : 0;
        const aiVector3D *bitangents = hasTangent ? mesh->mBitangents : 0;
        const unsigned int nvertices = mesh->mNumVertices;
        for (unsigned int j = 0; j < nvertices; j++) {
            const aiVector3D &vertex = vertices[j];
            assetVertex.position.setValue(vertex.x, vertex.y, vertex.z);
            if (hasNormals) {
                const aiVector3D &normal = normals[j];
                assetVertex.normal.setValue(normal.x, normal.y, normal.z);
            }
            else {
                assetVertex.normal.setZero();
            }
            if (hasTexCoords) {
                const aiVector3D &texcoord = texcoords[j];
                assetVertex.texcoord.setValue(texcoord.x, texcoord.y, texcoord.z);
            }
            else {
                assetVertex.texcoord.setZero();
            }
            if (hasTangent) {
                const aiVector3D &tangent = tangents[j];
                const aiVector3D &bitangent = bitangents[j];
                assetVertex.tangent.setValue(tangent.x, tangent.y, tangent.z);
                assetVertex.bitangent.setValue(bitangent.x, bitangent.y, bitangent.z);
            }
            else {
                assetVertex.tangent.setZero();
                assetVertex.bitangent.setZero();
            }
            assetVertices.append(assetVertex);
        }
        createVertexBundle(mesh, assetVertices, vertexIndices);
        assetVertices.clear();
        vertexIndices.clear();
        unbindVertexBundle(mesh);
    }
    const unsigned int nChildNodes = node->mChildren ? node->mNumChildren : 0;
    for (unsigned int i = 0; i < nChildNodes; i++) {
        ret = uploadRecurse(scene, node->mChildren[i], userData);
        if (!ret) {
            return ret;
        }
    }
    popAnnotationGroup(m_applicationContextRef);
    return ret;
}

void AssetRenderEngine::initializeEffectParameters()
{
    m_currentEffectEngineRef->useToon.setValue(false);
    m_currentEffectEngineRef->subsetCount.setValue(m_nmeshes);
    m_currentEffectEngineRef->vertexCount.setValue(m_nvertices);
    m_currentEffectEngineRef->boneTransformTexture.setTexture(0);
    m_currentEffectEngineRef->updateModelLightParameters(m_sceneRef, m_modelRef);
    m_currentEffectEngineRef->setModelMatrixParameters(m_modelRef);
}

void AssetRenderEngine::refreshEffect()
{
    if (IEffect *effectRef = m_currentEffectEngineRef->effect()) {
        if (effectRef->isDirty()) {
            m_currentEffectEngineRef->invalidate();
            m_currentEffectEngineRef->setEffect(effectRef, 0, false);
            effectRef->setDirty(false);
        }
    }
}

void AssetRenderEngine::renderRecurse(const aiScene *scene, const aiNode *node, IEffect::Pass *overridePass, const bool hasShadowMap)
{
    pushAnnotationGroup("AssetRenderEngine#renderRecurse", m_applicationContextRef);
    const unsigned int nmeshes = node->mNumMeshes;
    EffectEngine::DrawPrimitiveCommand command;
    for (unsigned int i = 0; i < nmeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        bool hasTexture = false, hasSphereMap = false;
        const char *target = hasShadowMap ? "object_ss" : "object";
        setAssetMaterial(scene->mMaterials[mesh->mMaterialIndex], hasTexture, hasSphereMap);
        IEffect::Technique *technique = m_currentEffectEngineRef->findTechnique(target, i, nmeshes, hasTexture, hasSphereMap, false);
        vsize nindices = *m_numIndices.find(mesh);
        if (technique) {
            if (overridePass) {
                technique->setOverridePass(overridePass);
            }
            else {
                Array<IEffect::Pass *> passes;
                technique->getOverridePasses(passes);
                if (passes.count() > 0) {
                    overridePass = passes[0];
                    technique = m_currentEffectEngineRef->findDefaultTechnique(target, i, nmeshes, hasTexture, hasSphereMap, false);
                    technique->setOverridePass(overridePass);
                }
            }
            bindVertexBundle(mesh);
            command.count = nindices;
            setDrawCommandMode(command, mesh);
            annotate("renderModel: model=%s mesh=%d", m_modelRef->name(IEncoding::kDefaultLanguage)->toByteArray(), i);
            pushAnnotationGroup("AssetRenderEngine::PrivateEffectEngine#executeTechniquePasses", m_applicationContextRef);
            m_currentEffectEngineRef->executeTechniquePasses(technique, command, 0);
            popAnnotationGroup(m_applicationContextRef);
            unbindVertexBundle(mesh);
        }
    }
    const unsigned int nChildNodes = node->mChildren ? node->mNumChildren : 0;
    for (unsigned int i = 0; i < nChildNodes; i++) {
        renderRecurse(scene, node->mChildren[i], overridePass, hasShadowMap);
    }
    popAnnotationGroup(m_applicationContextRef);
}

void AssetRenderEngine::renderZPlotRecurse(const aiScene *scene, const aiNode *node, IEffect::Pass *overridePass)
{
    pushAnnotationGroup("AssetRenderEngine#renderZPlotRecurse", m_applicationContextRef);
    const unsigned int nmeshes = node->mNumMeshes;
    float opacity;
    EffectEngine::DrawPrimitiveCommand command;
    for (unsigned int i = 0; i < nmeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        const struct aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        material->Get(AI_MATKEY_OPACITY, opacity);
        if (btFuzzyZero(opacity - 0.98f)) {
            continue;
        }
        bindVertexBundle(mesh);
        IEffect::Technique *technique = m_currentEffectEngineRef->findTechnique("zplot", i, nmeshes, false, false, false);
        if (technique) {
            technique->setOverridePass(overridePass);
            vsize nindices = *m_numIndices.find(mesh);
            command.count = nindices;
            setDrawCommandMode(command, mesh);
            annotate("renderZplot: model=%s mesh=%d", m_modelRef->name(IEncoding::kDefaultLanguage)->toByteArray(), i);
            pushAnnotationGroup("AssetRenderEngine::PrivateEffectEngine#executeTechniquePasses", m_applicationContextRef);
            m_currentEffectEngineRef->executeTechniquePasses(technique, command, 0);
            popAnnotationGroup(m_applicationContextRef);
        }
        unbindVertexBundle(mesh);
    }
    const unsigned int nChildNodes = node->mChildren ? node->mNumChildren : 0;
    for (unsigned int i = 0; i < nChildNodes; i++) {
        renderZPlotRecurse(scene, node->mChildren[i], overridePass);
    }
    popAnnotationGroup(m_applicationContextRef);
}

void AssetRenderEngine::setAssetMaterial(const aiMaterial *material, bool &hasTexture, bool &hasSphereMap)
{
    int textureIndex = 0;
    ITexture *textureRef = 0;
    std::string mainTexture, subTexture;
    aiString texturePath;
    hasTexture = false;
    hasSphereMap = false;
    if (material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath) == aiReturn_SUCCESS) {
        bool isAdditive = false;
        if (PrivateEffectEngine::splitTexturePath(texturePath.data, mainTexture, subTexture)) {
            textureRef = m_textureMap[subTexture];
            isAdditive = subTexture.find(".spa") != std::string::npos;
            m_currentEffectEngineRef->spadd.setValue(isAdditive);
            m_currentEffectEngineRef->useSpheremap.setValue(true);
            hasSphereMap = true;
        }
        textureRef = m_textureMap[mainTexture];
        if (textureRef && textureRef->data()) {
            m_currentEffectEngineRef->useTexture.setValue(true);
            hasTexture = true;
        }
    }
    m_currentEffectEngineRef->useTexture.setValue(hasTexture);
    m_currentEffectEngineRef->useSpheremap.setValue(hasSphereMap);
    m_currentEffectEngineRef->materialTexture.updateParameter(material);
    m_currentEffectEngineRef->materialSphereMap.updateParameter(material);
    // * ambient = diffuse
    // * specular / 10
    // * emissive
    aiColor4D emissive, diffuse, specular;
    Color color;
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == aiReturn_SUCCESS) {
        color.setValue(emissive.r, emissive.g, emissive.b, emissive.a);
    }
    else {
        color.setValue(0, 0, 0, 1);
    }
    m_currentEffectEngineRef->emissive.setGeometryColor(color);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == aiReturn_SUCCESS) {
        color.setValue(diffuse.r, diffuse.g, diffuse.b, diffuse.a * m_modelRef->opacity());
    }
    else {
        color.setValue(1, 1, 1, m_modelRef->opacity());
    }
    m_currentEffectEngineRef->ambient.setGeometryColor(color);
    m_currentEffectEngineRef->diffuse.setGeometryColor(color);
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) == aiReturn_SUCCESS) {
        static const float kDivide = 10.0;
        color.setValue(specular.r / kDivide, specular.g / kDivide, specular.b / kDivide, specular.a);
    }
    else {
        color.setValue(0, 0, 0, 1);
    }
    m_currentEffectEngineRef->specular.setGeometryColor(color);
    float shininess = 0, strength = 0;
    int ret1 = material->Get(AI_MATKEY_SHININESS, shininess);
    int ret2 = material->Get(AI_MATKEY_SHININESS_STRENGTH, strength);
    if (ret1 == aiReturn_SUCCESS && ret2 == aiReturn_SUCCESS) {
        m_currentEffectEngineRef->specularPower.setGeometryValue(shininess * strength);
    }
    else if (ret1 == aiReturn_SUCCESS) {
        m_currentEffectEngineRef->specularPower.setGeometryValue(shininess);
    }
    else {
        m_currentEffectEngineRef->specularPower.setGeometryValue(1);
    }
    int twoside = 0;
    if (material->Get(AI_MATKEY_TWOSIDED, twoside) == aiReturn_SUCCESS && twoside && m_cullFaceState) {
        disable(kGL_CULL_FACE);
        m_cullFaceState = false;
    }
    else if (!m_cullFaceState) {
        enable(kGL_CULL_FACE);
        m_cullFaceState = true;
    }
}

void AssetRenderEngine::createVertexBundle(const aiMesh *mesh,
                                           const Vertices &vertices,
                                           const Indices &indices)
{
    const IApplicationContext::FunctionResolver *resolver = m_applicationContextRef->sharedFunctionResolverInstance();
    pushAnnotationGroup("AssetRenderEngine#uploadRecurse", resolver);
    VertexBundleLayout *layout = m_vao.insert(mesh, new VertexBundleLayout(resolver));
    VertexBundle *bundle = m_vbo.insert(mesh, new VertexBundle(resolver));
    vsize isize = sizeof(indices[0]) * indices.count();
    annotate("createVertexBundle: model=%s", m_modelRef->name(IEncoding::kDefaultLanguage)->toByteArray());
    bundle->create(VertexBundle::kIndexBuffer, 0, VertexBundle::kGL_STATIC_DRAW, &indices[0], isize);
    VPVL2_VLOG(2, "Binding asset index buffer to the vertex buffer object");
    vsize vsize = vertices.count() * sizeof(vertices[0]);
    bundle->create(VertexBundle::kVertexBuffer, 0, VertexBundle::kGL_STATIC_DRAW, &vertices[0].position, vsize);
    VPVL2_VLOG(2, "Binding asset vertex buffer to the vertex buffer object");
    if (layout->create() && layout->bind()) {
        VPVL2_VLOG(2, "Created an vertex array object: " << layout->name());
    }
    bundle->bind(VertexBundle::kVertexBuffer, 0);
    bindStaticVertexAttributePointers();
    bundle->bind(VertexBundle::kIndexBuffer, 0);
    layout->unbind();
    m_numIndices.insert(mesh, indices.count());
    popAnnotationGroup(resolver);
}

void AssetRenderEngine::unbindVertexBundle(const aiMesh *mesh)
{
    VertexBundleLayout *const *layout = m_vao.find(mesh);
    if (layout && !(*layout)->unbind()) {
        IEffect *effectRef = m_currentEffectEngineRef->effect();
        effectRef->deactivateVertexAttribute(IEffect::kPositionVertexAttribute);
        effectRef->deactivateVertexAttribute(IEffect::kNormalVertexAttribute);
        effectRef->deactivateVertexAttribute(IEffect::kTextureCoordVertexAttribute);
        for (int i = IEffect::kUVA1VertexAttribute; i <= int(IEffect::kUVA4VertexAttribute); i++) {
            IEffect::VertexAttributeType attribType = static_cast<IEffect::VertexAttributeType>(int(IModel::DynamicVertexBuffer::kUVA1Stride) + i);
            effectRef->deactivateVertexAttribute(attribType);
        }
        VertexBundle *bundle = *m_vbo.find(mesh);
        bundle->unbind(VertexBundle::kVertexBuffer);
        bundle->unbind(VertexBundle::kIndexBuffer);
    }
}

void AssetRenderEngine::bindStaticVertexAttributePointers()
{
    pushAnnotationGroup("AssetRenderEngine#bindStaticVertexAttributePointers", m_applicationContextRef);
    static const Vertex v;
    IEffect *effectRef = m_currentEffectEngineRef->effect();
    const GLvoid *vertexPtr = 0;
    effectRef->setVertexAttributePointer(IEffect::kPositionVertexAttribute, IEffect::Parameter::kFloat4, sizeof(v), vertexPtr);
    effectRef->activateVertexAttribute(IEffect::kPositionVertexAttribute);
    const GLvoid *normalPtr = reinterpret_cast<const void *>(reinterpret_cast<const uint8 *>(&v.normal) - reinterpret_cast<const uint8 *>(&v.position));
    effectRef->setVertexAttributePointer(IEffect::kNormalVertexAttribute, IEffect::Parameter::kFloat4, sizeof(v), normalPtr);
    effectRef->activateVertexAttribute(IEffect::kNormalVertexAttribute);
    const GLvoid *texcoordPtr = reinterpret_cast<const void *>(reinterpret_cast<const uint8 *>(&v.texcoord) - reinterpret_cast<const uint8 *>(&v.position));
    effectRef->setVertexAttributePointer(IEffect::kTextureCoordVertexAttribute, IEffect::Parameter::kFloat4, sizeof(v), texcoordPtr);
    effectRef->activateVertexAttribute(IEffect::kTextureCoordVertexAttribute);
    const GLvoid *uva1Ptr = reinterpret_cast<const void *>(reinterpret_cast<const uint8 *>(&v.uva1) - reinterpret_cast<const uint8 *>(&v.position));
    effectRef->setVertexAttributePointer(IEffect::kUVA1VertexAttribute, IEffect::Parameter::kFloat4, sizeof(v), uva1Ptr);
    effectRef->activateVertexAttribute(IEffect::kUVA1VertexAttribute);
    const GLvoid *uva2Ptr = reinterpret_cast<const void *>(reinterpret_cast<const uint8 *>(&v.uva2) - reinterpret_cast<const uint8 *>(&v.position));
    effectRef->setVertexAttributePointer(IEffect::kUVA2VertexAttribute, IEffect::Parameter::kFloat4, sizeof(v), uva2Ptr);
    effectRef->activateVertexAttribute(IEffect::kUVA2VertexAttribute);
    const GLvoid *uva3Ptr = reinterpret_cast<const void *>(reinterpret_cast<const uint8 *>(&v.uva3) - reinterpret_cast<const uint8 *>(&v.position));
    effectRef->setVertexAttributePointer(IEffect::kUVA3VertexAttribute, IEffect::Parameter::kFloat4, sizeof(v), uva3Ptr);
    effectRef->activateVertexAttribute(IEffect::kUVA3VertexAttribute);
    const GLvoid *uva4Ptr = reinterpret_cast<const void *>(reinterpret_cast<const uint8 *>(&v.uva4) - reinterpret_cast<const uint8 *>(&v.position));
    effectRef->setVertexAttributePointer(IEffect::kUVA4VertexAttribute, IEffect::Parameter::kFloat4, sizeof(v), uva4Ptr);
    effectRef->activateVertexAttribute(IEffect::kUVA4VertexAttribute);
    popAnnotationGroup(m_applicationContextRef);
}

void AssetRenderEngine::setDrawCommandMode(EffectEngine::DrawPrimitiveCommand &command, const aiMesh *mesh)
{
    switch (mesh->mPrimitiveTypes) {
    case aiPrimitiveType_LINE:
        command.mode = kGL_LINES;
        break;
    case aiPrimitiveType_POINT:
        command.mode = kGL_POINTS;
        break;
    case aiPrimitiveType_TRIANGLE:
        command.mode = kGL_TRIANGLES;
        break;
    default:
        break;
    }
}

void AssetRenderEngine::annotate(const char * const format, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    annotateString(buffer, m_applicationContextRef->sharedFunctionResolverInstance());
}

} /* namespace fx */
} /* namespace VPVL2_VERSION_NS */
} /* namespace vpvl2 */

#endif
