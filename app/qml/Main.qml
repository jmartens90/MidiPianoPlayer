import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import MidiPiano

// Vereiste 56: 56.meaningful usage of a GUI (not just a screen that shows some text and a textfield)
// De interface is een speelbaar instrument: een multitouch-pianotoetsenbord (akkoorden, glissando,
// aanslagsterkte), draaiknoppen voor de ADSR-envelope, cutoff en volume, een keuze uit vier
// golfvormen, een live OpenGL-oscilloscoop, presets laden en opslaan, een demo-sequencer waarvan
// de noten oplichten op het toetsenbord, en octaaf-knoppen. De layout past zich aan van een breed
// PC-scherm naar een gsm in portret.
ApplicationWindow {
    id: window

    property int octaveShift: 0
    readonly property bool wide: width >= 1000

    width: 1180
    height: 700
    minimumWidth: 420
    minimumHeight: 420
    visible: true
    title: qsTr("MidiPianoPlayer")
    color: "#16171d"

    Material.theme: Material.Dark
    Material.accent: Material.Teal
    Material.primary: "#20222b"

    SynthController {
        id: synth
    }

    Connections {
        target: synth
        function onNoteStateChanged(note, on) { keyboard.setLit(note, on) }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 8
            spacing: 8

            Label {
                text: "MidiPianoPlayer"
                font.bold: true
                font.pixelSize: 18
                visible: window.width > 640
            }
            ComboBox {
                id: presetBox
                Layout.preferredWidth: 170
                model: synth.presets
                displayText: synth.patchName
                onActivated: synth.loadPreset(currentText)
            }
            ToolButton {
                text: qsTr("Save")
                onClicked: {
                    nameField.text = synth.patchName
                    saveDialog.open()
                }
            }
            Item { Layout.fillWidth: true }
            Label {
                text: synth.activeVoices + " / 8"
                opacity: 0.7
                ToolTip.visible: voicesArea.containsMouse
                ToolTip.text: qsTr("Voices playing")
                MouseArea { id: voicesArea; anchors.fill: parent; hoverEnabled: true }
            }
            Button {
                text: synth.sequencerRunning ? qsTr("Stop demo") : qsTr("Play demo")
                highlighted: synth.sequencerRunning
                onClicked: synth.toggleSequencer()
            }
        }
    }

    Dialog {
        id: saveDialog
        title: qsTr("Save preset")
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Save | Dialog.Cancel
        TextField {
            id: nameField
            width: 260
            placeholderText: qsTr("Preset name")
            onAccepted: saveDialog.accept()
        }
        onAccepted: synth.savePreset(nameField.text)
        onClosed: keyboard.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        GridLayout {
            Layout.fillWidth: true
            columns: window.wide ? 3 : 1
            columnSpacing: 16
            rowSpacing: 8

            ColumnLayout {
                spacing: 4
                Label { text: qsTr("Waveform"); opacity: 0.75; font.pixelSize: 12 }
                ButtonGroup { id: waveGroup }
                Flow {
                    Layout.preferredWidth: window.wide ? 180 : window.width - 24
                    spacing: 4
                    Repeater {
                        model: synth.waveformNames
                        Button {
                            required property int index
                            required property string modelData
                            text: modelData
                            checkable: true
                            checked: synth.waveform === index
                            ButtonGroup.group: waveGroup
                            focusPolicy: Qt.NoFocus
                            onClicked: synth.waveform = index
                        }
                    }
                }
            }

            Flow {
                Layout.fillWidth: !window.wide
                spacing: window.wide ? 10 : 6
                Knob { dialSize: window.wide ? 62 : 48; synth: synth; paramName: "attack"; label: qsTr("Attack"); from: 0.001; to: 2; logarithmic: true; unit: " s" }
                Knob { dialSize: window.wide ? 62 : 48; synth: synth; paramName: "decay"; label: qsTr("Decay"); from: 0.01; to: 2; logarithmic: true; unit: " s" }
                Knob { dialSize: window.wide ? 62 : 48; synth: synth; paramName: "sustain"; label: qsTr("Sustain"); from: 0; to: 1 }
                Knob { dialSize: window.wide ? 62 : 48; synth: synth; paramName: "release"; label: qsTr("Release"); from: 0.01; to: 3; logarithmic: true; unit: " s" }
                Knob { dialSize: window.wide ? 62 : 48; synth: synth; paramName: "cutoff"; label: qsTr("Cutoff"); from: 100; to: 12000; logarithmic: true; unit: " Hz"; decimals: 0 }
                Knob { dialSize: window.wide ? 62 : 48; synth: synth; paramName: "volume"; label: qsTr("Volume"); from: 0; to: 1 }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: window.wide ? 120 : 90
                radius: 8
                color: "#0f1014"
                border.color: "#2b2d36"
                clip: true
                WaveformView {
                    anchors.fill: parent
                    anchors.margins: 2
                    synth: synth
                    lineColor: window.Material.accent
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Button {
                text: "−"
                focusPolicy: Qt.NoFocus
                enabled: window.octaveShift > -3
                onClicked: window.octaveShift--
            }
            Label {
                text: qsTr("Octave C%1").arg(3 + window.octaveShift)
                Layout.preferredWidth: 80
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                text: "+"
                focusPolicy: Qt.NoFocus
                enabled: window.octaveShift < 3
                onClicked: window.octaveShift++
            }
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                text: synth.statusText
                elide: Text.ElideRight
                color: "#f0b35a"
            }
        }

        PianoKeyboard {
            id: keyboard
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 140
            firstNote: 48 + window.octaveShift * 12
            whiteKeyCount: Math.max(8, Math.min(29, Math.floor(width / 44)))
            onNotePressed: (note, velocity) => synth.noteOn(note, velocity)
            onNoteReleased: (note) => synth.noteOff(note)
        }
    }
}
