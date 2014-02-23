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

import qbs 1.0
import qbs.File
import qbs.FileInfo
import qbs.TextFile
import "../VPVM/VPVM.qbs.js" as vpvm

CppApplication {
    id: VPMM
    readonly property string applicationBundlePath: "VPMM.app/Contents"
    readonly property string libraryBuildDirectory: "build-" + qbs.buildVariant.toLowerCase()
    readonly property string libraryInstallDirectory: libraryBuildDirectory + "/install-root"
    readonly property string debugLibrarySuffix: qbs.enableDebugCode ? "d" : ""
    readonly property string assimpLibrarySuffix: qbs.toolchain.contains("msvc") ? "" : debugLibrarySuffix.toUpperCase()
    readonly property string nvFXLibrarySuffix: (cpp.architecture === "x86_64" ? "64" : "") + debugLibrarySuffix.toUpperCase()
    readonly property string sparkleFrameworkBasePath: sourceDirectory + "/../Sparkle-src/build/Release"
    readonly property var commonLibraries: [
        "assimp" + assimpLibrarySuffix,
        "FxParser" + nvFXLibrarySuffix,
        "FxLibGL" + nvFXLibrarySuffix,
        "FxLib" + nvFXLibrarySuffix,
        "BulletSoftBody",
        "BulletDynamics",
        "BulletCollision",
        "LinearMath"
    ]
    readonly property var commonIncludePaths: [ buildDirectory ].concat([
        "include",
        "../VPAPI/include",
        "../libvpvl2/include",
        "../bullet-src/" + libraryInstallDirectory + "/include/bullet",
        "../glm-src",
        "../assimp-src/include",
        "../alure-src/include",
        "../tbb-src/include",
        "../libgizmo-src/inc",
        "../openal-soft-src/" + libraryInstallDirectory + "/include"
    ].map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) }))
    readonly property var commonFiles: [
        "src/*.cc",
        "include/*.h",
        "../VPVM/licenses/licenses.qrc",
        "../libvpvl2/src/resources/resources.qrc"
    ].map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) })
    readonly property var commonDefiles: [
        "VPVL2_ENABLE_QT",
        "TW_STATIC",
        "TW_NO_LIB_PRAGMA"
    ]
    readonly property var requiredSubmodules: [
        "core", "gui", "widgets", "qml", "quick", "multimedia", "network"
    ]
    type: (qbs.targetOS.contains("osx") && !qbs.enableDebugCode) ? "applicationbundle" : "application"
    name: "VPMM"
    version: {
        var file = new TextFile(sourceDirectory + "/../libvpvl2/CMakeLists.txt", TextFile.ReadOnly), v = {}
        while (!file.atEof()) {
            var line = file.readLine()
            if (line.match(/(VPVL2_VERSION_\w+)\s+(\d+)/)) {
                v[RegExp.$1] = RegExp.$2
            }
        }
        return [ v["VPVL2_VERSION_MAJOR"], v["VPVL2_VERSION_COMPAT"], "3" ].join(".")
    }
    files: commonFiles
    cpp.defines: commonDefiles
    cpp.includePaths: commonIncludePaths
    cpp.libraryPaths: [
        "../tbb-src/lib",
        "../bullet-src/" + libraryInstallDirectory + "/lib",
        "../assimp-src/" + libraryInstallDirectory + "/lib",
        "../nvFX-src/" + libraryInstallDirectory + "/lib",
        "../alure-src/" + libraryInstallDirectory + "/lib",
        "../openal-soft-src/" + libraryInstallDirectory + "/lib",
        "../icu4c-src/" + libraryInstallDirectory + "/lib",
        "../zlib-src/" + libraryInstallDirectory + "/lib"
    ].map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) })
    Qt.quick.qmlDebugging: qbs.enableDebugCode
    Group {
        name: "Application"
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: qbs.targetOS.contains("osx") ? FileInfo.joinPaths(applicationBundlePath, "MacOS") : ""
    }
    Group {
        name: "Application Translation Resources"
        files: [
            "translations/*.qm",
            Qt.core.binPath + "/../translations/qt*.qm",
        ]
        qbs.install: true
        qbs.installDir: qbs.targetOS.contains("osx") ? FileInfo.joinPaths(applicationBundlePath, "Resources", "translations") : "translations"
    }
    Group {
        condition: qbs.buildVariant === "release"
        name: "Application Resources for Release Build"
        files: {
            var files = [ "qml/VPMM.qrc" ]
            if (qbs.targetOS.contains("windows")) {
                files.push("qt/win32.qrc")
            }
            else if (qbs.targetOS.contains("osx")) {
                files.push("qt/osx.qrc")
            }
            else if (qbs.targetOS.contains("linux")) {
                files.push("qt/linux.qrc")
            }
            return files.map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) })
        }
    }
    Group {
        name: "Application Resources for Debug Build"
        files: FileInfo.joinPaths(sourceDirectory, "qml/VPMM/*")
        qbs.install: qbs.buildVariant === "debug"
        qbs.installDir: qbs.targetOS.contains("osx") ? applicationBundlePath + "/Resources/qml/VPMM" : "qml/VPMM"
    }
    Properties {
        condition: qbs.targetOS.contains("osx")
        cpp.frameworks: [ "AppKit", "OpenGL", "OpenCL" ]
        cpp.frameworkPaths: [ sparkleFrameworkBasePath ]
        cpp.weakFrameworks: {
            var frameworks = []
            if (File.exists(sparkleFrameworkBasePath + "/Sparkle.framework")) {
                frameworks.push("Sparkle")
            }
            return frameworks
        }
        cpp.dynamicLibraries: commonLibraries.concat([ "alure-static", "openal", "tbb", "z" ])
        cpp.minimumOsxVersion: "10.6"
        cpp.infoPlistFile: "qt/osx/Info.plist"
        cpp.infoPlist: ({
                            "CFBundleVersion": version,
                            "CFBundleShortVersionString": version
                        })
    }
    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        cpp.dynamicLibraries: commonLibraries.concat([ "alure-static", "openal", "Xext", "X11", "tbb", "z", "GL"])
        cpp.rpaths: [ "$ORIGIN/lib", "$ORIGIN" ]
        cpp.positionIndependentCode: true
    }
    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.includePaths: commonIncludePaths.concat([
                                                        "../alure-src/include/AL",
                                                        "../openal-soft-src/" + libraryInstallDirectory + "/include/AL"
                                                    ].map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) }))
        cpp.dynamicLibraries: commonLibraries.concat([ "alure32-static",  "OpenAL32", "OpenGL32", "zlibstatic" + debugLibrarySuffix ])
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        consoleApplication: false
        cpp.cxxFlags: [ "/wd4068", "/wd4355", "/wd4819" ]
        cpp.includePaths: commonIncludePaths.concat([
                                                        "../alure-src/include/AL",
                                                        "../openal-soft-src/" + libraryInstallDirectory + "/include/AL"
                                                    ].map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) }))
        cpp.dynamicLibraries: commonLibraries.concat([ "alure32-static", "OpenAL32", "libGLESv2" + debugLibrarySuffix, "libEGL" + debugLibrarySuffix, "zlibstatic" + debugLibrarySuffix, "user32" ])
    }
    Group {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        name: "Application Depending Libraries"
        files: {
            var found = vpvm.findLibraries(commonLibraries.concat([ "openal", "tbb", "z" ]), cpp.libraryPaths, ".so*")
            if (qbs.targetOS.contains("linux")) {
                requiredSubmodules.push("dbus")
            }
            for (var i in requiredSubmodules) {
                var requiredSubmodule = requiredSubmodules[i]
                var name = requiredSubmodule.toUpperCase().charAt(0) + requiredSubmodule.substring(1)
                found.push(FileInfo.joinPaths(Qt.core.libPath, "libQt5" + name + ".so.5*"))
            }
            found.push(FileInfo.joinPaths(product.buildDirectory, "libvpvl2.so*"))
            found.push(FileInfo.joinPaths(Qt.core.libPath, "libicu*.so.*"))
            return found
        }
        qbs.install: true
        qbs.installDir: "lib"
    }
    Group {
        condition: qbs.toolchain.contains("msvc")
        name: "Application Depending Libraries for MSVC"
        files: {
            var found = vpvm.findLibraries(commonLibraries.concat([ "OpenAL32", "vpvl2" ]),
                                           cpp.libraryPaths.concat([ "../openal-soft-src/" + libraryInstallDirectory + "/bin", buildDirectory ]
                                                                   .map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) })),
                                           ".dll")
            if (qbs.toolchain.contains("msvc")) {
                for (var i in requiredSubmodules) {
                    var requiredSubmodule = requiredSubmodules[i]
                    var name = requiredSubmodule.toUpperCase().charAt(0) + requiredSubmodule.substring(1)
                    found.push(FileInfo.joinPaths(Qt.core.binPath, "Qt5" + name + debugLibrarySuffix + ".dll"))
                }
                found.push(FileInfo.joinPaths(product.buildDirectory, "libvpvl2.so*"))
                var thirdPartyLibraries = [ "d3dcompiler_46", "icu*" + debugLibrarySuffix, "libEGL" + debugLibrarySuffix, "libGLESv2" + debugLibrarySuffix ]
                for (var i in thirdPartyLibraries) {
                    found.push(FileInfo.joinPaths(Qt.core.binPath, thirdPartyLibraries[i] + ".dll"))
                }
            }
            return found
        }
        qbs.install: true
    }
    Group {
        name: "QtQuick QML Resources"
        condition: !qbs.targetOS.contains("osx")
        files: [ "QtQuick", "QtQuick.2" ].map(function(path){ return FileInfo.joinPaths(Qt.core.binPath, "../qml", path) })
        qbs.install: qbs.buildVariant === "release"
        qbs.installDir: "qml"
    }
    Group {
        name: "Qt plugins"
        condition: !qbs.targetOS.contains("osx")
        files: {
            var plugins = [
                        "accessible",
                        "bearer",
                        "iconengines",
                        "imageformats",
                        "mediaservice",
                        "platforms",
                        // "printsupport",
                        "sensorgestures",
                        "sensors"
                    ]
            if (!qbs.targetOS.contains("ios")) {
                plugins.push("position")
                if (!qbs.targetOS.contains("windows")) {
                    plugins.push("audio")
                    if (!qbs.targetOS.contains("osx")) {
                        plugins.push("generic" /* , "platforminputcontexts", "platformthemes" */)
                    }
                }
            }
            return plugins.map(function(item){ return FileInfo.joinPaths(Qt.core.pluginPath, item) })
        }
        excludeFiles: [ "**/q*d.dll", "**/*.pdb" ]
        qbs.install: qbs.buildVariant === "release"
        qbs.installDir: "plugins"
    }
    Group {
        name: "OSX Extensions"
        condition: qbs.targetOS.contains("osx")
        files: [ "src/*.mm" ].map(function(path){ return FileInfo.joinPaths(sourceDirectory, path) })
    }
    Depends { name: "AntTweakBar"; condition: !qbs.targetOS.contains("ios") }
    Depends { name: "gizmo" }
    Depends { name: "VPAPI" }
    Depends { name: "vpvl2" }
    Depends { name: "Qt"; submodules: requiredSubmodules }
}
