import QtQuick

// Multitouch-pianotoetsenbord. Elke vinger is een eigen touch point: akkoorden met meerdere vingers
// werken, en glijden over de toetsen speelt de toetsen waar je langs komt (glissando). Hoe lager je
// een toets raakt, hoe harder de noot (velocity), zoals bij een echte toets die je dieper indrukt.
// Op de PC kan je ook met het toetsenbord spelen: A W S E D F T G Y H U J K = C tot C.
Item {
    id: root

    property int firstNote: 48          // moet een C zijn
    property int whiteKeyCount: 15

    signal notePressed(int note, int velocity)
    signal noteReleased(int note)

    readonly property var whiteOffsets: [0, 2, 4, 5, 7, 9, 11]
    readonly property var hasBlackRight: [true, true, false, true, true, true, false]
    readonly property real whiteWidth: width / whiteKeyCount
    readonly property real blackWidth: whiteWidth * 0.6
    readonly property real blackHeight: height * 0.6

    // JavaScript-objecten melden geen wijzigingen; revision++ laat de kleuren opnieuw berekenen.
    property var downCount: ({})
    property var litNotes: ({})
    property var touches: ({})
    property var heldKeys: ({})
    property int revision: 0

    readonly property var computerKeys: ({
        "a": 0, "w": 1, "s": 2, "e": 3, "d": 4, "f": 5, "t": 6, "g": 7,
        "y": 8, "z": 8, "h": 9, "u": 10, "j": 11, "k": 12, "o": 13, "l": 14, "p": 15
    })

    focus: true

    function whiteNote(index) {
        return firstNote + 12 * Math.floor(index / 7) + whiteOffsets[index % 7]
    }
    function isBlack(note) {
        return [1, 3, 6, 8, 10].indexOf(((note % 12) + 12) % 12) >= 0
    }
    function isDown(note) {
        return (downCount[note] || 0) > 0 || litNotes[note] === true
    }
    function setLit(note, on) {
        litNotes[note] = on
        revision++
    }
    function press(note, velocity) {
        const count = downCount[note] || 0
        downCount[note] = count + 1
        revision++
        if (count === 0)
            notePressed(note, velocity)
    }
    function release(note) {
        const count = downCount[note] || 0
        if (count <= 0)
            return
        downCount[note] = count - 1
        revision++
        if (count === 1)
            noteReleased(note)
    }
    function releaseAll() {
        for (const id in touches)
            if (touches[id] >= 0)
                release(touches[id])
        for (const key in heldKeys)
            release(heldKeys[key])
        touches = ({})
        heldKeys = ({})
    }
    function noteAt(x, y) {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return -1
        if (y < blackHeight) {
            const left = Math.round(x / whiteWidth) - 1     // witte toets links van de kandidaat
            if (left >= 0 && left < whiteKeyCount - 1 && hasBlackRight[left % 7]
                    && Math.abs(x - (left + 1) * whiteWidth) <= blackWidth / 2)
                return whiteNote(left) + 1
        }
        return whiteNote(Math.min(whiteKeyCount - 1, Math.floor(x / whiteWidth)))
    }
    function velocityAt(note, y) {
        const keyHeight = isBlack(note) ? blackHeight : height
        return Math.round(45 + 82 * Math.min(1, Math.max(0, y / keyHeight)))
    }

    onFirstNoteChanged: releaseAll()
    onWhiteKeyCountChanged: releaseAll()

    Keys.onPressed: (event) => {
        const offset = computerKeys[event.text.toLowerCase()]
        if (offset === undefined || event.isAutoRepeat)
            return
        const note = firstNote + 12 + offset
        heldKeys[event.key] = note
        press(note, 100)
        event.accepted = true
    }
    Keys.onReleased: (event) => {
        if (event.isAutoRepeat || heldKeys[event.key] === undefined)
            return
        release(heldKeys[event.key])
        delete heldKeys[event.key]
        event.accepted = true
    }

    Repeater {
        model: root.whiteKeyCount
        Rectangle {
            required property int index
            readonly property int note: root.whiteNote(index)
            readonly property bool down: root.revision >= 0 && root.isDown(note)
            x: index * root.whiteWidth + 1
            width: root.whiteWidth - 2
            height: root.height
            radius: 5
            gradient: Gradient {
                GradientStop { position: 0.0; color: down ? "#7fe0d4" : "#ffffff" }
                GradientStop { position: 1.0; color: down ? "#3bb5a6" : "#e4e4de" }
            }
            Text {
                visible: note % 12 === 0
                text: "C" + (Math.floor(note / 12) - 1)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                color: "#6b6b70"
                font.pixelSize: Math.max(9, Math.min(14, root.whiteWidth * 0.32))
            }
        }
    }

    Repeater {
        model: root.whiteKeyCount - 1
        Rectangle {
            required property int index
            readonly property int note: root.whiteNote(index) + 1
            readonly property bool down: root.revision >= 0 && root.isDown(note)
            visible: root.hasBlackRight[index % 7]
            x: (index + 1) * root.whiteWidth - root.blackWidth / 2
            width: root.blackWidth
            height: root.blackHeight
            radius: 4
            gradient: Gradient {
                GradientStop { position: 0.0; color: down ? "#2aa595" : "#2c2d33" }
                GradientStop { position: 1.0; color: down ? "#1d7b70" : "#0e0e11" }
            }
        }
    }

    MultiPointTouchArea {
        anchors.fill: parent
        maximumTouchPoints: 10
        mouseEnabled: true

        function finish(points) {
            for (const point of points) {
                const note = root.touches[point.pointId]
                if (note !== undefined && note >= 0)
                    root.release(note)
                delete root.touches[point.pointId]
            }
        }

        onPressed: (points) => {
            root.forceActiveFocus()
            for (const point of points) {
                const note = root.noteAt(point.x, point.y)
                root.touches[point.pointId] = note
                if (note >= 0)
                    root.press(note, root.velocityAt(note, point.y))
            }
        }
        onUpdated: (points) => {
            for (const point of points) {
                const previous = root.touches[point.pointId]
                const note = root.noteAt(point.x, point.y)
                if (note === previous)
                    continue
                if (previous !== undefined && previous >= 0)
                    root.release(previous)
                if (note >= 0)
                    root.press(note, root.velocityAt(note, point.y))
                root.touches[point.pointId] = note
            }
        }
        onReleased: (points) => finish(points)
        onCanceled: (points) => finish(points)
    }
}
