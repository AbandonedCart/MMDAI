/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2011  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                2010-2012  hkrn                                    */
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

#ifndef VPVL2_PMX_JOINT_H_
#define VPVL2_PMX_JOINT_H_

#include "vpvl2/pmx/RigidBody.h"

class btGeneric6DofConstraint;
class btGeneric6DofSpringConstraint;

namespace vpvl2
{
namespace pmx
{

/**
 * @file
 * @author Nagoya Institute of Technology Department of Computer Science
 * @author hkrn
 *
 * @section DESCRIPTION
 *
 * Constraint class represents a joint of a Polygon Model Data object.
 */

class VPVL2_API Joint
{
public:
    Joint();
    ~Joint();

    static bool preparse(uint8_t *&ptr, size_t &rest, Model::DataInfo &info);
    static bool loadJoints(const Array<Joint *> &joints,
                           const Array<RigidBody *> &rigidBodies);

    void read(const uint8_t *data, const Model::DataInfo &info, size_t &size);
    void write(uint8_t *data) const;

    RigidBody *rigidBody1() const { return m_rigidBody1; }
    RigidBody *rigidBody2() const { return m_rigidBody2; }
    const StaticString *name() const { return m_name; }
    const StaticString *englishName() const { return m_englishName; }

private:
    RigidBody *m_rigidBody1;
    RigidBody *m_rigidBody2;
    StaticString *m_name;
    StaticString *m_englishName;
    Vector3 m_position;
    Vector3 m_rotation;
    Vector3 m_positionLowerLimit;
    Vector3 m_rotationLowerLimit;
    Vector3 m_positionUpperLimit;
    Vector3 m_rotationUpperLimit;
    Vector3 m_positionStiffness;
    Vector3 m_rotationStiffness;
    int m_rigidBodyIndex1;
    int m_rigidBodyIndex2;

    VPVL2_DISABLE_COPY_AND_ASSIGN(Joint)
};

} /* namespace pmx */
} /* namespace vpvl2 */

#endif

