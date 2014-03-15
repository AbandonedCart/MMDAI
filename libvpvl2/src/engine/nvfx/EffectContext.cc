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

#include "vpvl2/vpvl2.h"
#include "vpvl2/IApplicationContext.h"

#include "vpvl2/gl/Global.h"
#include "vpvl2/nvfx/Effect.h"
#include "vpvl2/nvfx/EffectContext.h"
#include "vpvl2/internal/util.h"

#include "../vendor/nvFX/GL/glew.h"

/* prevent compile error */
#ifndef GLhandleARB
#define GLhandleARB void *
#endif
#include <FxParser.h>

namespace {

using namespace vpvl2::VPVL2_VERSION_NS;
using namespace vpvl2::VPVL2_VERSION_NS::extensions;

VPVL2_DECL_TLS static bool g_initialized = false;
VPVL2_DECL_TLS static StringMap g_includeBuffers;
VPVL2_DECL_TLS static StringList g_includePaths;

static void handleIncludeCallback(const char *incName, FILE *&fp, const char *&buf)
{
    StringMap::const_iterator it = g_includeBuffers.find(incName);
    VPVL2_VLOG(2, "include=" << incName);
    if (it != g_includeBuffers.end()) {
        fp = 0;
        buf = it->second.c_str();
    }
    else {
        std::string s;
        for (StringList::const_iterator it = g_includePaths.begin(), end = g_includePaths.end(); it != end; it++) {
            s.assign(*it);
            s.append("/");
            s.append(incName);
            VPVL2_VLOG(2, "include=" << s);
            if (FILE *f = fopen(s.c_str(), "r")) {
                fp = f;
                break;
            }
        }
        fp = 0;
        buf = "";
    }
}

static void handleErrorCallback(const char *message)
{
    VPVL2_LOG(WARNING, message);
}

static void handleMessageCallback(const char *message)
{
    VPVL2_LOG(INFO, message);
}

static void discardsMessageCallback(const char * /* message */)
{
}

struct FunctionResolverProxy : nvFX::FunctionResolver {
    FunctionResolverProxy(const IApplicationContext::FunctionResolver *resolver)
        : m_resolver(resolver)
    {
    }
    ~FunctionResolverProxy() {
    }

    bool hasExtension(const char *name) const {
        return m_resolver->hasExtension(name);
    }
    void *resolve(const char *name) const {
        return m_resolver->resolveSymbol(name);
    }
    int queryVersion() const {
        return m_resolver->query(IApplicationContext::FunctionResolver::kQueryVersion);
    }

    const IApplicationContext::FunctionResolver *m_resolver;
};

}

namespace vpvl2
{
namespace VPVL2_VERSION_NS
{
namespace nvfx
{

bool EffectContext::initialize(const IApplicationContext::FunctionResolver *resolver)
{
    if (!g_initialized) {
        FunctionResolverProxy proxy(resolver);
        nvFX::initialize();
        nvFX::initializeOpenGLFunctions(&proxy);
        g_initialized = true;
    }
    return g_initialized;
}

void EffectContext::cleanup()
{
    if (g_initialized) {
        nvFX::cleanup();
        g_initialized = false;
    }
}

void EffectContext::enableIncludeCallback(const StringMap &includeBuffers, const StringList &includePaths)
{
    g_includeBuffers = includeBuffers;
    g_includePaths = includePaths;
    nvFX::setIncludeCallback(handleIncludeCallback);
}

void EffectContext::disableIncludeCallback()
{
    g_includeBuffers.clear();
    g_includePaths.clear();
    nvFX::setIncludeCallback(0);
}

void EffectContext::enableMessageCallback()
{
    nvFX::setErrorCallback(handleErrorCallback);
    nvFX::setMessageCallback(handleMessageCallback);
}

void EffectContext::disableMessageCallback()
{
    nvFX::setErrorCallback(discardsMessageCallback);
    nvFX::setMessageCallback(discardsMessageCallback);
}

EffectContext::EffectContext()
{
    enableMessageCallback();
}

EffectContext::~EffectContext()
{
}

void EffectContext::getEffectArguments(const IApplicationContext * /* applicationContextRef */, Array<const char *> &arguments)
{
    arguments.clear();
}

IEffect *EffectContext::compileFromFile(const IString *pathRef, IApplicationContext *applicationContextRef)
{
    nvFX::IContainer *container = 0;
    if (pathRef) {
        container = nvFX::IContainer::create();
        if (nvFX::loadEffectFromFile(container, reinterpret_cast<const char *>(pathRef->toByteArray()))) {
            return new nvfx::Effect(this, applicationContextRef, container, pathRef);
        }
    }
    return 0;
}

IEffect *EffectContext::compileFromSource(const IString *source, IApplicationContext *applicationContextRef)
{
    nvFX::IContainer *container = 0;
    if (source) {
        container = nvFX::IContainer::create();
        if (nvFX::loadEffect(container, reinterpret_cast<const char *>(source->toByteArray()))) {
            return new nvfx::Effect(this, applicationContextRef, container, 0);
        }
    }
    return 0;
}

void *EffectContext::internalContext() const
{
    return 0;
}

} /* namespace nvfx */
} /* namespace VPVL2_VERSION_NS */
} /* namespace vpvl2 */
