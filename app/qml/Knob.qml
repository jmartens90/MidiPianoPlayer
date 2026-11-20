import QtQuick
import QtQuick.Controls

// Een draaiknop voor een synth-parameter. Met logarithmic: true werkt de knop in octaven,
// zodat bv. de cutoff tussen 100 Hz en 1 kHz even veel draaibereik krijgt als tussen 1 en 10 kHz.
Item {
    id: root

    property var synth
    property string paramName
    property string label
    property real from: 0
    property real to: 1
    property bool logarithmic: false
    property string unit: ""
    property int decimals: 2
    property int dialSize: 58
    property bool compact: false
    property bool modulated: false   // een LFO of de modulatie-envelope draait aan deze knop

    implicitWidth: 66
    implicitHeight: column.implicitHeight

    function toParam(position) {
        return logarithmic ? from * Math.pow(to / from, position) : from + (to - from) * position
    }
    function toPosition(value) {
        return logarithmic ? Math.log(value / from) / Math.log(to / from) : (value - from) / (to - from)
    }
    function formatted(value) {
        if (unit === " Hz" && value >= 1000)
            return (value / 1000).toFixed(1) + " kHz"
        if (unit === " s" && value < 1)
            return Math.round(value * 1000) + " ms"
        return value.toFixed(decimals) + unit
    }
    function reload() {
        dial.value = toPosition(synth.param(paramName))
    }

    Column {
        id: column
        width: parent.width
        spacing: 0

        // De Material-stijl tekent een Dial nooit kleiner dan 64 px, dus kleinere knoppen schalen we.
        Item {
            width: root.dialSize
            height: root.dialSize
            anchors.horizontalCenter: parent.horizontalCenter

        Dial {
            id: dial
            width: 64
            height: 64
            anchors.centerIn: parent
            scale: root.dialSize / 64
            from: 0
            to: 1
            inputMode: Dial.Vertical
            focusPolicy: Qt.NoFocus
            onMoved: root.synth.setParam(root.paramName, root.toParam(value))
            Component.onCompleted: root.reload()
        }
        Rectangle {
            visible: root.modulated
            width: 8; height: 8; radius: 4
            color: "#ff9f43"
            anchors.right: parent.right
            anchors.top: parent.top
        }
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.label
            font.pixelSize: root.compact ? 10 : 12
            opacity: 0.75
            elide: Text.ElideRight
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.formatted(root.toParam(dial.value))
            font.pixelSize: root.compact ? 10 : 12
            elide: Text.ElideRight
        }
    }

    Connections {
        target: root.synth
        function onPatchChanged() { root.reload() }
    }
}
