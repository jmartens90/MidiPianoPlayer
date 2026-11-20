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
    property int panel: 0            // 0 = Synth, 1 = Effects, 2 = Sequencer
    property bool expanded: false    // paneel groot (toetsenbord klein)
    property int patchRevision: 0    // telt bij elke patch-wijziging, voor bindings op synth.param()
    readonly property int panelHeight: expanded ? Math.round(height * 0.62) : (compact ? 74 : 110)

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
        function onPatchChanged() { window.patchRevision++ }
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
            Item { Layout.fillWidth: true }
            ToolButton {
                // Instellingen: Bluetooth MIDI en audio. Zelf getekend (drie schuifregelaars), want het
                // tandwiel-teken wordt op Android als kleurige emoji getoond. Blauw bolletje = Bluetooth verbonden.
                id: settingsButton
                focusPolicy: Qt.NoFocus
                Layout.preferredWidth: 44
                onClicked: settingsDrawer.open()
                Column {
                    anchors.centerIn: parent
                    spacing: 5
                    Repeater {
                        model: [0.3, 0.7, 0.45]
                        Item {
                            required property real modelData
                            width: 22; height: 2
                            Rectangle { anchors.fill: parent; radius: 1; color: "#d8dae3" }
                            Rectangle {
                                width: 7; height: 7; radius: 3.5
                                x: parent.modelData * (parent.width - width)
                                anchors.verticalCenter: parent.verticalCenter
                                color: "#20222b"
                                border.color: "#d8dae3"
                                border.width: 2
                            }
                        }
                    }
                }
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
                Label { text: qsTr("Voices playing: %1 of 8 (synth)").arg(synth.activeVoices); opacity: 0.75 }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("Buffer"); Layout.fillWidth: true }
                    ComboBox {
                        id: bufferBox
                        focusPolicy: Qt.NoFocus
                        readonly property var values: [0, 5, 10, 20]
                        model: [qsTr("Auto"), "5 ms", "10 ms", "20 ms"]
                        currentIndex: Math.max(0, values.indexOf(synth.bufferMs))
                        onActivated: (i) => synth.bufferMs = values[i]
                    }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    opacity: 0.6
                    font.pixelSize: 12
                    text: qsTr("Smaller is faster to respond, but crackles sooner on a slow phone.")
                }
                Switch {
                    Layout.fillWidth: true
                    focusPolicy: Qt.NoFocus
                    text: qsTr("Steady Bluetooth timing")
                    checked: synth.steadyTiming
                    onToggled: synth.steadyTiming = checked
                }

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

        // Bovenaan de panelen: tabbladen en een knop om het paneel groot te maken (zoals bij een
        // Arturia-synth). Uitgeklapt krijgt het paneel meer dan de helft van het scherm.
        RowLayout {
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: [qsTr("Synth"), qsTr("Effects"), qsTr("Sequencer")]
                Button {
                    required property int index
                    required property string modelData
                    text: modelData
                    flat: window.panel !== index
                    highlighted: window.panel === index
                    focusPolicy: Qt.NoFocus
                    implicitHeight: window.compact ? 30 : 36
                    topInset: 0
                    bottomInset: 0
                    font.pixelSize: window.compact ? 11 : 13
                    onClicked: window.panel = index
                }
            }
            Item { Layout.fillWidth: true }
            Label {
                visible: synth.stepSequencerRunning && window.panel !== 2
                text: qsTr("Sequencer playing")
                color: window.Material.accent
                font.pixelSize: 11
            }
            ToolButton {
                // Pijltje zelf getekend: het driehoek-teken ontbreekt in het lettertype van sommige gsm's.
                text: window.expanded ? qsTr("Less") : qsTr("More")
                leftPadding: 26
                focusPolicy: Qt.NoFocus
                Canvas {
                    width: 10; height: 6
                    x: 10
                    anchors.verticalCenter: parent.verticalCenter
                    rotation: window.expanded ? 180 : 0
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.reset()
                        ctx.strokeStyle = "#d8dae3"
                        ctx.lineWidth = 2
                        ctx.beginPath()
                        ctx.moveTo(1, 5); ctx.lineTo(5, 1); ctx.lineTo(9, 5)
                        ctx.stroke()
                    }
                }
                implicitHeight: window.compact ? 30 : 36
                font.pixelSize: window.compact ? 11 : 13
                onClicked: window.expanded = !window.expanded
            }
        }

        StackLayout {
            id: panels
            currentIndex: window.panel
            Layout.fillWidth: true
            Layout.fillHeight: false   // geneste layouts vullen standaard; het toetsenbord krijgt de rest
            Layout.preferredHeight: window.panelHeight
            Layout.maximumHeight: window.panelHeight

            // 0: Synth - golfvorm, knoppen en oscilloscoop.
            RowLayout {
                spacing: window.compact ? 8 : 16

                ColumnLayout {
                    spacing: 2
                    Layout.alignment: Qt.AlignTop
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

                KnobStrip {
                    Layout.alignment: Qt.AlignTop
                    engine: window.engine
                    compact: window.compact
                    synthEnabled: window.engine.synthControlsEnabled
                    wavetableEnabled: window.engine.waveformIsWavetable
                    model: [
                        { param: "attack", label: qsTr("Attack"), from: 0.001, to: 2, log: true, unit: " s", kind: "synth" },
                        { param: "decay", label: qsTr("Decay"), from: 0.01, to: 2, log: true, unit: " s", kind: "synth" },
                        { param: "sustain", label: qsTr("Sustain"), from: 0, to: 1, kind: "synth" },
                        { param: "release", label: qsTr("Release"), from: 0.01, to: 3, log: true, unit: " s", kind: "synth" },
                        { param: "position", label: qsTr("Position"), from: 0, to: 1, kind: "wt" },
                        { param: "cutoff", label: qsTr("Cutoff"), from: 100, to: 12000, log: true, unit: " Hz", dec: 0, kind: "synth" },
                        { param: "resonance", label: qsTr("Reso"), from: 0, to: 1, kind: "synth" },
                        { param: "drive", label: qsTr("Drive"), from: 1, to: 10, log: true, unit: "x", dec: 1, kind: "synth" },
                        { param: "envmod", label: qsTr("Env→Filt"), from: 0, to: 1, kind: "synth" },
                        { param: "volume", label: qsTr("Volume"), from: 0, to: 1 }
                    ]
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 100
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

            // 1: Effects - echo en galm, voor elk instrument.
            RowLayout {
                spacing: window.compact ? 6 : 14

                ColumnLayout {
                    spacing: 0
                    Layout.alignment: Qt.AlignTop
                    Label { text: qsTr("Delay"); font.bold: true; font.pixelSize: 12 }
                    Switch {
                        id: syncSwitch
                        text: qsTr("Sync")
                        focusPolicy: Qt.NoFocus
                        checked: window.patchRevision >= 0 && synth.param("delay_sync") >= 0.5
                        onToggled: synth.setParam("delay_sync", checked ? 1 : 0)
                        font.pixelSize: 11
                        implicitHeight: 32
                    }
                    // Gesynchroniseerd: kies een notenwaarde; de tijd volgt het sequencer-tempo.
                    ComboBox {
                        visible: syncSwitch.checked
                        Layout.preferredWidth: window.compact ? 86 : 100
                        implicitHeight: window.compact ? 34 : 40
                        focusPolicy: Qt.NoFocus
                        readonly property var beats: [0.25, 0.5, 0.75, 1, 1.5, 2]
                        model: ["1/16", "1/8", "1/8 .", "1/4", "1/4 .", "1/2"]
                        currentIndex: window.patchRevision >= 0 ? Math.max(0, beats.indexOf(synth.param("delay_beats"))) : 0
                        onActivated: (index) => synth.setParam("delay_beats", beats[index])
                    }
                }
                KnobStrip {
                    Layout.alignment: Qt.AlignTop
                    engine: window.engine
                    compact: window.compact
                    model: syncSwitch.checked
                           ? [
                               { param: "delay_mix", label: qsTr("Mix"), from: 0, to: 1 },
                               { param: "delay_feedback", label: qsTr("Feedback"), from: 0, to: 0.95 },
                               { param: "delay_tone", label: qsTr("Tone"), from: 500, to: 12000, log: true, unit: " Hz", dec: 0 }
                           ]
                           : [
                               { param: "delay_time", label: qsTr("Time"), from: 0.02, to: 2, log: true, unit: " s" },
                               { param: "delay_mix", label: qsTr("Mix"), from: 0, to: 1 },
                               { param: "delay_feedback", label: qsTr("Feedback"), from: 0, to: 0.95 },
                               { param: "delay_tone", label: qsTr("Tone"), from: 500, to: 12000, log: true, unit: " Hz", dec: 0 }
                           ]
                }
                Rectangle { width: 1; Layout.fillHeight: true; color: "#2b2d36" }
                Label { text: qsTr("Reverb"); font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignTop }
                KnobStrip {
                    Layout.alignment: Qt.AlignTop
                    engine: window.engine
                    compact: window.compact
                    model: [
                        { param: "reverb", label: qsTr("Mix"), from: 0, to: 1 },
                        { param: "reverb_predelay", label: qsTr("Predelay"), from: 0, to: 0.25, unit: " s" },
                        { param: "reverb_decay", label: qsTr("Decay"), from: 0, to: 1 },
                        { param: "reverb_damp", label: qsTr("Damping"), from: 0, to: 1 },
                        { param: "reverb_lowcut", label: qsTr("Low cut"), from: 20, to: 1000, log: true, unit: " Hz", dec: 0 },
                        { param: "reverb_highcut", label: qsTr("High cut"), from: 1000, to: 16000, log: true, unit: " Hz", dec: 0 }
                    ]
                }
                Item { Layout.fillWidth: true }
            }

            // 2: Sequencer.
            SequencerPanel {
                engine: window.engine
                compact: window.compact
                expanded: window.expanded
            }
        }

        PianoKeyboard {
            id: keyboard
            Layout.fillWidth: true
            Layout.fillHeight: true
            firstNote: 48 + window.octaveShift * 12
            whiteKeyCount: Math.max(8, Math.min(29, Math.floor(width / 44)))
            onNotePressed: (note, velocity) => synth.noteOn(note, velocity)

            // Meldingen (preset geladen, Bluetooth-fouten, ...) verschijnen even bovenaan het
            // toetsenbord en verdwijnen na 3 seconden, zodat ze de bovenbalk niet overlappen.
            Rectangle {
                id: toast
                z: 10
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 8
                width: toastLabel.implicitWidth + 28
                height: toastLabel.implicitHeight + 14
                radius: height / 2
                color: "#e62b2d36"
                opacity: 0
                Behavior on opacity { NumberAnimation { duration: 200 } }
                Label { id: toastLabel; anchors.centerIn: parent; color: "#f0b35a"; text: synth.statusText }
                Timer { id: toastTimer; interval: 3000; onTriggered: toast.opacity = 0 }
                Connections {
                    target: synth
                    function onStatusTextChanged() { toast.opacity = 1; toastTimer.restart() }
                }
            }
            onNoteReleased: (note) => synth.noteOff(note)
            onPedalChanged: (down) => synth.sustain = down
        }
    }
}
