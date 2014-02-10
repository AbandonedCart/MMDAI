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
import qbs.FileInfo
import qbs.TextFile

Product {
    id: libvpvl2
    readonly property string libraryBuildDirectory: "build-" + qbs.buildVariant.toLowerCase()
    readonly property string libraryInstallDirectory: libraryBuildDirectory + "/install-root"
    readonly property string debugLibrarySuffix: qbs.enableDebugCode ? "d" : ""
    readonly property string assimpLibrarySuffix: qbs.toolchain.contains("msvc") ? "" : debugLibrarySuffix.toUpperCase()
    readonly property string nvFXLibrarySuffix: (cpp.architecture === "x86_64" ? "64" : "") + debugLibrarySuffix.toUpperCase()
    readonly property var commonFiles: [
        "src/core/asset/*.cc",
        "src/core/base/*.cc",
        "src/core/internal/*.cc",
        "src/core/mvd/*.cc",
        "src/core/pmd2/*.cc",
        "src/core/pmx/*.cc",
        "src/core/vmd/*.cc",
        "src/engine/fx/*.cc",
        "src/engine/gl2/*.cc",
        "src/engine/nvfx/*.cc",
        "src/ext/Archive.cc",
        "src/ext/BaseApplicationContext.cc",
        "src/ext/StringMap.cc",
        "src/ext/World.cc",
        "src/ext/XMLProject.cc",
        "include/**/*.h",
        "include/vpvl2/config.h.in",
        "vendor/nvFX/*.cc",
        "vendor/SOIL/*.c",
        "vendor/minizip-1.1/*.c",
        "vendor/tinyxml2-1.0.11/*.cpp"
    ].map(function(x){ return FileInfo.joinPaths(sourceDirectory, x) })
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
    readonly property var commonIncludePaths: [
        "include",
        "vendor/cl-1.2",
        "vendor/nvFX",
        "vendor/SOIL",
        "vendor/minizip-1.1",
        "vendor/tinyxml2-1.0.11",
        "../glm-src",
        "../bullet-src/" + libraryInstallDirectory + "/include/bullet",
        "../assimp-src/" + libraryInstallDirectory + "/include",
        "../nvFX-src/" + libraryInstallDirectory + "/include",
        "../zlib-src/" + libraryInstallDirectory + "/include",
        "../AntTweakBar-src/include",
        "../tbb-src/include"
    ].map(function(x){ return FileInfo.joinPaths(sourceDirectory, x) }).concat(buildDirectory)
    readonly property var commonLibraryPaths: [
        "../bullet-src",
        "../assimp-src",
        "../nvFX-src",
        "../zlib-src",
    ].map(function(x){ return FileInfo.joinPaths(sourceDirectory, x, libraryInstallDirectory, "lib") }).concat([ "../tbb-src/lib" ])
    readonly property var commonConfigDefinitions: [
        "VPVL2_ENABLE_OPENGL",
        "VPVL2_COORDINATE_OPENGL",
        "VPVL2_LINK_ASSIMP3",
        "VPVL2_LINK_NVFX",
        "VPVL2_ENABLE_EXTENSIONS_ARCHIVE",
        "VPVL2_ENABLE_EXTENSIONS_APPLICATIONCONTEXT",
        "VPVL2_ENABLE_EXTENSIONS_PROJECT",
        "VPVL2_ENABLE_EXTENSIONS_WORLD"
    ].concat(qbs.enableDebugCode ? [ "VPVL2_ENABLE_DEBUG_ANNOTATIONS", "BUILD_SHARED_LIBS" ] : [])
    readonly property var configDefinitions: commonConfigDefinitions
    type: qbs.buildVariant === "debug" ? "dynamiclibrary" : "staticlibrary"
    name: "vpvl2"
    version: {
        var file = new TextFile(sourceDirectory + "/CMakeLists.txt", TextFile.ReadOnly), v = {}
        while (!file.atEof()) {
            var line = file.readLine()
            if (line.match(/(VPVL2_VERSION_\w+)\s+(\d+)/)) {
                v[RegExp.$1] = RegExp.$2
            }
        }
        return [ v["VPVL2_VERSION_MAJOR"], v["VPVL2_VERSION_COMPAT"], v["VPVL2_VERSION_MINOR"] ].join(".")
    }
    files: commonFiles
    cpp.defines: {
        var defines = [ "VPVL2_ENABLE_QT", "USE_FILE32API", "TW_STATIC", "TW_NO_LIB_PRAGMA", "STBI_NO_STDIO", "STBI_NO_WRITE" ]
        if (qbs.enableDebugCode && qbs.toolchain.contains("msvc")) {
            defines.push("vpvl2_EXPORTS")
        }
        return defines
    }
    cpp.includePaths: commonIncludePaths
    cpp.libraryPaths: commonLibraryPaths
    Transformer {
        inputs: sourceDirectory + "/include/vpvl2/config.h.in"
        Artifact {
            fileName: "vpvl2/config.h"
            fileTags: "vpvl2_config"
        }
        prepare: {
            var command = new JavaScriptCommand()
            var newConfigDefinitions = product.configDefinitions
            command.description = "Proceeding " + input.fileName + "..."
            command.highlight = "codegen"
            command.configDefinitions = newConfigDefinitions
            command.versionString = product.version
            command.sourceCode = function() {
                var inputFile = new TextFile(input.fileName)
                var content = inputFile.readAll()
                inputFile.close()
                var outputFile = new TextFile(output.fileName, TextFile.WriteOnly)
                for (var i in configDefinitions) {
                    var value = configDefinitions[i]
                    if (value) {
                        var regexp = new RegExp("#cmakedefine\\s+" + value, "gm")
                        content = content.replace(regexp, "#define " + value)
                    }
                }
                var file = new TextFile(product.sourceDirectory + "/CMakeLists.txt", TextFile.ReadOnly), cmakeVariables = {}
                while (!file.atEof()) {
                    var line = file.readLine()
                    if (line.match(/(VPVL2_COMMIT_REVISION)_ID\s+"\$Id:\s+(\w+)\s+\$"/) ||
                            line.match(/(VPVL2_VERSION_\w+)\s+(\d+)/)) {
                        cmakeVariables[RegExp.$1] = RegExp.$2
                    }
                }
                content = content.replace(/#cmakedefine\s+(\w+)/gm, "/* #undef $1 */")
                content = content.replace(/@VPVL2_VERSION@/gm, versionString)
                content = content.replace(/@(VPVL2_\w+)@/gm, function(match, p1) { return cmakeVariables[p1] })
                outputFile.truncate()
                outputFile.write(content)
                outputFile.close()
            }
            return command
        }
    }
    Properties {
        condition: qbs.targetOS.contains("osx")
        configDefinitions: commonConfigDefinitions.concat(["VPVL2_OS_OSX", "VPVL2_LINK_ATB", "VPVL2_ENABLE_OPENCL", "VPVL2_LINK_INTEL_TBB"])
        cpp.dynamicLibraries: commonLibraries.concat([ "tbb", "z" ])
        cpp.frameworks: [ "AppKit", "OpenGL", "OpenCL" ]
        cpp.minimumOsxVersion: "10.6"
    }
    Properties {
        condition: qbs.targetOS.contains("linux")
        configDefinitions: commonConfigDefinitions.concat(["VPVL2_OS_LINUX", "VPVL2_LINK_ATB", "VPVL2_LINK_INTEL_TBB"])
    }
    Properties {
        condition: qbs.targetOS.contains("ios")
        configDefinitions: commonConfigDefinitions.concat(["VPVL2_OS_IOS"])
    }
    Properties {
        condition: qbs.targetOS.contains("android")
        configDefinitions: commonConfigDefinitions.concat(["VPVL2_OS_ANDROID"])
    }
    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.dynamicLibraries: commonLibraries.concat([ "OpenGL32" ])
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        configDefinitions: commonConfigDefinitions.concat(["VPVL2_OS_WINDOWS", "VPVL2_ENABLE_GLES2", "VPVL2_LINK_ATB", "VPVL2_LINK_EGL"])
        cpp.cxxFlags: [ "/wd4068", "/wd4355", "/wd4819" ]
        cpp.dynamicLibraries: commonLibraries.concat([ "user32" ]).concat([ "libGLESv2", "libEGL", "zlibstatic" ].map(function(x){ return x + debugLibrarySuffix }))
    }
    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        cpp.dynamicLibraries: commonLibraries.concat([ "Xext", "X11", "tbb", "z", "GL" ])
    }
    Group {
        condition: qbs.targetOS.contains("osx")
        name: "OSX Extension"
        files: [ "src/engine/cl/*.cc" ].map(function(x){ return FileInfo.joinPaths(sourceDirectory, x) })
    }
    Depends { name: "cpp" }
    Depends {
        name: "AntTweakBar"
        condition: !qbs.targetOS.contains("ios")
    }
    Depends {
        name: "Qt"
        submodules: [ "core", "gui" ]
    }
}
