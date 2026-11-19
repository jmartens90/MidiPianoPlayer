import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import MidiPiano

// Vereiste 56: 56.meaningful usage of a GUI (not just a screen that shows some text and a textfield)
// De interface is een speelbaar instrument: een multitouch-pianotoetsenbord (akkoorden, glissando,
// aanslagsterkte), een keuze uit acht gesamplede instrumenten (vleugel, elektrische piano, orgel,
// ...) of de eigen synth met vier golfvormen en draaiknoppen voor ADSR, cutoff, galm en volume, een sustainpedaal, een live
// OpenGL-oscilloscoop, presets laden en opslaan, een demo-sequencer waarvan de noten oplichten op het
// toetsenbord, en octaaf-knoppen. Op een gsm staat de app liggend, met een
// compacte bovenbalk zodat het toetsenbord zo groot mogelijk is.
ApplicationWindow {
    id: window

    property int octaveShift: 0
    readonly property SynthController engine: synth   // voor delegates met een eigen "synth"-property
    readonly property bool isAndroid: Qt.platform.os === "android"
    readonly property bool compact: height < 560

    width: 1180
    height: 700
    minimumWidth: 640
    minimumHeight: 360
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
            anchors.leftMargin: 10
            anchors.rightMargin: 6
            spacing: 6

            Label {
                text: "MidiPianoPlayer"
                font.bold: true
                font.pixelSize: 17
                visible: window.width > 1100
            }
            ComboBox {
                id: instrumentBox
                Layout.preferredWidth: window.compact ? 165 : 190
                model: synth.instrumentNames
                currentIndex: synth.instrument
                focusPolicy: Qt.NoFocus
                onActivated: (index) => synth.instrument = index
            }
            ComboBox {
                id: presetBox
                Layout.preferredWidth: window.compact ? 135 : 160
                visible: synth.synthControlsEnabled
                model: synth.presets
                displayText: synth.patchName
                focusPolicy: Qt.NoFocus
                onActivated: synth.loadPreset(currentText)
            }
            ToolButton {
                text: qsTr("Save")
                visible: synth.synthControlsEnabled
                onClicked: {
                    nameField.text = synth.patchName
                    saveDialog.open()
                }
            }
            ToolButton {
                text: "−"
                enabled: window.octaveShift > -3
                onClicked: window.octaveShift--
            }
            Label {
                text: "C" + (3 + window.octaveShift)
                horizontalAlignment: Text.AlignHCenter
                Layout.preferredWidth: 28
            }
            ToolButton {
                text: "+"
                enabled: window.octaveShift < 3
                onClicked: window.octaveShift++
            }
            Button {
                // Sustainpedaal: aan = losgelaten noten blijven klinken. Op de computer ook de spatiebalk.
                text: qsTr("Pedal")
                checkable: true
                checked: synth.sustain
                highlighted: checked
                focusPolicy: Qt.NoFocus
                onToggled: synth.sustain = checked
            }
            Label {
                Layout.fillWidth: true
                text: synth.statusText
                elide: Text.ElideRight
                color: "#f0b35a"
            }
            Label {
                text: synth.activeVoices
                opacity: 0.6
                Layout.preferredWidth: 22
                horizontalAlignment: Text.AlignRight
            }
            Button {
                text: synth.sequencerRunning ? qsTr("Stop demo") : qsTr("Play demo")
                highlighted: synth.sequencerRunning
                focusPolicy: Qt.NoFocus
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
        anchors.margins: window.compact ? 6 : 12
        spacing: window.compact ? 4 : 10

        // Klankregeling: golfvorm, knoppen en oscilloscoop op een rij.
        RowLayout {
            id: controls
            Layout.fillWidth: true
            Layout.fillHeight: false   // geneste layouts vullen standaard; het toetsenbord krijgt de rest
            Layout.preferredHeight: window.compact ? 74 : 110
            Layout.maximumHeight: window.compact ? 74 : 110
            spacing: window.compact ? 8 : 16

            ColumnLayout {
                spacing: 2
                enabled: synth.synthControlsEnabled
                opacity: enabled ? 1 : 0.35
                Label { text: qsTr("Waveform"); opacity: 0.75; font.pixelSize: 11 }
                ComboBox {
                    visible: window.compact
                    Layout.preferredWidth: 120
                    model: synth.waveformNames
                    currentIndex: synth.waveform
                    focusPolicy: Qt.NoFocus
                    onActivated: (index) => synth.waveform = index
                }
                GridLayout {
                    visible: !window.compact
                    columns: 2
                    rowSpacing: 0
                    columnSpacing: 4
                    ButtonGroup { id: waveGroup }
                    Repeater {
                        model: synth.waveformNames
                        Button {
                            required property int index
                            required property string modelData
                            text: modelData
                            Layout.preferredWidth: 88
                            checkable: true
                            checked: synth.waveform === index
                            ButtonGroup.group: waveGroup
                            focusPolicy: Qt.NoFocus
                            onClicked: synth.waveform = index
                        }
                    }
                }
            }

            Row {
                spacing: window.compact ? 2 : 8
                Repeater {
                    model: [
                        { param: "attack", label: qsTr("Attack"), from: 0.001, to: 2, log: true, unit: " s", dec: 2 },
                        { param: "decay", label: qsTr("Decay"), from: 0.01, to: 2, log: true, unit: " s", dec: 2 },
                        { param: "sustain", label: qsTr("Sustain"), from: 0, to: 1, log: false, unit: "", dec: 2 },
                        { param: "release", label: qsTr("Release"), from: 0.01, to: 3, log: true, unit: " s", dec: 2 },
                        { param: "cutoff", label: qsTr("Cutoff"), from: 100, to: 12000, log: true, unit: " Hz", dec: 0 },
                        { param: "reverb", label: qsTr("Reverb"), from: 0, to: 1, log: false, unit: "", dec: 2 },
                        { param: "volume", label: qsTr("Volume"), from: 0, to: 1, log: false, unit: "", dec: 2 }
                    ]
                    Knob {
                        required property var modelData
                        width: window.compact ? 56 : 66
                        dialSize: window.compact ? 40 : 58
                        compact: window.compact
                        synth: window.engine
                        paramName: modelData.param
                        label: modelData.label
                        from: modelData.from
                        to: modelData.to
                        logarithmic: modelData.log
                        unit: modelData.unit
                        decimals: modelData.dec
                        // Golfvorm, ADSR en cutoff gelden alleen voor de synth; galm en volume voor alles.
                        enabled: modelData.param === "volume" || modelData.param === "reverb"
                                 || window.engine.synthControlsEnabled
                        opacity: enabled ? 1 : 0.35
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 120
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

        PianoKeyboard {
            id: keyboard
            Layout.fillWidth: true
            Layout.fillHeight: true
            firstNote: 48 + window.octaveShift * 12
            whiteKeyCount: Math.max(8, Math.min(29, Math.floor(width / 44)))
            onNotePressed: (note, velocity) => synth.noteOn(note, velocity)
            onNoteReleased: (note) => synth.noteOff(note)
            onPedalChanged: (down) => synth.sustain = down
        }
    }
}
