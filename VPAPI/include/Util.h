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

#ifndef UTIL_H
#define UTIL_H

#include <QColor>
#include <QJsonValue>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QString>
#include <QVector3D>
#include <QVector4D>

#include <vpvl2/Common.h>
#include <glm/mat4x4.hpp>

namespace vpvl2 {
class IString;
}

class Util {
public:
    struct LessThan {
        template<typename TObject>
        inline bool operator()(const TObject *left, const TObject *right) {
            return left->index() < right->index();
        }
    };

    static const QColor kRed;
    static const QColor kGreen;
    static const QColor kBlue;
    static const QColor kYellow;
    static const QColor kGray;

    static QString toQString(const vpvl2::IString *value);
    static bool equalsString(const QString lhs, const vpvl2::IString *rhs);
    static QMatrix4x4 fromMatrix4(const glm::mat4 &value);
    static vpvl2::Vector3 toVector3(const QVector3D &value);
    static QVector3D fromVector3(const vpvl2::Vector3 &value);
    static vpvl2::Vector4 toVector4(const QVector4D &value);
    static QVector4D fromVector4(const vpvl2::Vector4 &value);
    static vpvl2::Vector3 toColorRGB(const QColor &value);
    static QColor fromColorRGB(const vpvl2::Vector3 &value);
    static vpvl2::Color toColorRGBA(const QColor &value);
    static QColor fromColorRGBA(const vpvl2::Color &value);
    static vpvl2::Quaternion toQuaternion(const QQuaternion &value);
    static QQuaternion fromQuaternion(const vpvl2::Quaternion &value);
    static QString resourcePath(const QString &basePath);
    static QJsonValue toJson(const QColor &value);
    static QJsonValue toJson(const QVector3D &value);
    static QJsonValue toJson(const QVector4D &value);
    static QJsonValue toJson(const QQuaternion &value);

private:
    Util();
    ~Util();
};

#endif // UTIL_H
