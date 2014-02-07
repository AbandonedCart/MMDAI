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

#include "../helper.h"
#include <vpvl2/vpvl2.h>
#include <vpvl2/extensions/glfw/ApplicationContext.h>

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LinearMath/btIDebugDraw.h>
#include <glm/gtx/string_cast.hpp>

using namespace vpvl2;
using namespace vpvl2::extensions;
using namespace vpvl2::extensions::icu4c;
using namespace vpvl2::extensions::glfw;

namespace {

struct MemoryMappedFile {
    MemoryMappedFile()
        : address(0),
          size(0),
          opaque(0)
    {
    }
    ~MemoryMappedFile() {
    }
    bool open(const std::string &path) {
        return ApplicationContext::mapFileDescriptor(path, address, size, opaque);
    }
    void close() {
        ApplicationContext::unmapFileDescriptor(address, size, opaque);
    }
    uint8 *address;
    vsize size;
    intptr_t opaque;
};

VPVL2_MAKE_SMARTPTR(ApplicationContext);

class DebugDrawer : public btIDebugDraw {
public:
    DebugDrawer()
        : m_debugMode(0)
    {
    }
    ~DebugDrawer() {
    }

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
        glDisable(GL_DEPTH_TEST);
        glBegin(GL_LINES);
        glColor3fv(color);
        glVertex3fv(from);
        glVertex3fv(to);
        glEnd();
        glEnable(GL_DEPTH_TEST);
    }
    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int /* lifeTime */, const btVector3 &color) {
        drawLine(PointOnB, PointOnB + normalOnB * distance, color);
    }
    void reportErrorWarning(const char *warningString) {
        VPVL2_LOG(WARNING, warningString);
    }
    void draw3dText(const btVector3 & /* location */, const char *textString) {
        VPVL2_VLOG(1, textString);
    }
    void setDebugMode(int debugMode) {
        m_debugMode = debugMode;
    }
    int getDebugMode() const {
        return m_debugMode;
    }

    void drawRigidBody(const IModel *value, btDiscreteDynamicsWorld *world) {
        Array<IRigidBody *> rigidBodies;
        value->getRigidBodyRefs(rigidBodies);
        const int numRigidBodies = rigidBodies.count();
        for (int i = 0; i < numRigidBodies; i++) {
            IRigidBody *rigidBody = rigidBodies[i];
            btRigidBody *body = static_cast<btRigidBody *>(rigidBody->bodyPtr());
            world->debugDrawObject(body->getWorldTransform(), body->getCollisionShape(), Vector3(1, 0, 0));
        }
    }
    void drawModel(const IModel *model, btDiscreteDynamicsWorld *world) {
        Array<IBone *> bones;
        model->getBoneRefs(bones);
        const int nbones = bones.count();
        glDisable(GL_DEPTH_TEST);
        for (int i = 0; i < nbones; i++) {
            const IBone *bone = bones[i];
            if (bone->isVisible() && bone->parentBoneRef()) {
                glBegin(GL_LINES);
                glVertex3fv(bone->worldTransform().getOrigin());
                glVertex3fv(bone->parentBoneRef()->worldTransform().getOrigin());
                glColor3fv(Vector3(1, 0, 0));
                glEnd();
            }
        }
        drawRigidBody(model, world);
        glEnable(GL_DEPTH_TEST);
    }

private:
    int m_debugMode;
};

VPVL2_MAKE_SMARTPTR(DebugDrawer);

class Application {
public:
    Application()
        : m_window(0),
          m_world(new World()),
          m_scene(new Scene(true)),
          m_debugDrawer(new DebugDrawer()),
          m_width(0),
          m_height(0),
          m_restarted(glfwGetTime()),
          m_current(m_restarted),
          m_currentFPS(0),
          m_pressedKey(0),
          m_pressed(false),
          m_autoplay(false)
    {
    }
    ~Application() {
        m_debugDrawer.reset();
        /* release ApplicationContext for deleting OpenGL resources before destroying OpenGL context */
        m_applicationContext->release();
        m_scene.reset();
        /* explicitly release World instance first to ensure release btRigidBody */
        m_world.reset();
        m_dictionary.releaseAll();
        glfwDestroyWindow(m_window);
    }

    void updateFPS() {
        m_current = glfwGetTime();
        if (m_current - m_restarted > 1) {
#ifdef _MSC_VER
            _snprintf(title, sizeof(title), "libvpvl2 with GLFW (FPS:%d)", m_currentFPS);
#else
            snprintf(title, sizeof(title), "libvpvl2 with GLFW (FPS:%d)", m_currentFPS);
#endif
            glfwSetWindowTitle(m_window, title);
            m_restarted = m_current;
            m_currentFPS = 0;
        }
        m_currentFPS++;
    }
    bool initialize() {
        glfwSetErrorCallback(&Application::handleError);
        if (glfwInit() < 0) {
            std::cerr << "glfwInit() failed: " << std::endl;
            return false;
        }
        ::ui::loadSettings("config.ini", m_config);
        vsize width = m_width = m_config.value("window.width", 640),
                height = m_height = m_config.value("window.height", 480);
        int redSize = m_config.value("opengl.size.red", 8),
                greenSize = m_config.value("opengl.size.green", 8),
                blueSize = m_config.value("opengl.size.blue", 8),
                alphaSize = m_config.value("opengl.size.alpha", 8),
                depthSize = m_config.value("opengl.size.depth", 24),
                stencilSize = m_config.value("opengl.size.stencil", 8),
                samplesSize = m_config.value("opengl.size.samples", 4);
        bool enableAA = m_config.value("opengl.enable.aa", false),
                enableCoreProfile = m_config.value("opengl.enable.core", false),
                enableDebug = m_config.value("opengl.enable.debug", false);
        if (enableCoreProfile) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        }
        if (enableDebug) {
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        }
        glfwWindowHint(GLFW_RED_BITS, redSize);
        glfwWindowHint(GLFW_GREEN_BITS, greenSize);
        glfwWindowHint(GLFW_BLUE_BITS, blueSize);
        glfwWindowHint(GLFW_ALPHA_BITS, alphaSize);
        glfwWindowHint(GLFW_DEPTH_BITS, depthSize);
        glfwWindowHint(GLFW_STENCIL_BITS, stencilSize);
        glfwWindowHint(GLFW_SAMPLES, enableAA ? samplesSize : 0);
        m_window = glfwCreateWindow(width, height, "libvpvl2 with GLFW (FPS:N/A)", 0, 0);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetKeyCallback(m_window, &Application::handleKeyEvent);
        glfwSetMouseButtonCallback(m_window, &Application::handleMouseButton);
        glfwSetCursorPosCallback(m_window, &Application::handleCursorPosition);
        glfwSetScrollCallback(m_window, &Application::handleScroll);
        glfwSetWindowSizeCallback(m_window, &Application::handleWindowSize);
        if (!m_window) {
            std::cerr << "glfwCreateWindow() failed" << std::endl;
            return false;
        }
        glfwMakeContextCurrent(m_window);
        m_encoding.reset(new Encoding(&m_dictionary));
        m_applicationContext.reset(new ApplicationContext(m_scene.get(), m_encoding.get(), &m_config));
        Scene::initialize(m_applicationContext->sharedFunctionResolverInstance());
        std::cerr << "GL_VERSION:  " << glGetString(GL_VERSION) << std::endl;
        std::cerr << "GL_VENDOR:   " << glGetString(GL_VENDOR) << std::endl;
        std::cerr << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
        std::cerr << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        m_factory.reset(new Factory(m_encoding.get()));
        m_applicationContext->initializeOpenGLContext(enableDebug);
        m_autoplay = m_config.value("enable.playing", true);
#ifdef VPVL2_LINK_ASSIMP
        AntTweakBar::initialize(enableCoreProfile);
        m_controller.create(m_applicationContext.get());
#endif
        return true;
    }
    void load() {
        if (m_config.value("enable.opencl", false)) {
            m_scene->setAccelerationType(Scene::kOpenCLAccelerationType1);
        }
        m_scene->lightRef()->setToonEnable(m_config.value("enable.toon", true));
        if (m_config.value("enable.sm", false)) {
            int sw = m_config.value("sm.width", 2048);
            int sh = m_config.value("sm.height", 2048);
            m_applicationContext->createShadowMap(Vector3(sw, sh, 0));
            const Vector3 &direction = m_scene->lightRef()->direction(), &eye = -direction * 100, &center = direction * 100;
            const glm::mat4 &view = glm::lookAt(glm::vec3(eye.x(), eye.y(), eye.z()), glm::vec3(center.x(), center.y(), center.z()), glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 &projection = glm::infinitePerspective(45.0f, sw / float(sh), 0.1f);
            m_applicationContext->setLightMatrices(glm::mat4(), view, projection);
        }
        m_applicationContext->setViewportRegion(glm::ivec4(0, 0, m_width, m_height));
        ::ui::initializeDictionary(m_config, m_dictionary);
        ::ui::loadAllModels(m_config, m_applicationContext.get(), m_scene.get(), m_factory.get(), m_encoding.get());
        m_debugDrawer->setDebugMode(//btIDebugDraw::DBG_DrawAabb |
                                    //btIDebugDraw::DBG_DrawConstraints |
                                    //btIDebugDraw::DBG_DrawConstraintLimits |
                                    //btIDebugDraw::DBG_DrawContactPoints |
                                    //btIDebugDraw::DBG_DrawWireframe |
                                    0
                                    );
        m_world->dynamicWorldRef()->setDebugDrawer(m_debugDrawer.get());
        m_scene->setWorldRef(m_world->dynamicWorldRef());
        m_scene->seekTimeIndex(0, Scene::kUpdateAll);
        m_scene->update(Scene::kUpdateAll | Scene::kResetMotionState);
#if 0
        Array<IModel *> models;
        m_scene->getModelRefs(models);
        IModel *model = models[1];
        typedef std::set<const IBone *> BoneRefs;
        typedef std::map<const std::string, BoneRefs> MaterialRefs;
        MaterialRefs m;
        Array<IBone *> bones;
        Array<IMaterial *> materials;
        Array<IVertex *> vertices;
        model->getBoneRefs(bones);
        model->getMaterialRefs(materials);
        model->getVertexRefs(vertices);
        for (int i = 0; i < vertices.count(); i++) {
            const IVertex *vertex = vertices[i];
            const IMaterial *material = vertex->materialRef();
            const std::string s(reinterpret_cast<const char *>(material->name(IEncoding::kDefaultLanguage)->toByteArray()));
            MaterialRefs::iterator it = m.find(s);
            if (it != m.end()) {
                BoneRefs &b = it->second;
                for (int j = 0; j < 4; j++) {
                    const IBone *bone = vertex->boneRef(j);
                    if (bone->index() != -1) {
                        b.insert(bone);
                    }
                }
            }
            else {
                m.insert(std::make_pair(s, BoneRefs()));
            }
        }
        int i = 0;
        for (MaterialRefs::const_iterator it = m.begin(); it != m.end(); it++) {
            VPVL2_VLOG(1, i << " material=" << it->first);
            int j = 0;
            for (BoneRefs::const_iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                VPVL2_VLOG(1, j << " bone=" << (*it2)->name(IEncoding::kDefaultLanguage)->toByteArray());
                j++;
            }
            i++;
        }
#endif
    }
    bool isActive() const {
        return !glfwWindowShouldClose(m_window);
    }
    void handleFrame(double base, double &last, uint64 &oldTimeIndex) {
        m_applicationContext->renderShadowMap();
        m_applicationContext->renderOffscreen();
        ::ui::drawScreen(*m_scene.get());
        m_applicationContext->renderEffectParameterUIWidgets();
        double current = glfwGetTime();
        if (m_autoplay) {
            const IKeyframe::TimeIndex &newTimeIndex = IKeyframe::TimeIndex(uint64(((current - base) * 1000) / Scene::defaultFPS()));
            m_scene->seekTimeIndex(newTimeIndex, Scene::kUpdateAll);
            m_world->stepSimulation(newTimeIndex - oldTimeIndex, Scene::defaultFPS());
            m_scene->update(Scene::kUpdateAll & ~Scene::kUpdateCamera);
            updateFPS();
            oldTimeIndex = newTimeIndex;
            last = current;
        }
        else if (m_pressedKey == GLFW_KEY_SPACE) {
            m_scene->seekTimeIndex(last, Scene::kUpdateAll);
            m_world->dynamicWorldRef()->stepSimulation(1 / Scene::defaultFPS(), 2);
            m_scene->update(Scene::kUpdateAll & ~Scene::kUpdateCamera);
            last += 1;
        }
#if 0
        glm::mat4 m, v, p;
        m_applicationContext->getCameraMatrices(m, v, p);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(v));
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(p));
        m_world->dynamicWorldRef()->debugDrawWorld();
        Array<IModel *> models;
        m_scene->getModelRefs(models);
        if (models.count() > 0) {
            //m_debugDrawer->drawModel(models[0], m_world->dynamicWorldRef());
        }
#endif
        m_scene->update(Scene::kUpdateCamera);
        m_pressedKey = 0;
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

private:
    static void handleError(int err, const char *errstr) {
        std::cerr << "errno=" << err << " errstr=" << errstr << std::endl;
    }
    static void handleKeyEvent(GLFWwindow *window, int key, int /* scancode */, int action, int modifiers) {
        Application *context = static_cast<Application *>(glfwGetWindowUserPointer(window));
        ICamera *camera = context->m_scene->cameraRef();
        if (!context->m_applicationContext->handleKeyPress(key, modifiers) && action == GLFW_PRESS) {
            const Scalar degree(15.0);
            switch (key) {
            case GLFW_KEY_RIGHT:
                camera->setAngle(camera->angle() + Vector3(0, degree, 0));
                context->m_applicationContext->updateCameraMatrices();
                break;
            case GLFW_KEY_LEFT:
                camera->setAngle(camera->angle() + Vector3(0, -degree, 0));
                context->m_applicationContext->updateCameraMatrices();
                break;
            case GLFW_KEY_UP:
                camera->setAngle(camera->angle() + Vector3(degree, 0, 0));
                context->m_applicationContext->updateCameraMatrices();
                break;
            case GLFW_KEY_DOWN:
                camera->setAngle(camera->angle() + Vector3(-degree, 0, 0));
                context->m_applicationContext->updateCameraMatrices();
                break;
            case GLFW_KEY_ESCAPE:
                glfwWindowShouldClose(window);
                break;
            default:
                context->m_pressedKey = key;
                break;
            }
        }
    }
    static void handleMouseButton(GLFWwindow *window, int button, int action, int modifiers) {
        Application *context = static_cast<Application *>(glfwGetWindowUserPointer(window));
        bool pressed = action == GLFW_PRESS;
        IApplicationContext::MousePositionType type(IApplicationContext::kMouseCursorPosition);
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            type = IApplicationContext::kMouseLeftPressPosition;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            type = IApplicationContext::kMouseMiddlePressPosition;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            type = IApplicationContext::kMouseRightPressPosition;
            break;
        default:
            break;
        }
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        glm::vec4 v(x, y, pressed, 0);
        context->m_pressed = pressed;
        context->m_pressedModifier = pressed ? modifiers : 0;
        context->m_applicationContext->setMousePosition(v, type);
        context->m_applicationContext->handleMouse(v, type);
        context->m_lastCursorPosition = glm::vec2(v);
    }
    static void handleCursorPosition(GLFWwindow *window, double x, double y) {
        Application *context = static_cast<Application *>(glfwGetWindowUserPointer(window));
        glm::vec4 v(x, y, context->m_pressed, 0);
        context->m_applicationContext->setMousePosition(v, IApplicationContext::kMouseCursorPosition);
        if (!context->m_applicationContext->handleMouse(v, IApplicationContext::kMouseCursorPosition) && context->m_pressed) {
            ICamera *camera = context->m_scene->cameraRef();
            const Scalar &factor = 0.5;
            camera->setAngle(camera->angle() + Vector3((y - context->m_lastCursorPosition.y) * factor, (x - context->m_lastCursorPosition.x) * factor, 0));
            context->m_applicationContext->updateCameraMatrices();
            context->m_lastCursorPosition = glm::vec2(x, y);
        }
    }
    static void handleScroll(GLFWwindow *window, double x, double y) {
        Application *context = static_cast<Application *>(glfwGetWindowUserPointer(window));
        glm::vec4 v(x, y, context->m_pressed, 0);
        if (!context->m_applicationContext->handleMouse(v, IApplicationContext::kMouseWheelPosition)) {
            ICamera *camera = context->m_scene->cameraRef();
            const Scalar &factor = 1.0;
            camera->setDistance(camera->distance() + y * factor);
            const Matrix3x3 &m = camera->modelViewTransform().getBasis();
            if ((context->m_pressedModifier & GLFW_MOD_SHIFT) != 0) {
                const Vector3 &v = m[0] * x * factor;
                camera->setLookAt(camera->lookAt() + v);
            }
            context->m_applicationContext->updateCameraMatrices();
        }
    }
    static void handleWindowSize(GLFWwindow *window, int width, int height) {
        Application *context = static_cast<Application *>(glfwGetWindowUserPointer(window));
        context->m_width = width;
        context->m_height = height;
        context->m_applicationContext->setViewportRegion(glm::ivec4(0, 0, width, height));
        glViewport(0, 0, width, height);
    }

    GLFWwindow *m_window;
    StringMap m_config;
    Encoding::Dictionary m_dictionary;
    WorldSmartPtr m_world;
    EncodingSmartPtr m_encoding;
    FactorySmartPtr m_factory;
    SceneSmartPtr m_scene;
    ApplicationContextSmartPtr m_applicationContext;
    DebugDrawerSmartPtr m_debugDrawer;
    glm::vec2 m_lastCursorPosition;
    vsize m_width;
    vsize m_height;
    double m_restarted;
    double m_current;
    int m_currentFPS;
    int m_pressedModifier;
    int m_pressedKey;
    char title[32];
    bool m_pressed;
    bool m_autoplay;
};

} /* namespace anonymous */

int main(int /* argc */, char *argv[])
{
    Application application;
    tbb::task_scheduler_init initializer; (void) initializer;
    BaseApplicationContext::initializeOnce(argv[0], 0, 2);
    if (!application.initialize()) {
        BaseApplicationContext::terminate();
        return EXIT_FAILURE;
    }
    application.load();
    double base = glfwGetTime(), last = base;
    uint64 timeIndex = 0;
    glClearColor(0, 0, 1, 1);
    while (application.isActive()) {
        application.handleFrame(base, last, timeIndex);
    }
    BaseApplicationContext::terminate();
    return EXIT_SUCCESS;
}
