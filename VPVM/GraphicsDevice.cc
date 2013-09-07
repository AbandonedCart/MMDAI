/**

 Copyright (c) 2010-2013  hkrn

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

#include "GraphicsDevice.h"

#include <vpvl2/extensions/gl/CommonMacros.h>

GraphicsDevice::GraphicsDevice(QObject *parent) :
    QObject(parent)
{
}

GraphicsDevice::~GraphicsDevice()
{
}

void GraphicsDevice::initialize()
{
    m_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    m_vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    m_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    m_shadingLanguage = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    const QString extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    foreach (const QString &extension, extensions.split(" ")) {
        m_extensions.append(extension.trimmed());
    }
    VPVL2_LOG(INFO, "GL_VERSION: " << m_version.toStdString());
    VPVL2_LOG(INFO, "GL_VENDOR: " << m_vendor.toStdString());
    VPVL2_LOG(INFO, "GL_RENDERER: " << m_renderer.toStdString());
    VPVL2_LOG(INFO, "GL_SHADING_LANGUAGE_VERSION: " << m_shadingLanguage.toStdString());
}

QString GraphicsDevice::extensionsText() const
{
    return m_extensions.join("\n");
}
