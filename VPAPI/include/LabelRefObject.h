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

#ifndef LABELREFOBJECT_H
#define LABELREFOBJECT_H

#include <QJsonValue>
#include <QObject>
#include <QQmlListProperty>
#include <vpvl2/ILabel.h>

class BoneRefObject;
class ModelProxy;
class MorphRefObject;

class LabelRefObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ModelProxy *parentModel READ parentModel CONSTANT FINAL)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(int index READ index CONSTANT FINAL)
    Q_PROPERTY(QQmlListProperty<BoneRefObject> bones READ bones NOTIFY bonesChanged FINAL)
    Q_PROPERTY(QQmlListProperty<MorphRefObject> morphs READ morphs NOTIFY morphsChanged FINAL)
    Q_PROPERTY(bool special READ isSpecial WRITE setSpecial NOTIFY specialChanged FINAL)
    Q_PROPERTY(bool dirty READ isDirty NOTIFY dirtyChanged)

public:
    LabelRefObject(ModelProxy *modelRef, vpvl2::ILabel *labelRef);
    ~LabelRefObject();

    void addBone(BoneRefObject *bone);
    void addMorph(MorphRefObject *morph);
    void removeBone(BoneRefObject *bone);
    void removeMorph(MorphRefObject *morph);

    Q_INVOKABLE void addObject(QObject *value);
    Q_INVOKABLE void removeObject(QObject *value);
    Q_INVOKABLE QJsonValue toJson() const;

    vpvl2::ILabel *data() const;
    ModelProxy *parentModel() const;
    QString name() const;
    void setName(const QString &value);
    int index() const;
    QQmlListProperty<BoneRefObject> bones();
    QQmlListProperty<MorphRefObject> morphs();
    bool isSpecial() const;
    void setSpecial(bool value);
    bool isDirty() const;
    void setDirty(bool value);

signals:
    void nameChanged();
    void specialChanged();
    void bonesChanged();
    void morphsChanged();
    void dirtyChanged();

private:
    ModelProxy *m_parentModelRef;
    vpvl2::ILabel *m_labelRef;
    QList<BoneRefObject *> m_bones;
    QList<MorphRefObject *> m_morphs;
    bool m_dirty;
};

#endif // LABELREFOBJECT_H
