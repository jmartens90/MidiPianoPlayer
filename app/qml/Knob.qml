import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Een draaiknop voor een synth-parameter. Met logarithmic: true werkt de knop in octaven,
// zodat bv. de cutoff tussen 100 Hz en 1 kHz even veel draaibereik krijgt als tussen 1 en 10 kHz.
ColumnLayout {
    id: root

    property var synth
    property string paramName
    property string label
    property real from: 0
    property real to: 1
    property bool logarithmic: false
    property string unit: ""
    property int decimals: 2
    property int dialSize: 62

    spacing: 0

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

    Dial {
        id: dial
        Layout.preferredWidth: root.dialSize
        Layout.preferredHeight: root.dialSize
        Layout.alignment: Qt.AlignHCenter
        from: 0
        to: 1
        inputMode: Dial.Vertical
        onMoved: root.synth.setParam(root.paramName, root.toParam(value))
        Component.onCompleted: root.reload()
    }
    Label {
        Layout.alignment: Qt.AlignHCenter
        text: root.label
        font.pixelSize: 12
        opacity: 0.75
    }
    Label {
        Layout.alignment: Qt.AlignHCenter
        text: root.formatted(root.toParam(dial.value))
        font.pixelSize: 12
    }

    Connections {
        target: root.synth
        function onPatchChanged() { root.reload() }
    }
}
