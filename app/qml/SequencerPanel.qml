import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Step sequencer. Ingeklapt: afspelen, tempo, gate, swing en een rij van 16 stappen (tik = aan/uit).
// Uitgeklapt: een rooster met een octaaf noten per stap (tik op een vakje = die noot op die stap) en
// een rij accenten, zoals op een groovebox.
Item {
    id: panel

    property var engine
    property bool compact: false
    property bool expanded: false
    property int gridBase: 36   // laagste noot in het rooster (C2)

    readonly property var names: ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    function noteName(note) { return names[note % 12] + (Math.floor(note / 12) - 1) }
    function stepAt(i) { return engine.steps[i] }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            spacing: panel.compact ? 4 : 10
            Layout.fillWidth: true
            Layout.fillHeight: !panel.expanded

            Button {
                text: panel.engine.stepSequencerRunning ? qsTr("Stop") : qsTr("Play")
                highlighted: panel.engine.stepSequencerRunning
                focusPolicy: Qt.NoFocus
                Layout.preferredWidth: panel.compact ? 64 : 80
                onClicked: panel.engine.toggleStepSequencer()
            }
            KnobStrip {
                engine: panel.engine
                compact: panel.compact
                model: [
                    { param: "seq_bpm", label: qsTr("Tempo"), from: 40, to: 240, unit: " bpm", dec: 0 },
                    { param: "seq_gate", label: qsTr("Gate"), from: 0.1, to: 1 },
                    { param: "seq_swing", label: qsTr("Swing"), from: 0, to: 0.6 }
                ]
            }

            // Ingeklapt: 16 stappen op een rij.
            Row {
                visible: !panel.expanded
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 3
                Repeater {
                    model: 16
                    Rectangle {
                        required property int index
                        readonly property var step: panel.engine.steps[index]
                        width: (parent.width - 15 * 3) / 16
                        height: parent.height
                        radius: 4
                        color: step.active ? (step.accent ? "#3fd1bf" : "#2a9d8f") : "#24262e"
                        border.width: panel.engine.currentStep === index ? 2 : (index % 4 === 0 ? 1 : 0)
                        border.color: panel.engine.currentStep === index ? "#ffffff" : "#4a4d58"
                        Label {
                            anchors.centerIn: parent
                            visible: parent.step.active && parent.width > 22
                            text: panel.noteName(parent.step.note)
                            font.pixelSize: panel.compact ? 9 : 11
                            rotation: parent.width < 34 ? -90 : 0
                            color: "#0e1013"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: panel.engine.setStep(index, !parent.step.active, parent.step.note, parent.step.accent)
                        }
                    }
                }
            }

            // Uitgeklapt: octaaf van het rooster en wissen / standaardpatroon.
            Item { visible: panel.expanded; Layout.fillWidth: true }
            Label { visible: panel.expanded; text: qsTr("Octave") ; opacity: 0.75 }
            ToolButton { visible: panel.expanded; text: "−"; enabled: panel.gridBase > 12; onClicked: panel.gridBase -= 12 }
            Label { visible: panel.expanded; text: panel.noteName(panel.gridBase) }
            ToolButton { visible: panel.expanded; text: "+"; enabled: panel.gridBase < 96; onClicked: panel.gridBase += 12 }
            Button { visible: panel.expanded; text: qsTr("Clear"); flat: true; focusPolicy: Qt.NoFocus; onClicked: panel.engine.clearSteps() }
            Button { visible: panel.expanded; text: qsTr("Reset"); flat: true; focusPolicy: Qt.NoFocus; onClicked: panel.engine.resetSteps() }
        }

        // Rooster: 12 rijen (hoog boven), 16 kolommen, onderaan de accenten.
        Item {
            visible: panel.expanded
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property real labelWidth: 30
            readonly property real cellW: (width - labelWidth) / 16
            readonly property real cellH: height / 13

            Repeater {
                model: 12
                Label {
                    required property int index
                    readonly property int note: panel.gridBase + 11 - index
                    x: 0
                    y: index * parent.cellH
                    height: parent.cellH
                    width: parent.labelWidth - 4
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: panel.noteName(note)
                    font.pixelSize: Math.min(11, parent.cellH * 0.7)
                    opacity: note % 12 === 0 ? 1 : 0.6
                }
            }
            Label {
                x: 0
                y: 12 * parent.cellH
                height: parent.cellH
                width: parent.labelWidth - 4
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                text: qsTr("Acc")
                font.pixelSize: Math.min(11, parent.cellH * 0.7)
            }
            Repeater {
                model: 16 * 13
                Rectangle {
                    required property int index
                    readonly property int column: index % 16
                    readonly property int row: Math.floor(index / 16)
                    readonly property bool accentRow: row === 12
                    readonly property int note: panel.gridBase + 11 - row
                    readonly property var step: panel.engine.steps[column]
                    readonly property bool on: accentRow ? step.accent : (step.active && step.note === note)
                    readonly property bool black: [1, 3, 6, 8, 10].indexOf(note % 12) >= 0
                    x: parent.labelWidth + column * parent.cellW + 1
                    y: row * parent.cellH + 1
                    width: parent.cellW - 2
                    height: parent.cellH - 2
                    radius: 3
                    color: on ? (accentRow ? "#f0b35a" : "#3fd1bf")
                              : (accentRow ? "#2b2620" : (black ? "#1a1b21" : "#23252d"))
                    border.width: panel.engine.currentStep === column ? 1 : 0
                    border.color: "#ffffff"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            const s = parent.step
                            if (parent.accentRow)
                                panel.engine.setStep(parent.column, s.active, s.note, !s.accent)
                            else if (parent.on)
                                panel.engine.setStep(parent.column, false, s.note, s.accent)
                            else
                                panel.engine.setStep(parent.column, true, parent.note, s.accent)
                        }
                    }
                }
            }
        }
    }
}
