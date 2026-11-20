import QtQuick
import QtQuick.Controls

// Een rij draaiknoppen uit een lijst ({param, label, from, to, log, unit, dec, kind}). kind bepaalt
// wanneer een knop actief is: "all" altijd, "synth" alleen voor de eigen synth, "wt" alleen bij een
// wavetable-golfvorm.
Row {
    id: strip

    property var engine
    property var model: []
    property bool compact: false
    property bool synthEnabled: true
    property bool wavetableEnabled: true
    property int knobWidth: compact ? 52 : 64
    property int dialSize: compact ? 38 : 56

    spacing: compact ? 0 : 4

    Repeater {
        model: strip.model
        Knob {
            required property var modelData
            readonly property string kind: modelData.kind === undefined ? "all" : modelData.kind
            width: strip.knobWidth
            dialSize: strip.dialSize
            compact: strip.compact
            synth: strip.engine
            paramName: modelData.param
            label: modelData.label
            from: modelData.from
            to: modelData.to
            logarithmic: modelData.log === true
            unit: modelData.unit === undefined ? "" : modelData.unit
            decimals: modelData.dec === undefined ? 2 : modelData.dec
            enabled: kind === "all" || (strip.synthEnabled && (kind === "synth" || strip.wavetableEnabled))
            opacity: enabled ? 1 : 0.35
            modulated: strip.engine.modulatedParams.indexOf(modelData.param) >= 0
        }
    }
}
