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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import com.github.mmdai.VPVM 1.0 as VPVM
import "FontAwesome.js" as FontAwesome

ApplicationWindow {
    id: applicationWindow
    readonly property bool isOSX: Qt.platform.os === "osx"
    readonly property int applicationLayoutAnchorMargin : 10
    minimumWidth: 960
    minimumHeight: 620
    title: "%1 - %2".arg(Qt.application.name).arg(scene.project.title)

    function __handleApplicationStateChange() {
        var state = Qt.application.state
        scene.state = state === Qt.ApplicationActive ? scene.lastStateAtSuspend : "suspend"
    }
    function __handleRequestedFileUrlChange() {
        var fileUrl = applicationBootstrapOption.requestedFileUrl, fileUrlString = fileUrl.toString()
        if (/\.(?:pm[xd]|x)$/i.test(fileUrlString)) {
            scene.project.loadModel(fileUrl)
        }
        else if (/\.(?:vmd|mvd)$/.test(fileUrlString)) {
            scene.project.loadMotion(fileUrl, scene.currentModel, VPVM.Project.ModelMotion)
        }
        else if (/\.vpd$/.test(fileUrlString)) {
            scene.project.loadPose(fileUrl, scene.currentModel)
        }
    }
    function updateWindowRect() {
        x = applicationPreference.windowRect.x
        y = applicationPreference.windowRect.y
        width = applicationPreference.windowRect.width
        height = applicationPreference.windowRect.height
    }
    function exitApplication() {
        scene.state = "suspend"
        applicationPreference.windowRectChanged.disconnect(updateWindowRect)
        applicationPreference.windowRect.x = x
        applicationPreference.windowRect.y = y
        applicationPreference.windowRect.width = width
        applicationPreference.windowRect.height = height
        applicationPreference.sync()
        Qt.quit()
    }
    MessageDialog {
        id: confirmSavingProjectBeforeClosingDialog
        title: qsTr("The project has been modified.")
        text: qsTr("Do you want to save your changes of %1?").arg(scene.project.title)
        icon: StandardIcon.Question
        standardButtons: StandardButton.Save | StandardButton.Discard | StandardButton.Cancel
        onAccepted: {
            saveProjectAction.trigger()
            exitApplication()
        }
        onDiscard: exitApplication()
    }
    onClosing: {
        if (scene.project.dirty) {
            close.accepted = false
            confirmSavingProjectBeforeClosingDialog.open()
        }
    }
    Component.onCompleted: {
        scene.project.initializeOnce()
        updateWindowRect()
        applicationPreference.windowRectChanged.connect(updateWindowRect)
        applicationBootstrapOption.requestedFileUrlChanged.connect(__handleRequestedFileUrlChange)
        Qt.application.stateChanged.connect(__handleApplicationStateChange)
    }

    ApplicationWindow {
        id: timelineWindow
        width: timelineContainer.width
        height: timelineContainer.height
        flags: isOSX ? Qt.Window | Qt.WindowFullscreenButtonHint : Qt.Window
        onClosing: {
            scene.state = "attachedTimeline"
            close.accepted = true
        }
        Item {
            id: windowedTimeline
            anchors.fill: parent
        }
    }
    ApplicationWindow {
        id: progressWindow
        property string text
        property real maximumValue
        property real minimumValue
        width: 350
        height: 80
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: applicationLayoutAnchorMargin
            Layout.alignment: Qt.AlignCenter
            Text {
                Layout.fillWidth: true
                text: progressWindow.text
            }
            ProgressBar {
                Layout.fillWidth: true
                minimumValue: progressWindow.minimumValue
                maximumValue: progressWindow.maximumValue
            }
            Item { Layout.fillHeight: true }
        }
    }

    WindowLoader { id: globalPreferenceWindowLoader; loaderSource: Qt.resolvedUrl("GlobalPreference.qml") }
    WindowLoader { id: projectPreferenceWindowLoader; loaderSource: Qt.resolvedUrl("ProjectPreference.qml") }
    WindowLoader { id: aboutWindowLoader; loaderSource: Qt.resolvedUrl("AboutWindow.qml") }
    FontLoader { id: fontAwesome; source: "FontAwesome.%1".arg(isOSX ? "otf" : "ttf") }

    MessageDialog {
        id: confirmSavingProjectBeforeNewProjectDialog
        title: qsTr("The project has been modified.")
        text: qsTr("Do you want to save your changes of %1?").arg(scene.project.title)
        icon: StandardIcon.Question
        standardButtons: StandardButton.Save | StandardButton.Discard | StandardButton.Cancel
        onAccepted: {
            saveProjectAction.trigger()
            scene.project.createAsync()
        }
        onDiscard: scene.project.createAsync()
    }
    Action {
        id: newProjectAction
        text: qsTr("New Project")
        shortcut: StandardKey.New
        tooltip: qsTr("Create a new project. If the project exists, it will be deleted and undone.")
        onTriggered: {
            if (scene.project.dirty) {
                confirmSavingProjectBeforeNewProjectDialog.open()
            }
            else {
                scene.project.createAsync()
            }
        }
    }
    Action {
        id: newMotionAction
        tooltip: qsTr("Create a new motion to the current model. If the model is bound to the exist motion, it will be deleted and undone.")
        text: qsTr("New Motion")
        onTriggered: scene.project.initializeMotion(scene.currentModel, VPVM.Project.ModelMotion)
    }
    FileDialog {
        id: loadProjectDialog
        nameFilters: [ qsTr("Project File (*.xml)") ]
        selectExisting: true
        onAccepted: {
            var fileUrlString = fileUrl.toString(),
                    indexOf = fileUrlString.lastIndexOf("/"),
                    name = indexOf >= 0 ? fileUrlString.substring(indexOf + 1) : fileUrlString
            progressWindow.text = qsTr("Loading Project %1").arg(name)
            progressWindow.show()
            scene.project.loadAsync(fileUrl)
            saveProjectDialog.savedPath = fileUrl
            progressWindow.hide()
        }
    }
    MessageDialog {
        id: confirmSavingProjectBeforeLoadingProjectDialog
        title: qsTr("The project has been modified.")
        text: qsTr("Do you want to save your changes of %1?").arg(scene.project.title)
        icon: StandardIcon.Question
        standardButtons: StandardButton.Save | StandardButton.Discard | StandardButton.Cancel
        onAccepted: {
            saveProjectAction.trigger()
            loadProjectDialog.open()
        }
        onDiscard: loadProjectDialog.open()
    }
    Action {
        id: loadProjectAction
        text: qsTr("Load Project")
        tooltip: qsTr("Load a project from file. If the project exists, it will be deleted and undone.")
        shortcut: StandardKey.Open
        onTriggered: {
            if (scene.project.dirty) {
                confirmSavingProjectBeforeLoadingProjectDialog.open()
            }
            else {
                loadProjectDialog.open()
            }
        }
    }
    SaveDialog {
        id: saveProjectDialog
        nameFilters: loadProjectDialog.nameFilters
        title: qsTr("Save Project")
        suffix: "xml"
    }
    Action {
        id: saveProjectAction
        text: qsTr("Save Project")
        tooltip: qsTr("Save the current project to the file.")
        shortcut: StandardKey.Save
        onTriggered: saveProjectAsAction.save(saveProjectDialog.getPath())
    }
    Action {
        id: saveProjectAsAction
        text: qsTr("Save Project As")
        tooltip: qsTr("Save the current project to the specified file.")
        shortcut: StandardKey.SaveAs
        function save(fileUrl) {
            var fileUrlString = fileUrl.toString(),
                    indexOf = fileUrlString.lastIndexOf("/"),
                    name = indexOf >= 0 ? fileUrlString.substring(indexOf + 1) : fileUrlString
            if (fileUrlString !== "") {
                progressWindow.text = qsTr("Saving Project %1").arg(name)
                progressWindow.show()
                scene.project.save(fileUrl)
                progressWindow.hide()
            }
        }
        onTriggered: save(saveProjectDialog.getPathAs())
    }
    FileDialog {
        id: addModelDialog
        nameFilters: [
            qsTr("Model File (*.pmd *.pmx *.x)"),
            qsTr("MikuMikuDance Model File (*.pmd *.pmx)"),
            qsTr("DirectX Model File (*.x)")
        ]
        selectExisting: true
        onAccepted: scene.project.loadModel(addModelDialog.fileUrl)
        onRejected: motionCreateablesList.currentIndex = 0
    }
    Action {
        id: addModelAction
        text: qsTr("Add Model/Asset")
        tooltip: qsTr("Load a model from file. The loaded model will make current.")
        shortcut: StandardKey.AddTab
        onTriggered: addModelDialog.open()
    }
    FileDialog {
        id: addEffectDialog
        nameFilters: [
            qsTr("Effect File (*.glslfx)")
        ]
        selectExisting: true
        onAccepted: scene.project.loadEffect(addEffectDialog.fileUrl)
        onRejected: motionCreateablesList.currentIndex = 0
    }
    Action {
        id: addEffectAction
        text: qsTr("Add Effect")
        tooltip: qsTr("Load a effect from file.")
        onTriggered: addEffectDialog.open()
    }
    FileDialog {
        id: setModelMotionDialog
        nameFilters: [
            qsTr("Motion File (*.vmd *.mvd)"),
            qsTr("VOCALOID Motion Data (*.vmd)"),
            qsTr("Motion Vector Data (*.mvd)")
        ]
        selectExisting: true
        onAccepted: scene.project.loadMotion(fileUrl, scene.currentModel, VPVM.Project.ModelMotion)
    }
    Action {
        id: setModelMotionAction
        enabled: scene.currentModel
        text: qsTr("Set Motion for Current Selected Model")
        tooltip: qsTr("Load a model motion from file to the current model. If the model is bound to the exist motion, it will be deleted and undone.")
        onTriggered: setModelMotionDialog.open()
    }
    FileDialog {
        id: setCameraMotionDialog
        nameFilters: setModelMotionDialog.nameFilters
        selectExisting: true
        onAccepted: scene.project.loadMotion(fileUrl, null, VPVM.Project.CameraMotion)
    }
    Action {
        id: setCameraMotionAction
        text: qsTr("Set Motion for Camera")
        tooltip: qsTr("Load a camera motion from file. If the camera motion is bound, it will be deleted and undone.")
        onTriggered: setCameraMotionDialog.open()
    }
    FileDialog {
        id: setLightMotionDialog
        nameFilters: setModelMotionDialog.nameFilters
        selectExisting: true
        onAccepted: scene.project.loadMotion(fileUrl, null, VPVM.Project.LightMotion)
    }
    Action {
        id: setLightMotionAction
        text: qsTr("Set Motion for Light")
        tooltip: qsTr("Load a light motion from file. If the light motion is bound, it will be deleted and undone.")
        onTriggered: setLightMotionDialog.open()
    }
    FileDialog {
        id: loadPoseDialog
        nameFilters: [ qsTr("Model Pose File (*.vpd)") ]
        selectExisting: true
        onAccepted: scene.project.loadPose(fileUrl, scene.currentModel)
    }
    Action {
        id: loadPoseAction
        enabled: scene.currentModel
        text: qsTr("Load Pose")
        tooltip: qsTr("Load a pose from file. The loaded pose will be registered as keyframes to the current time index.")
        onTriggered: loadPoseDialog.open()
    }
    FileDialog {
        id: loadAudioDialog
        nameFilters: [ qsTr("AIFF/WAVE File (*.wav *.aif)") ]
        selectExisting: true
        onAccepted: scene.project.audioSource = fileUrl
    }
    Action {
        id: loadAudioAction
        text: qsTr("Load Audio")
        tooltip: qsTr("Load audio from a file.")
        onTriggered: loadAudioDialog.open()
    }
    FileDialog {
        id: loadVideoDialog
        selectExisting: true
        onAccepted: scene.project.videoSource = fileUrl
    }
    Action {
        id: loadVideoAction
        text: qsTr("Load Video as Background")
        tooltip: qsTr("Load video from a file as a background movie.")
        onTriggered: loadVideoDialog.open()
    }
    SaveDialog {
        id: saveMotionDialog
        nameFilters: setModelMotionDialog.nameFilters
        title: qsTr("Save Model Motion")
        suffix: "vmd"
    }
    Action {
        id: saveModelMotionAction
        enabled: scene.currentMotion
        text: qsTr("Save Model Motion")
        tooltip: qsTr("Save the current motion bound to the model to the file.")
        onTriggered: scene.currentMotion.save(saveMotionDialog.getPath())
    }
    Action {
        id: saveModelMotionAsAction
        enabled: scene.currentMotion
        text: qsTr("Save Model Motion As")
        tooltip: qsTr("Save the current motion bound to the model to the specified file.")
        onTriggered: scene.currentMotion.save(saveMotionDialog.getPathAs())
    }
    Action {
        id: addSelectedBoneKeyframeAction
        enabled: scene.currentMotion && scene.hasBoneSelected
        text: qsTr("Selected Bone")
        tooltip: qsTr("Register keyframe(s) of selected bone.")
        shortcut: "Ctrl+I,B"
        onTriggered: scene.currentMotion.addKeyframe(scene.currentModel.firstTargetBone, timeline.timeIndex)
    }
    Action {
        id: addSelectedMorphKeyframeAction
        enabled: scene.currentMotion && scene.hasMorphSelected
        text: qsTr("Selected Morph")
        tooltip: qsTr("Register keyframe(s) of selected morph.")
        shortcut: "Ctrl+I,M"
        onTriggered: scene.currentMotion.addKeyframe(scene.currentModel.firstTargetMorph, timeline.timeIndex)
    }
    Action {
        id: addAllKeyframesAction
        enabled: scene.currentMotion
        text: qsTr("All Keyframe(s) at Current Time")
        tooltip: qsTr("Register all selected keyframe(s) at current time index.")
        shortcut: "Ctrl+Alt+I"
    }
    Action {
        id: deleteKeyframesSelectedAction
        enabled: scene.currentMotion
        text: qsTr("Selected Keyframe(s)")
        tooltip: qsTr("Delete all selected keyframes.")
        shortcut: StandardKey.DeleteCompleteLine
        onTriggered: scene.currentMotion.removeAllSelectedKeyframes()
    }
    Action {
        id: deleteAllKeyframesAtCurrentTimeAction
        enabled: scene.currentMotion
        text: qsTr("All Keyframes at Current Time")
        tooltip: qsTr("Delete all selected keyframes at current time index.")
        shortcut: StandardKey.Delete
        onTriggered: scene.currentMotion.removeAllKeyframesAt(timeline.timeIndex)
    }
    Action {
        id: copyAction
        enabled: scene.currentMotion && timeline.hasSelectedKeyframes
        text: qsTr("&Copy")
        tooltip: qsTr("Copy current selected keyframes.")
        shortcut: StandardKey.Copy
        onTriggered: scene.currentMotion.copyKeyframes()
    }
    Action {
        id: pasteAction
        enabled: scene.currentMotion && scene.currentMotion.canPaste
        text: qsTr("&Paste")
        tooltip: qsTr("Paste copied keyframes.")
        shortcut: StandardKey.Paste
        onTriggered: scene.currentMotion.pasteKeyframes(timeline.timeIndex, false)
    }
    Action {
        id: cutAction
        enabled: scene.currentMotion && timeline.hasSelectedKeyframes && timeline.timeSeconds > 0
        text: qsTr("Cu&t")
        tooltip: qsTr("Cut current selected keyframes.")
        shortcut: StandardKey.Cut
        onTriggered: scene.currentMotion.cutKeyframes()
    }
    Action {
        id: pasteInversedAction
        enabled: scene.currentMotion && scene.currentMotion.canPaste
        text: qsTr("Paste Inversed")
        tooltip: qsTr("Paste copied keyframes. If the name of the bone starts with Left or Right, it will be mirrored.")
        onTriggered: scene.currentMotion.pasteKeyframes(timeline.timeIndex, true)
    }
    Action {
        id: undoAction
        enabled: scene.project.canUndo
        text: qsTr("Undo")
        tooltip: qsTr("Undo the previous action.")
        shortcut: StandardKey.Undo
        onTriggered: scene.project.undo()
    }
    Action {
        id: redoAction
        enabled: scene.project.canRedo
        text: qsTr("Redo")
        tooltip: qsTr("Redo the previous action.")
        shortcut: StandardKey.Redo
        onTriggered: scene.project.redo()
    }
    SaveDialog {
        id: exportImageDialog
        nameFilters: [
            qsTr("Image (*.png *.bmp *.jpg)")
        ]
        title: qsTr("Export Image")
        suffix: "png"
    }
    Action {
        id: exportImageAction
        text: qsTr("Export Image")
        tooltip: qsTr("Export current scene as an image.")
        shortcut: StandardKey.Print
        onTriggered: scene.exportImage(exportImageDialog.getPathAs(), exportTab.size)
    }
    SaveDialog {
        id: exportVideoDialog
        nameFilters: [
            qsTr("Video (*.avi *.mov *.mp4)")
        ]
        title: qsTr("Export Video")
        suffix: isOSX ? "mov" : "avi"
    }
    Action {
        id: exportVideoAction
        enabled: playSceneAction.enabled
        text: qsTr("Export Video")
        tooltip: qsTr("Export all entire scene as a video.")
        onTriggered: scene.exportVideo(exportVideoDialog.getPathAs(), exportTab.size, exportTab.videoType, exportTab.frameImageType)
    }
    Action {
        id: selectAllKeyframesAction
        enabled: scene.currentMotion
        text: qsTr("All Selected")
        tooltip: qsTr("Select all keyframes in the timeline.")
        shortcut: StandardKey.SelectAll
    }
    Action {
        id: selectCurrentTimeIndexKeyframesAction
        enabled: scene.currentMotion
        text: qsTr("All Selected at Current TimeIndex")
        tooltip: qsTr("Select all keyframes in the timeline at the current time index.")
        shortcut: StandardKey.SelectStartOfLine
        onTriggered: timeline.selectKeyframesAtCurrentTimeIndex()
    }
    Action {
        id: toggleLockBoneTrackAction
        enabled: scene.hasBoneSelected
        text: qsTr("Bone Track")
        tooltip: qsTr("Toggle locking the bone track. The locked bone track will not be able to move keyframe(s).")
        onTriggered: timeline.toggleLockMotionTrack(scene.currentModel.firstTargetBone)
    }
    Action {
        id: toggleLockMorphTrackAction
        enabled: scene.hasMorphSelected
        text: qsTr("Morph Track")
        tooltip: qsTr("Toggle locking the morph track. The locked morph track will not be able to move keyframe(s).")
        onTriggered: timeline.toggleLockMotionTrack(scene.currentModel.firstTargetMorph)
    }
    Action {
        id: toggleVisibleBoneTrackAction
        enabled: scene.hasBoneSelected
        text: qsTr("Bone Track")
        tooltip: qsTr("Toggle visible the bone track. The invisible bone track will not be selectable from scene.")
        onTriggered: timeline.toggleVisibleMotionTrack(scene.currentModel.firstTargetBone)
    }
    Action {
        id: toggleVisibleMorphTrackAction
        enabled: scene.hasMorphSelected
        text: qsTr("Morph Track")
        tooltip: qsTr("Toggle visible the morph track. The invisible morph track will not be selectable from scene.")
        onTriggered: timeline.toggleVisibleMotionTrack(scene.currentModel.firstTargetMorph)
    }
    Action {
        id: nextTimeIndexAction
        enabled: scene.currentMotion
        text: qsTr("Next Time Index")
        tooltip: qsTr("Seek current time index to the next time index.")
        shortcut: StandardKey.FindNext
        onTriggered: scene.seekNextTimeIndex(1)
    }
    Action {
        id: previousTimeIndexAtion
        enabled: scene.currentMotion
        text: qsTr("Previous Time Index")
        tooltip: qsTr("Seek current time index to the previous time index.")
        shortcut: StandardKey.FindPrevious
        onTriggered: scene.seekPreviousTimeIndex(1)
    }
    Action {
        id: showLabelsInEnglishAction
        text: qsTr("Show Labels in English")
        checkable: true
        checked: false
        onCheckedChanged: scene.project.language = checked ? VPVM.Project.English : VPVM.Project.Japanese
    }
    Action {
        id: playSceneAction
        enabled: scene.project.durationTimeIndex > 0
        text: qsTr("Play")
        tooltip: qsTr("Start playing scene.")
        onTriggered: scene.state = "play"
    }
    Action {
        id: pauseSceneAction
        enabled: playSceneAction.enabled && scene.canSetRange
        text: qsTr("Pause")
        tooltip: qsTr("Pause playing scene.")
        onTriggered: scene.state = "pause"
    }
    Action {
        id: stopSceneAction
        enabled: playSceneAction.enabled
        text: qsTr("Stop")
        tooltip: qsTr("Stop playing scene and seek timeline to zero.")
        onTriggered: {
            scene.state = "stop"
            scene.cancelExportingVideo()
        }
    }
    Action {
        id: playSceneLoopAction
        text: qsTr("Enable looped playing")
        tooltip: qsTr("Play scene with loop unless stopped.")
        checkable: true
        checked: scene.loop
        onToggled: scene.loop = checked
    }
    Action {
        id: applyPresetFrontAction
        text: qsTr("Front")
        tooltip: qsTr("Set (Rotate) camera to front of scene.")
        shortcut: "F1"
        onTriggered: cameraMenu.setPreset(VPVM.Camera.FrontPreset)
    }
    Action {
        id: applyPresetBackAction
        text: qsTr("Back")
        tooltip: qsTr("Set (Rotate) camera to back of scene.")
        shortcut: "F2"
        onTriggered: cameraMenu.setPreset(VPVM.Camera.BackPreset)
    }
    Action {
        id: applyPresetTopAction
        text: qsTr("Top")
        tooltip: qsTr("Set (Rotate) camera to top of scene.")
        shortcut: "F3"
        onTriggered: cameraMenu.setPreset(VPVM.Camera.TopPreset)
    }
    Action {
        id: applyPresetLeftAction
        text: qsTr("Left")
        tooltip: qsTr("Set (Rotate) camera to left of scene.")
        shortcut: "F4"
        onTriggered: cameraMenu.setPreset(VPVM.Camera.LeftPreset)
    }
    Action {
        id: applyPresetRightAction
        text: qsTr("Right")
        tooltip: qsTr("Set (Rotate) camera to right of scene.")
        shortcut: "F5"
        onTriggered: cameraMenu.setPreset(VPVM.Camera.RightPreset)
    }
    Action {
        id: resetCameraAction
        text: qsTr("Reset Camera")
        tooltip: qsTr("Reset camera to the initial default parameters.")
        onTriggered: scene.camera.reset()
    }
    Action {
        id: resetLightAction
        text: qsTr("Reset Light")
        tooltip: qsTr("Reset light to the initial default parameters.")
        onTriggered: scene.light.reset()
    }
    Action {
        id: openProjectPreferenceWindow
        text: qsTr("Project Preference")
        tooltip: qsTr("Open project preference dialog.")
        shortcut: "Ctrl+Shift+,"
        onTriggered: projectPreferenceWindowLoader.open({ "scene": scene })
    }
    Action {
        id: deleteModelAction
        text: qsTr("Delete Current Model")
        tooltip: qsTr("Delete current model. this will delete model and the bound motions, cannot be undone.")
        enabled: scene.currentModel
        onTriggered: scene.project.deleteModel(scene.currentModel)
    }
    ExclusiveGroup {
        id: editModeActionGroup
        Action {
            id: setSelectModeAction
            enabled: scene.currentModel
            text: qsTr("Select")
            tooltip: qsTr("Toggle the current edit mode to the mode of selecting bones.")
            checkable: true
            checked: scene.editMode === VPVM.RenderTarget.SelectMode
            onTriggered: scene.editMode = VPVM.RenderTarget.SelectMode
        }
        Action {
            id: setMoveModeAction
            enabled: scene.hasBoneSelected && scene.currentModel.firstTargetBone.movable
            text: qsTr("Move")
            tooltip: qsTr("Toggle the current edit mode to the mode of moving a bone.")
            checkable: enabled
            checked: scene.editMode === VPVM.RenderTarget.MoveMode
            onTriggered: scene.editMode = VPVM.RenderTarget.MoveMode
        }
        Action {
            id: setRotateModeAction
            enabled: scene.hasBoneSelected && scene.currentModel.firstTargetBone.rotateable
            text: qsTr("Rotate")
            tooltip: qsTr("Toggle the current edit mode to the mode of rotating a bone.")
            checkable: enabled
            checked: scene.editMode === VPVM.RenderTarget.RotateMode
            onTriggered: scene.editMode = VPVM.RenderTarget.RotateMode
        }
    }
    ExclusiveGroup {
        id: transformModeActionGroup
        function handleType(type) {
            setTransformModeGlobalAction.checked = type === VPVM.Model.GlobalTransform
            setTransformModeLocalAction.checked = type === VPVM.Model.LocalTransform
            setTransformModeViewAction.checked = type === VPVM.Model.ViewTransform
        }
        Action {
            id: setTransformModeGlobalAction
            text: qsTr("Global")
            tooltip: qsTr("Toggle the current transforming bone mode to Global (absolute coordinate).")
            checkable: true
            enabled: scene.currentModel
            onCheckedChanged: if (checked) scene.currentModel.transformType = VPVM.Model.GlobalTransform
        }
        Action {
            id: setTransformModeLocalAction
            text: qsTr("Local")
            tooltip: qsTr("Toggle the current transforming bone mode to Local (relative from bone coordinate).")
            checkable: true
            checked: true
            enabled: scene.currentModel
            onCheckedChanged: if (checked) scene.currentModel.transformType = VPVM.Model.LocalTransform
        }
        Action {
            id: setTransformModeViewAction
            text: qsTr("View")
            tooltip: qsTr("Toggle the current transforming bone mode to View (relative from camera lookat coordinate).")
            checkable: true
            enabled: scene.currentModel
            onCheckedChanged: if (checked) scene.currentModel.transformType = VPVM.Model.ViewTransform
        }
    }
    Action {
        id: resetBoneXAxisTranslationAction
        enabled: scene.hasBoneSelected
        text: qsTr("X Axis")
        tooltip: qsTr("Reset X axis of the bone.")
        onTriggered: scene.resetBone(scene.currentModel.firstTargetBone, VPVM.Project.TranslationAxisX)
    }
    Action {
        id: resetBoneYAxisTranslationAction
        enabled: scene.hasBoneSelected
        text: qsTr("Y Axis")
        tooltip: qsTr("Reset Y axis of the bone.")
        onTriggered: scene.resetBone(scene.currentModel.firstTargetBone, VPVM.Project.TranslationAxisY)
    }
    Action {
        id: resetBoneZAxisTranslationAction
        enabled: scene.hasBoneSelected
        text: qsTr("Z Axis")
        tooltip: qsTr("Reset Z axis of the bone.")
        onTriggered: scene.resetBone(scene.currentModel.firstTargetBone, VPVM.Project.TranslationAxisZ)
    }
    Action {
        id: resetBoneXYZAxesTranslationAction
        enabled: scene.hasBoneSelected
        text: qsTr("All X/Y/Z Axes")
        tooltip: qsTr("Reset All X/Y/Z axes of the bone.")
        onTriggered: scene.resetBone(scene.currentModel.firstTargetBone, VPVM.Project.TranslationAxisXYZ)
    }
    Action {
        id: resetBoneOrientationAction
        enabled: scene.hasBoneSelected
        text: qsTr("Reset Selected Bone Orientation")
        tooltip: qsTr("Reset orientation of the bone.")
        onTriggered: scene.resetBone(scene.currentModel.firstTargetBone, VPVM.Project.Orientation)
    }
    Action {
        id: resetAllBonesAction
        enabled: scene.currentModel
        text: qsTr("Reset All Bones")
        tooltip: qsTr("Reset all translation and rotations of all the bones.")
        onTriggered: scene.resetBone(null)
    }
    Action {
        id: resetSelectedMorphAction
        enabled: scene.hasMorphSelected
        text: qsTr("Reset Selected Morph")
        tooltip: qsTr("Reset selected morph weight to zero.")
        onTriggered: scene.resetMorph(scene.currentModel.firstTargetMorph)
    }
    Action {
        id: resetAllMorphsAction
        enabled: scene.currentModel
        text: qsTr("Reset All Morphs")
        tooltip: qsTr("Reset all morphs weight to zero.")
        onTriggered: scene.resetMorph(null)
    }
    Action {
        id: detachTimelineAction
        text: qsTr("Detach Timeline")
        tooltip: qsTr("Detach left timeline panel. If you want to attach again, you should click the close button.")
    }
    Action {
        id: toggleEnableFullSceneAction
        text: qsTr("Enable Full Scene Mode")
        tooltip: qsTr("Expands Scene and Hides Timeline and Tabs.")
        checkable: true
        checked: scene.isFullView
        onToggled: {
            scene.isFullView = checked
            propertyPanel.visible = !checked /* manually set visible of propertyPanel */
        }
    }
    Action {
        id: toggleDetachTimelineAction
        text: qsTr("Toggle Attach/Detach Timeline")
        tooltip: qsTr("Attach/Detach left timeline panel. If you want to attach again, you should click the close button.")
        checkable: true
        checked: scene.state === "detachedTimeline"
        onToggled: scene.state = checked ? "detachedTimeline" : "attachedTimeline"
    }
    Action {
        id: toggleVisiblePropertyPanelAction
        text: qsTr("Toggle Visible of Property Panel")
        tooltip: qsTr("Toggle visible of bottom property panel.")
        enabled: !scene.isFullView
        checkable: true
        checked: enabled && propertyPanel.visible
        onToggled: {
            propertyPanel.visible = checked
            timeline.refresh()
        }
    }
    Action {
        id: openGlobalPreferenceAction
        text: qsTr("Preference")
        tooltip: qsTr("Show global preference dialog.")
        shortcut: StandardKey.Preferences
        onTriggered: globalPreferenceWindowLoader.open({ "graphicsDevice": scene.graphicsDevice })
    }
    Action {
        id: openAboutAction
        text: qsTr("About %1").arg(Qt.application.name)
        tooltip: qsTr("Show information dialog of %1.").arg(Qt.application.name)
        onTriggered: aboutWindowLoader.open()
    }
    Action {
        id: openAboutQtAction
        text: qsTr("About Qt 5.2")
        tooltip: qsTr("Show information dialog of Qt.")
        onTriggered: VPVM.UIAuxHelper.openAboutQt()
    }
    Action {
        id: updateApplicationAction
        text: qsTr("Check for Updates...")
        tooltip: qsTr("Check updates of %1.").arg(Qt.application.name)
        onTriggered: VPVM.Updater.checkForUpdate()
    }
    Action {
        id: exitApplicationAction
        text: qsTr("&Exit")
        tooltip: qsTr("Exit this application.")
        shortcut: StandardKey.Quit
        onTriggered: {
            if (scene.project.dirty) {
                confirmSavingProjectBeforeClosingDialog.open()
            }
            else {
                exitApplication()
            }
        }
    }

    SystemPalette { id: systemPalette }
    color: systemPalette.window
    statusBar: StatusBar {
        visible: !scene.isFullView
        Label {
            id: statusBarLabel
            Layout.fillWidth: true
        }
    }
    menuBar: MenuBar {
        Menu {
            id: fileMenu
            title: isOSX ? qsTr("File") : qsTr("&File")
            MenuItem { action: newProjectAction }
            MenuItem { action: newMotionAction }
            MenuSeparator {}
            MenuItem { action: loadProjectAction }
            MenuItem { action: loadPoseAction }
            MenuItem { action: loadAudioAction }
            MenuItem { action: loadVideoAction }
            MenuSeparator {}
            MenuItem { action: addModelAction }
            MenuItem { action: addEffectAction; visible: applicationPreference.effectEnabled }
            MenuItem { action: setModelMotionAction }
            MenuItem { action: setCameraMotionAction }
            MenuSeparator {}
            MenuItem { action: saveProjectAction }
            MenuItem { action: saveProjectAsAction }
            MenuItem { action: saveModelMotionAction }
            MenuItem { action: saveModelMotionAsAction }
            MenuSeparator {}
            MenuItem { action: exportImageAction }
            MenuItem { action: exportVideoAction }
            MenuSeparator { visible: applicationShareableServiceNames.length > 0 }
            Menu {
                id: shareMenu
                title: qsTr("Share")
                visible: applicationShareableServiceNames.length > 0
                Instantiator {
                    model: applicationShareableServiceNames
                    MenuItem {
                        text: applicationShareableServiceNames[index]
                        onTriggered: scene.share(text)
                    }
                    onObjectAdded: shareMenu.insertItem(index, object)
                    onObjectRemoved: shareMenu.removeItem(object)
                }
            }
            MenuSeparator { visible: exitApplicationMenuItem.visible }
            MenuItem {
                id: exitApplicationMenuItem
                visible: !isOSX
                action: exitApplicationAction
            }
        }
        Menu {
            id: editMenu
            title: isOSX ? qsTr("Edit") : qsTr("&Edit")
            Menu {
                title: qsTr("Register keyframes of")
                MenuItem { action: addSelectedBoneKeyframeAction }
                MenuItem { action: addSelectedMorphKeyframeAction }
            }
            Menu {
                title: qsTr("Select keyframes of")
                MenuItem { action: selectCurrentTimeIndexKeyframesAction }
            }
            Menu {
                title: qsTr("Delete keyframes of")
                MenuItem { action: deleteKeyframesSelectedAction }
                MenuItem { action: deleteAllKeyframesAtCurrentTimeAction }
            }
            MenuSeparator {}
            Menu {
                title: qsTr("Toggle lock of")
                MenuItem { action: toggleLockBoneTrackAction }
                MenuItem { action: toggleLockMorphTrackAction }
            }
            Menu {
                title: qsTr("Toggle visibility of")
                MenuItem { action: toggleVisibleBoneTrackAction }
                MenuItem { action: toggleVisibleMorphTrackAction }
            }
            MenuSeparator {}
            MenuItem { action: copyAction }
            MenuItem { action: pasteAction }
            MenuItem { action: pasteInversedAction }
            MenuItem { action: cutAction }
            MenuSeparator {}
            MenuItem { id: undoMenuItem; action: undoAction; enabled: false }
            MenuItem { id: redoMenuItem; action: redoAction; enabled: false }
        }
        Menu {
            id: cameraMenu
            title: isOSX ? qsTr("Project") : qsTr("&Project")
            property bool isPreset: false
            function setPreset(type) {
                isPreset = true
                scene.camera.setPreset(type)
                isPreset = false
            }
            MenuItem { action: playSceneAction }
            MenuItem { action: pauseSceneAction }
            MenuItem { action: stopSceneAction }
            MenuItem { action: playSceneLoopAction }
            MenuSeparator {}
            Menu {
                title: qsTr("Camera Preset")
                MenuItem { action: applyPresetFrontAction }
                MenuItem { action: applyPresetBackAction }
                MenuItem { action: applyPresetTopAction }
                MenuItem { action: applyPresetLeftAction }
                MenuItem { action: applyPresetRightAction }
            }
            MenuSeparator {}
            MenuItem { action: resetCameraAction }
            MenuItem { action: resetLightAction }
            MenuSeparator {}
            MenuItem { action: nextTimeIndexAction }
            MenuItem { action: previousTimeIndexAtion }
            MenuSeparator {}
            MenuItem { action: showLabelsInEnglishAction }
            MenuSeparator {}
            MenuItem { action: openProjectPreferenceWindow }
        }
        Menu {
            id: modelMenu
            title: isOSX ? qsTr("Model") : qsTr("&Model")
            Menu {
                id: boneMenu
                title: qsTr("Bone")
                Menu {
                    title: qsTr("Edit Mode")
                    MenuItem { action: setSelectModeAction }
                    MenuItem { action: setMoveModeAction }
                    MenuItem { action: setRotateModeAction }
                }
                Menu {
                    title: qsTr("Transform Mode")
                    MenuItem { action: setTransformModeGlobalAction }
                    MenuItem { action: setTransformModeLocalAction }
                    MenuItem { action: setTransformModeViewAction }
                }
                MenuSeparator {}
                Menu {
                    title: qsTr("Reset Selected Bone Translation of")
                    MenuItem { action: resetBoneXAxisTranslationAction }
                    MenuItem { action: resetBoneYAxisTranslationAction }
                    MenuItem { action: resetBoneZAxisTranslationAction }
                    MenuItem { action: resetBoneXYZAxesTranslationAction }
                }
                MenuItem { action: resetBoneOrientationAction }
                MenuItem { action: resetAllBonesAction }
            }
            Menu {
                id: morphMenu
                title: qsTr("Morph")
                MenuItem { action: resetSelectedMorphAction }
                MenuItem { action: resetAllMorphsAction }
            }
            MenuSeparator {}
            MenuItem { action: deleteModelAction }
        }
        Menu {
            id: windowMenu
            title: isOSX ? qsTr("Window") : qsTr("&Window")
            MenuItem { action: toggleEnableFullSceneAction }
            MenuSeparator {}
            MenuItem { action: toggleDetachTimelineAction }
            MenuItem { action: toggleVisiblePropertyPanelAction }
            MenuSeparator {}
            MenuItem { action: openAboutQtAction }
        }
        Menu {
            id: helpMenu
            title: isOSX ? qsTr("Help") : qsTr("&Help")
            MenuItem { action: openGlobalPreferenceAction }
            MenuSeparator { visible: !isOSX }
            MenuItem { visible: VPVM.Updater.available; action: updateApplicationAction }
            MenuItem { action: openAboutAction }
        }
    }

    SplitView {
        id: applicationContainer
        anchors.fill: parent
        orientation: Qt.Vertical
        SplitView {
            orientation: Qt.Horizontal
            Layout.fillHeight: true
            Item {
                id: timelineContainer
                width: 400
                Layout.minimumWidth: 300
                visible: !scene.isFullView
                Rectangle {
                    id: timelineView
                    property bool initialized: false
                    anchors.fill: parent
                    color: systemPalette.window
                    ColumnLayout {
                        anchors.fill: parent
                        MotionCreateablesListModel {
                            id: motionCreateablesListModel
                        }
                        MotionCreateablesList {
                            id: motionCreateablesList
                            anchors.fill: parent
                        }
                        Timeline {
                            id: timeline
                            property var __lastCurrentMotion: null
                            property var __lastCurrentModel: null
                            property var __lastSelectedBone: null
                            property var __lastSelectedMorph: null
                            property int __lastTabIndex: 0
                            property real __lastDraggingKeyframeIndex: 0
                            function saveEditMotionState() {
                                __lastCurrentMotion = scene.currentMotion
                                __lastTabIndex = sceneTabView.currentIndex
                                var model = scene.currentModel
                                if (model) {
                                    __lastCurrentModel = model
                                    __lastSelectedBone = model.firstTargetBone
                                    __lastSelectedMorph = model.firstTargetMorph
                                    model.selectBone(null)
                                    model.firstTargetMorph = null
                                }
                                scene.currentMotion = scene.currentModel = null
                            }
                            function restoreEditMotionState() {
                                var motion = __lastCurrentMotion
                                if (motion) {
                                    scene.currentMotion = motion
                                    sceneTabView.currentIndex = __lastTabIndex
                                    var model = __lastCurrentModel
                                    if (model) {
                                        scene.currentModel = model
                                        scene.currentModel.firstTargetMorph = __lastSelectedMorph
                                        scene.currentModel.selectBone(__lastSelectedBone)
                                        timeline.assignModel(model)
                                    }
                                    return true
                                }
                                return false
                            }
                            function clearEditMotionState() {
                                __lastCurrentMotion = __lastCurrentModel = __lastSelectedBone = __lastSelectedMorph = null
                                motionCreateablesListModel.updateModels()
                            }
                            enabled: scene.currentMotion
                            enableInputEvent: scene.isHUDAvailable
                            anchors.fill: parent
                            backgroundFillColor: systemPalette.window
                            selectedLabelFillColor: systemPalette.highlight
                            onTimeSecondsChanged: if (!scene.playing) scene.seek(timeline.timeIndex)
                            onTimelineWillHide: {
                                saveEditMotionState()
                                timelineView.state = "selectMotionCreateables"
                                motionCreateablesList.forceActiveFocus()
                            }
                            onTimelineWillPlay: playSceneAction.trigger()
                            onTimelineWillPause: pauseSceneAction.trigger()
                            onTimelineWillStop: stopSceneAction.trigger()
                            onDraggingKeyframesDidBegin: __lastDraggingKeyframeIndex = timeIndex
                            onDraggingKeyframesDidCommit: {
                                var oldTimeIndex = __lastDraggingKeyframeIndex
                                for (var i in keyframes) {
                                    var keyframe = keyframes[i], keyframeTimeIndex = keyframe.timeIndex
                                    if (keyframeTimeIndex === 0 || (timeIndex < oldTimeIndex && keyframeTimeIndex < timeIndex)) {
                                        notificationArea.notify(qsTr("Cannot merge selected keyframes out of destination time index"))
                                        return
                                    }
                                }
                                scene.currentMotion.mergeKeyframes(keyframes, timeIndex, oldTimeIndex)
                                __lastDraggingKeyframeIndex = 0
                            }
                            /* Timeline -> Motion */
                            onKeyframesDidSelect: scene.currentMotion.selectedKeyframes = keyframes
                            onKeyframeWillAdd: {
                                /* called by Timeline#markKeyframeAdded */
                                var motion = scene.currentMotion
                                if (motion) {
                                    motion.addKeyframe(opaque, timeIndex)
                                }
                            }
                            onOpaqueObjectDidSelect: {
                                var model = scene.currentModel
                                if (model) {
                                    model.selectOpaqueObject(opaque)
                                }
                            }
                            DropArea {
                                anchors.fill: parent
                                onDropped: {
                                    if (drop.hasUrls) {
                                        var url = drop.urls[0]
                                        scene.project.loadMotion(url, scene.currentModel, VPVM.Project.ModelMotion)
                                    }
                                }
                            }
                        }
                        Rectangle {
                            id: coverTimeline
                            anchors.fill: parent
                            opacity: 0.5
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    timelineView.state = "selectMotionCreateables"
                                    motionCreateablesList.forceActiveFocus()
                                }
                            }
                        }
                        Text {
                            anchors.centerIn: parent
                            font.family: applicationPreference.fontFamily
                            visible: coverTimeline.visible
                            text: qsTr("No motion is selected to edit.\nClick here to select the motion to edit.\n(Adding a model is also here)")
                        }
                    }
                    state: "initialState"
                    states: [
                        State {
                            name: "initialState"
                            PropertyChanges { target: motionCreateablesList; visible: false }
                            PropertyChanges { target: timeline; visible: true; enabled: false }
                            PropertyChanges { target: coverTimeline; visible: true }
                        },
                        State {
                            name: "selectMotionCreateables"
                            PropertyChanges { target: motionCreateablesList; visible: true }
                            PropertyChanges { target: timeline; visible: false }
                            PropertyChanges { target: coverTimeline; visible: false }
                        },
                        State {
                            name: "editMotion"
                            PropertyChanges { target: motionCreateablesList; visible: false }
                            PropertyChanges { target: timeline; visible: true; enabled: true }
                            PropertyChanges { target: coverTimeline; visible: false }
                        }
                    ]
                }
            }
            Scene {
                id: scene
                /* Motion -> Timeline */
                function __handleMotionKeyframeDidAdd(keyframe) {
                    timeline.addKeyframe(keyframe)
                }
                function __handleMotionKeyframeDidRemove(keyframe) {
                    timeline.removeKeyframe(keyframe)
                }
                function __handleMotionKeyframeDidReplace(dst, src) {
                    timeline.replaceKeyframe(dst, src)
                }
                function __handleMotionTimeIndexDidChange(keyframe, newTimeIndex, oldTimeIndex) {
                    timeline.timeIndex = newTimeIndex
                }
                function __handleModelChildMotionChanged() {
                    /* reload child motion of current model to refresh timeline using updated child motion */
                    var model = scene.currentModel
                    if (model) {
                        var motion = model.childMotion
                        if (motion) {
                            timeline.assignModel(model)
                        }
                    }
                }
                function __handleModelOrderIndexChanged() {
                    motionCreateablesListModel.updateModels()
                }
                function __registerMotionCallbacks(motion) {
                    motion.keyframeDidAdd.connect(__handleMotionKeyframeDidAdd)
                    motion.keyframeDidRemove.connect(__handleMotionKeyframeDidRemove)
                    motion.keyframeDidReplace.connect(__handleMotionKeyframeDidReplace)
                    motion.timeIndexDidChange.connect(__handleMotionTimeIndexDidChange)
                }
                Layout.fillWidth: true
                offsetX: 0
                offsetY: statusBar.height * statusBar.visible + propertyPanel.height * propertyPanel.visible
                project.onLanguageChanged: timeline.refresh()
                project.onProjectDidCreate: motionCreateablesListModel.updateModels()
                project.onProjectDidLoad: {
                    var model = scene.currentModel
                    if (model) {
                        timeline.assignModel(model)
                        timelineView.state = "editMotion"
                    }
                    else {
                        timelineView.state = "initialState"
                    }
                    notificationArea.notify(qsTr("The project %1 is loaded.").arg(project.title))
                    motionCreateablesListModel.updateModels()
                }
                project.onProjectDidSave: {
                    notificationArea.notify(qsTr("The project %1 is saved.").arg(project.title))
                }
                project.onModelDidRemove: {
                    timeline.clearEditMotionState()
                    timelineView.state = "selectMotionCreateables"
                }
                project.onMotionDidLoad: __registerMotionCallbacks(motion)
                camera.onMotionChanged: {
                    var motion = camera.motion
                    __registerMotionCallbacks(motion)
                    motionCreateablesListModel.get(1).motion = motion
                }
                light.onMotionChanged: {
                    var motion = light.motion
                    __registerMotionCallbacks(motion)
                    motionCreateablesListModel.get(2).motion = motion
                }
                onNotificationDidPost: notificationArea.notify(message)
                onBoneTransformTypeDidChange: transformModeActionGroup.handleType(type)
                onBoneDidSelect: timeline.markTrackSelected(bone)
                onMorphDidSelect: timeline.markTrackSelected(morph)
                onUploadingModelDidSucceed: {
                    model.childMotionChanged.connect(__handleModelChildMotionChanged)
                    model.orderIndexChanged.connect(__handleModelOrderIndexChanged)
                    if (!isProject) {
                        sceneTabView.currentIndex = sceneTabView.modelTabIndex
                        timeline.assignModel(model)
                        timelineView.state = "editMotion"
                        motionCreateablesListModel.updateModels()
                        notificationArea.notify(qsTr("The model %1 is loaded.").arg(model.name))
                    }
                }
                onUploadingModelDidFail: {
                    if (!isProject) {
                        notificationArea.notify(qsTr("The model %1 cannot be loaded").arg(model.name))
                    }
                }
                onUploadingEffectDidSucceed: notificationArea.notify(qsTr("The effect %1 is loaded.").arg(model.name))
                onUploadingEffectDidFail: notificationArea.notify(qsTr("The effect %1 cannot be loaded.").arg(model.name))
                onPlayingChanged: playing ? timeline.saveEditMotionState() : timeline.restoreEditMotionState()
                onEncodeDidFinish: notificationArea.notify(isNormalExit ? qsTr("Encoding process is finished normally.") : qsTr("Encoding process is failed."))
                onEncodeDidCancel: notificationArea.notify(qsTr("Encode process is cancelled."))
                onToggleTimelineVisible: {
                    if (!state || state === "attachedTimeline") {
                        timelineContainer.visible = timelineContainer.visible ? false : true
                    }
                }
                onToggleTimelineWindowed: state = "detachedTimeline"
                states: [
                    State {
                        name: "attachedTimeline"
                        StateChangeScript { script: timelineWindow.close() }
                        ParentChange { target: timelineView; parent: timelineContainer }
                        PropertyChanges { target: timelineContainer; visible: !scene.isFullView }
                        StateChangeScript { script: timeline.refresh() }
                    },
                    State {
                        name: "detachedTimeline"
                        StateChangeScript { script: timelineWindow.show() }
                        ParentChange { target: timelineView; parent: windowedTimeline }
                        PropertyChanges {
                            target: timelineContainer
                            width: 400
                            height: applicationWindow.height
                            visible: false
                        }
                        StateChangeScript { script: timeline.refresh() }
                    }
                ]
                NotificationArea {
                    id: notificationArea
                    anchors.top: parent.top
                    width: parent.width
                }
            }
            Binding {
                target: timeline
                property: "timeIndex"
                value: scene.project.currentTimeIndex
            }
        }
        Rectangle {
            id: propertyPanel
            readonly property real maximumPositionValue: Math.pow(2, 31)
            readonly property real minimumPositionValue: -Math.pow(2, 31)
            readonly property real maximumRotaitonValue: 360
            readonly property real minimumRotaitonValue: -360
            property real positionStepSize: 0.05
            property real rotationStepSize: 0.1
            property int positionDecimalPrecision: 3
            property int rotationDecimalPrecision: 3
            Layout.minimumHeight: 220
            Layout.maximumHeight: 400
            anchors.margins: applicationLayoutAnchorMargin
            height: 220
            color: systemPalette.window
            enabled: scene.isHUDAvailable
            visible: !scene.isFullView
            TabView {
                id: sceneTabView
                readonly property int modelTabIndex : 0
                readonly property int cameraTabIndex : 1
                readonly property int lightTabIndex : 2
                readonly property int timelineTabIndex : 3
                anchors.fill: parent
                anchors.margins: applicationLayoutAnchorMargin
                ModelTab { id: modelTab }
                CameraTab { id: cameraTab }
                LightTab { id: lightTab }
                TimelineTab { id: timelineTab }
                ExportTab { id: exportTab }
            }
        }
    }
}
