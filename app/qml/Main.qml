import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import MidiPiano

// Vereiste 56: 56.meaningful usage of a GUI (not just a screen that shows some text and a textfield)
// De interface is een speelbaar instrument: een multitouch-pianotoetsenbord (akkoorden, glissando,
// aanslagsterkte), een keuze uit acht gesamplede instrumenten (vleugel, elektrische piano, orgel,
// ...) of de eigen wavetable-synth (tien golfvormen, morphen met Position) met draaiknoppen voor
// ADSR, een resonante filter met drive en filter-envelope, en volume. Verder een instelbare galm, een
// sustainpedaal, een live OpenGL-oscilloscoop, presets laden en opslaan, een demo-sequencer waarvan
// de noten oplichten op het toetsenbord, en octaaf-knoppen. Een instellingenpaneel zoekt en verbindt
// Bluetooth-MIDI-toestellen (de PSoC 6); hun noten lichten ook op. Op een gsm staat de app liggend,
// met een compacte bovenbalk zodat het toetsenbord zo groot mogelijk is.
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

    // App naar de achtergrond: ingedrukte toetsen loslaten (de vinger komt nooit meer "los").
    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive)
                keyboard.releaseAll()
        }
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
            ToolButton {
                // Instellingen: Bluetooth MIDI en audio-info. Het blauwe bolletje = Bluetooth verbonden.
                text: "\u2699"
                font.pixelSize: 22
                focusPolicy: Qt.NoFocus
                onClicked: settingsDrawer.open()
                Rectangle {
                    visible: synth.bluetooth.connected
                    width: 8; height: 8; radius: 4
                    color: "#4aa3ff"
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                }
            }
            Button {
                text: synth.sequencerRunning ? qsTr("Stop demo") : qsTr("Play demo")
                highlighted: synth.sequencerRunning
                focusPolicy: Qt.NoFocus
                onClicked: synth.toggleSequencer()
            }
        }
    }

    // Instellingen: schuift van rechts in beeld.
    Drawer {
        id: settingsDrawer
        edge: Qt.RightEdge
        width: Math.min(440, window.width * 0.6)
        height: window.height
        onClosed: keyboard.forceActiveFocus()

        Flickable {
            anchors.fill: parent
            contentHeight: settingsColumn.implicitHeight + 32
            clip: true

            ColumnLayout {
                id: settingsColumn
                x: 16
                y: 16
                width: parent.width - 32
                spacing: 10

                Label { text: qsTr("Settings"); font.bold: true; font.pixelSize: 20 }

                Label { text: qsTr("Bluetooth MIDI"); font.bold: true; font.pixelSize: 15; Layout.topMargin: 6 }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    opacity: 0.75
                    text: qsTr("Play the synth from a Bluetooth MIDI device, such as the PSoC 6 sequencer or a BLE keyboard.")
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: synth.bluetooth.statusText
                    color: synth.bluetooth.connected ? "#4aa3ff" : "#f0b35a"
                }
                Label {
                    visible: synth.bluetooth.connected
                    text: qsTr("MIDI messages received: %1").arg(synth.bluetooth.messageCount)
                    opacity: 0.75
                }
                RowLayout {
                    spacing: 8
                    visible: synth.bluetooth.available
                    Button {
                        text: synth.bluetooth.scanning ? qsTr("Stop search") : qsTr("Search devices")
                        highlighted: !synth.bluetooth.connected
                        focusPolicy: Qt.NoFocus
                        onClicked: synth.bluetooth.scanning ? synth.bluetooth.stopScan() : synth.bluetooth.startScan()
                    }
                    Button {
                        visible: synth.bluetooth.connected
                        text: qsTr("Disconnect")
                        focusPolicy: Qt.NoFocus
                        onClicked: synth.bluetooth.disconnectDevice()
                    }
                    BusyIndicator {
                        running: synth.bluetooth.scanning
                        visible: running
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                    }
                }
                Repeater {
                    model: synth.bluetooth.devices
                    ItemDelegate {
                        required property int index
                        required property string modelData
                        Layout.fillWidth: true
                        text: modelData
                        focusPolicy: Qt.NoFocus
                        onClicked: synth.bluetooth.connectTo(index)
                    }
                }

                Label { text: qsTr("Audio"); font.bold: true; font.pixelSize: 15; Layout.topMargin: 10 }
                Label { text: synth.audioInfo; opacity: 0.75 }

                Label { text: qsTr("Keyboard"); font.bold: true; font.pixelSize: 15; Layout.topMargin: 10 }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    opacity: 0.75
                    text: qsTr("On a computer: A W S E D F T G Y H U J K play the notes, hold Space for the sustain pedal.")
                }
            }
        }
    }

    // Galm-instellingen: werken voor elk instrument, ook de piano.
    Popup {
        id: reverbPanel
        anchors.centerIn: parent
        modal: true
        padding: 16
        onClosed: keyboard.forceActiveFocus()
        Column {
            spacing: 8
            Label { text: qsTr("Reverb"); font.bold: true; font.pixelSize: 16 }
            Row {
                spacing: 8
                Repeater {
                    model: [
                        { param: "reverb", label: qsTr("Mix"), from: 0, to: 1, log: false, unit: "", dec: 2 },
                        { param: "reverb_predelay", label: qsTr("Predelay"), from: 0, to: 0.25, log: false, unit: " s", dec: 2 },
                        { param: "reverb_decay", label: qsTr("Decay"), from: 0, to: 1, log: false, unit: "", dec: 2 },
                        { param: "reverb_damp", label: qsTr("Damping"), from: 0, to: 1, log: false, unit: "", dec: 2 },
                        { param: "reverb_lowcut", label: qsTr("Low cut (pre)"), from: 20, to: 1000, log: true, unit: " Hz", dec: 0 },
                        { param: "reverb_highcut", label: qsTr("High cut (post)"), from: 1000, to: 16000, log: true, unit: " Hz", dec: 0 }
                    ]
                    Knob {
                        required property var modelData
                        width: 100
                        dialSize: 58
                        synth: window.engine
                        paramName: modelData.param
                        label: modelData.label
                        from: modelData.from
                        to: modelData.to
                        logarithmic: modelData.log
                        unit: modelData.unit
                        decimals: modelData.dec
                    }
                }
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
                Label { text: qsTr("Wave"); opacity: 0.75; font.pixelSize: 11 }
                // Vier klassieke golfvormen en zes wavetables (Analog, Harmonics, PWM, Sync, Formant, Fold).
                ComboBox {
                    Layout.preferredWidth: window.compact ? 118 : 140
                    model: synth.waveformNames
                    currentIndex: synth.waveform
                    focusPolicy: Qt.NoFocus
                    onActivated: (index) => synth.waveform = index
                }
            }

            Row {
                spacing: window.compact ? 0 : 4
                Repeater {
                    // kind: "synth" = alleen voor de eigen synth, "wt" = alleen bij een wavetable,
                    // "all" = ook voor de gesamplede instrumenten.
                    model: [
                        { param: "attack", label: qsTr("Attack"), from: 0.001, to: 2, log: true, unit: " s", dec: 2, kind: "synth" },
                        { param: "decay", label: qsTr("Decay"), from: 0.01, to: 2, log: true, unit: " s", dec: 2, kind: "synth" },
                        { param: "sustain", label: qsTr("Sustain"), from: 0, to: 1, log: false, unit: "", dec: 2, kind: "synth" },
                        { param: "release", label: qsTr("Release"), from: 0.01, to: 3, log: true, unit: " s", dec: 2, kind: "synth" },
                        { param: "position", label: qsTr("Position"), from: 0, to: 1, log: false, unit: "", dec: 2, kind: "wt" },
                        { param: "cutoff", label: qsTr("Cutoff"), from: 100, to: 12000, log: true, unit: " Hz", dec: 0, kind: "synth" },
                        { param: "resonance", label: qsTr("Reso"), from: 0, to: 1, log: false, unit: "", dec: 2, kind: "synth" },
                        { param: "drive", label: qsTr("Drive"), from: 1, to: 10, log: true, unit: "x", dec: 1, kind: "synth" },
                        { param: "envmod", label: qsTr("Env→Filt"), from: 0, to: 1, log: false, unit: "", dec: 2, kind: "synth" },
                        { param: "volume", label: qsTr("Volume"), from: 0, to: 1, log: false, unit: "", dec: 2, kind: "all" }
                    ]
                    Knob {
                        required property var modelData
                        width: window.compact ? 52 : 64
                        dialSize: window.compact ? 38 : 56
                        compact: window.compact
                        synth: window.engine
                        paramName: modelData.param
                        label: modelData.label
                        from: modelData.from
                        to: modelData.to
                        logarithmic: modelData.log
                        unit: modelData.unit
                        decimals: modelData.dec
                        enabled: modelData.kind === "all"
                                 || (window.engine.synthControlsEnabled
                                     && (modelData.kind === "synth" || window.engine.waveformIsWavetable))
                        opacity: enabled ? 1 : 0.35
                    }
                }
            }

            Button {
                text: qsTr("Reverb")
                Layout.preferredWidth: window.compact ? 78 : 92
                highlighted: reverbPanel.opened
                focusPolicy: Qt.NoFocus
                onClicked: reverbPanel.open()
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
