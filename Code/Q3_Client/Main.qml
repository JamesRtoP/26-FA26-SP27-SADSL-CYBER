import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visibility: Window.Windowed
    flags: Qt.Window | Qt.FramelessWindowHint
           | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint

    readonly property var currentScreenInfo: {
        const screens = Qt.application.screens
        for (let i = 0; i < screens.length; ++i) {
            if (screens[i].name === Screen.name)
                return screens[i]
        }
        return null
    }

    readonly property rect usableScreenGeometry:
        currentScreenInfo && currentScreenInfo.availableGeometry !== undefined
        ? currentScreenInfo.availableGeometry
        : Qt.rect(Screen.virtualX, Screen.virtualY,
                  Screen.width, Screen.height - 48)

    x: usableScreenGeometry.x
    y: usableScreenGeometry.y
    width: usableScreenGeometry.width
    height: usableScreenGeometry.height
    minimumWidth: 880
    minimumHeight: 580
    title: "Q3-Chat"
    color: "#0d1110"
    font.family: "Segoe UI"

    readonly property color panel: "#131a16"
    readonly property color surface: "#1c261f"
    readonly property color divider: "#2a372e"
    readonly property color foreground: "#e7eee9"
    readonly property color muted: "#99aa9d"
    readonly property color accent: "#a3e875"

    property bool loggedIn: false
    property bool authBusy: false
    property bool chatBusy: false
    property bool sendBusy: false
    // The window visually fills the desktop without entering the
    // Windows Maximized state. This avoids restore flickering.
    property bool desktopExpanded: true
    property rect restoredGeometry: Qt.rect(0, 0, 1240, 800)

    function expandToDesktop(saveCurrentGeometry) {
        if (saveCurrentGeometry && !desktopExpanded) {
            restoredGeometry = Qt.rect(
                root.x,
                root.y,
                root.width,
                root.height
            )
        }

        const area = root.usableScreenGeometry

        root.x = area.x
        root.y = area.y
        root.width = area.width
        root.height = area.height
        root.desktopExpanded = true
    }

    function restoreDesktopWindow() {
        root.desktopExpanded = false
        root.x = restoredGeometry.x
        root.y = restoredGeometry.y
        root.width = restoredGeometry.width
        root.height = restoredGeometry.height
    }

    Component.onCompleted: {
        const area = root.usableScreenGeometry
        const normalWidth = Math.min(1240, area.width)
        const normalHeight = Math.min(800, area.height)

        restoredGeometry = Qt.rect(
            area.x + Math.round((area.width - normalWidth) / 2),
            area.y + Math.round((area.height - normalHeight) / 2),
            normalWidth,
            normalHeight
        )

        desktopExpanded = true
    }

    property string accountName: ""
    property string pendingAccount: ""
    property string pendingRecipient: ""
    property string pendingBody: ""
    property int pendingMessageChat: -1

    property string authStatus: ""
    property string chatStatus: ""
    property string messageStatus: ""

    // UI session state only. No disk storage or history loading.
    property var conversations: []
    property int selectedChat: -1

    readonly property var currentChat:
        selectedChat >= 0 && selectedChat < conversations.length
        ? conversations[selectedChat] : null

    function authenticate(registering) {
        if (authBusy)
            return

        pendingAccount = usernameField.text.trim()
        authStatus = ""
        authBusy = true

        if (registering)
            clientService.registerUser(pendingAccount, passwordField.text)
        else
            clientService.loginUser(pendingAccount, passwordField.text)
    }

    function beginChat() {
        const recipient = recipientField.text.trim()
        if (!recipient.length || chatBusy)
            return

        // Reopen an existing session conversation.
        for (let i = 0; i < conversations.length; ++i) {
            if (conversations[i].name === recipient) {
                selectedChat = i
                newChatDialog.close()
                return
            }
        }

        pendingRecipient = recipient
        chatStatus = ""
        chatBusy = true
        clientService.startChat(recipient)
    }

    function sendCurrentMessage() {
        const body = composer.text.trim()
        if (!currentChat || !body.length || sendBusy)
            return

        // Remember the target until its asynchronous response arrives.
        pendingMessageChat = selectedChat
        pendingBody = body
        messageStatus = ""
        sendBusy = true
        clientService.sendMessage(currentChat.name, body)
    }

    function formattedEventTime(timestamp) {
        if (!timestamp)
            return ""

        const date = new Date(timestamp)

        if (isNaN(date.getTime()))
            return ""

        return Qt.formatTime(date, "HH:mm")
    }

    function populateConversations(events) {
        const loadedChats = []
        const chatIndices = {}
        const myUsername = root.accountName.toLowerCase()

        for (let i = 0; i < events.length; ++i) {
            const event = events[i]
            const sender = String(event.sender || "")
            const recipient = String(event.recipient || "")

            let otherUsername = ""

            if (sender.toLowerCase() === myUsername)
                otherUsername = recipient
            else if (recipient.toLowerCase() === myUsername)
                otherUsername = sender
            else
                continue

            if (!otherUsername.length)
                continue

            const key = otherUsername.toLowerCase()
            let chatIndex = chatIndices[key]

            if (chatIndex === undefined) {
                chatIndex = loadedChats.length
                chatIndices[key] = chatIndex

                loadedChats.push({
                    name: otherUsername,
                    preview: "No messages yet",
                    messages: []
                })
            }

            if (String(event.eventType || "") === "message") {
                const body =
                    event.data === undefined
                    || event.data === null
                    ? ""
                    : String(event.data)

                if (body.length > 0) {
                    loadedChats[chatIndex].messages.push({
                        body: body,
                        time: root.formattedEventTime(
                            event.timestamp),
                        sender: sender
                    })

                    loadedChats[chatIndex].preview = body
                }
            }
        }

        root.conversations = loadedChats
        root.selectedChat =
            loadedChats.length > 0 ? 0 : -1
    }

    Connections {
        target: clientService

        function onRegistrationSucceeded() {
            root.authBusy = false
            root.authStatus =
                "Account created. You can sign in now."
        }

        function onRegistrationFailed(message) {
            root.authBusy = false
            root.authStatus =
                message
                || "Registration failed. Check the homeserver."
        }

        function onLoginSucceeded(authToken) {
            root.authBusy = false
            root.authStatus = ""
            root.accountName = root.pendingAccount
            root.loggedIn = true
            root.conversations = []
            root.selectedChat = -1
            passwordField.clear()

            clientService.loadEvents()
        }

        function onLoginFailed(message) {
            root.authBusy = false
            root.authStatus =
                message
                || "Sign-in failed. Check the homeserver."
        }

        function onEventsLoaded(events) {
            root.populateConversations(events)
        }

        function onEventsLoadFailed(message) {
            console.warn("Could not load events:", message)

            root.messageStatus =
                message
                || "Could not load conversation history."
        }

        function onChatStarted() {
            root.conversations =
                root.conversations.concat([{
                    name: root.pendingRecipient,
                    preview: "No messages yet",
                    messages: []
                }])

            root.selectedChat =
                root.conversations.length - 1

            root.chatBusy = false
            root.chatStatus = ""
            recipientField.clear()
            newChatDialog.close()
        }

        function onChatFailed(message) {
            root.chatBusy = false

            root.chatStatus =
                message
                || "Couldn't start this conversation."
        }

        function onMessageSent() {
            let updated = root.conversations.slice()

            let chat = Object.assign(
                {},
                updated[root.pendingMessageChat]
            )

            chat.messages = chat.messages.concat([{
                body: root.pendingBody,
                time: Qt.formatTime(
                    new Date(),
                    "HH:mm"
                ),
                sender: root.accountName
            }])

            chat.preview = root.pendingBody
            updated[root.pendingMessageChat] = chat
            root.conversations = updated

            root.sendBusy = false
            root.pendingMessageChat = -1
            root.pendingBody = ""
            composer.clear()

            Qt.callLater(function() {
                messageList.positionViewAtEnd()
            })
        }

        function onMessageFailed(message) {
            root.sendBusy = false
            root.pendingMessageChat = -1

            root.messageStatus =
                message
                || "Message wasn't sent. Try again."
        }
    }

    component AppButton: Button {
        id: control
        property bool primary: false

        implicitHeight: 42
        implicitWidth: Math.max(80, contentItem.implicitWidth + 28)
        hoverEnabled: true

        contentItem: Text {
            text: control.text
            color: control.primary ? "#17230f" : root.foreground
            font.pixelSize: 13
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 6
            opacity: control.enabled ? 1 : 0.45
            color: control.primary
                   ? (control.down ? "#83c45a" : control.hovered ? "#b5f08d" : root.accent)
                   : (control.down ? "#334637" : control.hovered ? "#28382d" : root.surface)
            border.color: control.activeFocus ? root.accent : "transparent"
        }
    }

    component AppField: TextField {
        id: control

        implicitHeight: 44
        leftPadding: 13
        rightPadding: 13
        color: root.foreground
        placeholderTextColor: root.muted
        selectionColor: "#446237"
        selectedTextColor: root.foreground
        font.pixelSize: 14

        background: Rectangle {
            radius: 6
            color: "#0e1510"
            border.color: control.activeFocus ? root.accent : root.divider
        }
    }

    component Avatar: Rectangle {
        property string name: ""
        implicitWidth: 40
        implicitHeight: 40
        radius: 7
        color: "#2b3c2c"

        Text {
            anchors.centerIn: parent
            text: parent.name.substring(0, 2).toUpperCase()
            color: root.accent
            font.pixelSize: 12
            font.weight: Font.Bold
        }
    }

    // Transparent custom window-control layer.
    Item {
        id: appWindowControls
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        z: 1000

        // Invisible drag region. It does not add a visible row.
        Item {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: controlButtons.left

            DragHandler {
                target: null

                onActiveChanged: {
                    if (active) {
                        // Restore before dragging a full-size window.
                        if (root.desktopExpanded)
                            root.restoreDesktopWindow()

                        root.startSystemMove()
                    }
                }
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton

                onDoubleTapped: {
                    if (root.desktopExpanded)
                        root.restoreDesktopWindow()
                    else
                        root.expandToDesktop(true)
                }
            }
        }

        // Only these buttons are visible—there is no title-bar background.
        Row {
            id: controlButtons
            anchors.top: parent.top
            anchors.right: parent.right
            height: 32
            spacing: 0

            Button {
                id: minimizeWindowButton
                width: 42
                height: 32
                hoverEnabled: true
                focusPolicy: Qt.NoFocus

                contentItem: Text {
                    text: "—"
                    color: "#C2CDD7"
                    font.family: "Segoe UI Symbol"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: minimizeWindowButton.down
                           ? "#2B3A47"
                           : minimizeWindowButton.hovered
                             ? "#20303C"
                             : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 90
                        }
                    }
                }

                Accessible.name: "Minimize window"

                ToolTip.visible: hovered
                ToolTip.text: "Minimize"
                ToolTip.delay: 500

                onClicked: root.showMinimized()
            }

            Button {
                id: expandWindowButton
                width: 42
                height: 32
                hoverEnabled: true
                focusPolicy: Qt.NoFocus

                contentItem: Text {
                    text: root.desktopExpanded ? "❐" : "□"
                    color: "#C2CDD7"
                    font.family: "Segoe UI Symbol"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: expandWindowButton.down
                           ? "#2B3A47"
                           : expandWindowButton.hovered
                             ? "#20303C"
                             : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 90
                        }
                    }
                }

                Accessible.name:
                    root.desktopExpanded
                    ? "Restore window"
                    : "Expand window"

                ToolTip.visible: hovered
                ToolTip.text:
                    root.desktopExpanded
                    ? "Restore"
                    : "Expand"
                ToolTip.delay: 500

                onClicked: {
                    if (root.desktopExpanded)
                        root.restoreDesktopWindow()
                    else
                        root.expandToDesktop(true)
                }
            }

            Button {
                id: closeWindowButton
                width: 42
                height: 32
                hoverEnabled: true
                focusPolicy: Qt.NoFocus

                contentItem: Text {
                    text: "×"
                    color: "#E8EEF5"
                    font.family: "Segoe UI Symbol"
                    font.pixelSize: 17
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: closeWindowButton.down
                           ? "#A51D24"
                           : closeWindowButton.hovered
                             ? "#C42B34"
                             : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: 90
                        }
                    }
                }

                Accessible.name: "Close window"

                ToolTip.visible: hovered
                ToolTip.text: "Close"
                ToolTip.delay: 500

                onClicked: root.close()
            }
        }
    }

    // Sign-in screen.
    Rectangle {
        id: signInScreen
        anchors.fill: parent
        visible: !root.loggedIn
        color: "#0B0F14"

        function submit(registering) {
            const usernameMissing =
                usernameField.text.trim().length === 0
            const passwordMissing =
                passwordField.text.length === 0

            if (usernameMissing || passwordMissing) {
                if (usernameMissing && passwordMissing)
                    root.authStatus = "Enter your username and password."
                else if (usernameMissing)
                    root.authStatus = "Enter your username."
                else
                    root.authStatus = "Enter your password."

                if (usernameMissing)
                    usernamePulse.restart()

                if (passwordMissing)
                    passwordPulse.restart()

                if (usernameMissing)
                    usernameField.forceActiveFocus()
                else
                    passwordField.forceActiveFocus()

                return
            }

            root.authenticate(registering)
        }

        Canvas {
            id: networkBackground
            anchors.fill: parent

            readonly property var nodes: [
                [0.03, 0.14], [0.14, 0.08], [0.26, 0.18],
                [0.08, 0.33], [0.21, 0.39], [0.33, 0.29],
                [0.02, 0.57], [0.15, 0.62], [0.29, 0.55],
                [0.07, 0.83], [0.23, 0.91], [0.35, 0.76],
                [0.45, 0.07], [0.57, 0.17], [0.65, 0.05],
                [0.73, 0.24], [0.86, 0.10], [0.97, 0.22],
                [0.67, 0.43], [0.81, 0.39], [0.94, 0.48],
                [0.72, 0.62], [0.88, 0.67], [0.99, 0.77],
                [0.64, 0.83], [0.78, 0.94], [0.91, 0.88],
                [0.43, 0.90], [0.53, 0.76]
            ]

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onVisibleChanged: {
                if (visible)
                    requestPaint()
            }

            Component.onCompleted: requestPaint()

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                const reach = Math.min(width * 0.23, 280)

                for (let i = 0; i < nodes.length; ++i) {
                    const x1 = nodes[i][0] * width
                    const y1 = nodes[i][1] * height

                    for (let j = i + 1; j < nodes.length; ++j) {
                        const x2 = nodes[j][0] * width
                        const y2 = nodes[j][1] * height
                        const dx = x2 - x1
                        const dy = y2 - y1
                        const distance = Math.sqrt(dx * dx + dy * dy)

                        if (distance < reach) {
                            const alpha =
                                0.06 + 0.18 * (1 - distance / reach)

                            ctx.strokeStyle =
                                "rgba(85,189,255," + alpha + ")"
                            ctx.lineWidth = 1
                            ctx.beginPath()
                            ctx.moveTo(x1, y1)
                            ctx.lineTo(x2, y2)
                            ctx.stroke()
                        }
                    }
                }

                for (let k = 0; k < nodes.length; ++k) {
                    const x = nodes[k][0] * width
                    const y = nodes[k][1] * height
                    const prominent = k % 5 === 0

                    if (prominent) {
                        ctx.strokeStyle =
                            "rgba(85,189,255,0.25)"
                        ctx.lineWidth = 1
                        ctx.beginPath()
                        ctx.arc(x, y, 7, 0, Math.PI * 2)
                        ctx.stroke()
                    }

                    ctx.fillStyle =
                        prominent ? "#55BDFF" : "#376783"
                    ctx.beginPath()
                    ctx.arc(
                        x,
                        y,
                        prominent ? 2.5 : 1.8,
                        0,
                        Math.PI * 2
                    )
                    ctx.fill()
                }
            }
        }

        ScrollView {
            id: signInScroll
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth
            contentHeight: Math.max(
                availableHeight,
                signInCard.implicitHeight + 48
            )

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Item {
                width: signInScroll.availableWidth
                height: signInScroll.contentHeight

                Rectangle {
                    id: signInCard
                    anchors.centerIn: parent
                    width: Math.min(380, parent.width - 48)
                    implicitHeight: signInForm.implicitHeight + 64
                    height: implicitHeight
                    radius: 12
                    color: "#141B24"
                    border.color: "#2B3645"
                    border.width: 1

                    ColumnLayout {
                        id: signInForm
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 32
                        spacing: 0

                        Text {
                            Layout.fillWidth: true
                            text: "Q3-Chat"
                            color: "#55BDFF"
                            font.family: "ROG FONTS"
                            font.pixelSize: 33
                            font.weight: Font.Bold
                            horizontalAlignment: Text.AlignHCenter
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 22
                            Layout.bottomMargin: 28
                            spacing: 12

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: "#34465A"
                            }

                            Text {
                                text: "SIGN IN"
                                color: "#A9B8C8"
                                font.family: "Consolas"
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 2
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: "#34465A"
                            }
                        }

                        Text {
                            text: "Username"
                            color: "#E8EEF5"
                            font.family: "Consolas"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                        }

                        TextField {
                            id: usernameField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            Layout.topMargin: 8
                            Layout.bottomMargin: 20

                            enabled: !root.authBusy
                            selectByMouse: true
                            activeFocusOnTab: true

                            leftPadding: 14
                            rightPadding: 14
                            topPadding: 0
                            bottomPadding: 0

                            horizontalAlignment: TextInput.AlignLeft
                            verticalAlignment: TextInput.AlignVCenter

                            font.family: "Consolas"
                            font.pixelSize: 14
                            color: "#E8EEF5"
                            selectionColor: "#245F86"
                            selectedTextColor: "#FFFFFF"
                            opacity: enabled ? 1 : 0.65
                            Accessible.name: "Username"

                            background: Rectangle {
                                radius: 6
                                color: "#0B0F14"
                                border.color:
                                    usernameField.activeFocus
                                    ? "#55BDFF" : "#374456"
                                border.width:
                                    usernameField.activeFocus ? 2 : 1
                            }

                            SequentialAnimation {
                                id: usernamePulse
                                loops: 2

                                NumberAnimation {
                                    target: usernameField
                                    property: "scale"
                                    from: 1.0
                                    to: 1.02
                                    duration: 90
                                }

                                NumberAnimation {
                                    target: usernameField
                                    property: "scale"
                                    from: 1.02
                                    to: 1.0
                                    duration: 130
                                }
                            }

                            onTextEdited: root.authStatus = ""
                            onAccepted: passwordField.forceActiveFocus()
                        }

                        Text {
                            text: "Password"
                            color: "#E8EEF5"
                            font.family: "Consolas"
                            font.pixelSize: 13
                            font.weight: Font.Medium
                        }

                        TextField {
                            id: passwordField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            Layout.topMargin: 8

                            enabled: !root.authBusy
                            echoMode: TextInput.Password
                            selectByMouse: true
                            activeFocusOnTab: true

                            leftPadding: 14
                            rightPadding: 14
                            topPadding: 0
                            bottomPadding: 0

                            horizontalAlignment: TextInput.AlignLeft
                            verticalAlignment: TextInput.AlignVCenter

                            font.family: "Consolas"
                            font.pixelSize: 14
                            color: "#E8EEF5"
                            selectionColor: "#245F86"
                            selectedTextColor: "#FFFFFF"
                            opacity: enabled ? 1 : 0.65
                            Accessible.name: "Password"

                            background: Rectangle {
                                radius: 6
                                color: "#0B0F14"
                                border.color:
                                    passwordField.activeFocus
                                    ? "#55BDFF" : "#374456"
                                border.width:
                                    passwordField.activeFocus ? 2 : 1
                            }

                            SequentialAnimation {
                                id: passwordPulse
                                loops: 2

                                NumberAnimation {
                                    target: passwordField
                                    property: "scale"
                                    from: 1.0
                                    to: 1.02
                                    duration: 90
                                }

                                NumberAnimation {
                                    target: passwordField
                                    property: "scale"
                                    from: 1.02
                                    to: 1.0
                                    duration: 130
                                }
                            }

                            onTextEdited: root.authStatus = ""

                            onAccepted: {
                                signInScreen.submit(false)
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.topMargin: visible ? 16 : 0
                            visible: root.authStatus.length > 0

                            text: root.authStatus
                            textFormat: Text.PlainText
                            color: "#E8EEF5"
                            font.family: "Consolas"
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            lineHeight: 1.2

                            Accessible.role: Accessible.StaticText
                            Accessible.name: text
                        }

                        Button {
                            id: loginButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            Layout.topMargin: 24

                            text: root.authBusy
                                  ? "Please wait…" : "Sign in"
                            enabled: !root.authBusy
                            hoverEnabled: true
                            activeFocusOnTab: true
                            opacity: enabled ? 1 : 0.55

                            contentItem: Text {
                                text: loginButton.text
                                color: "#07131D"
                                font.family: "Consolas"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 6
                                color: loginButton.down ? "#329FDE"
                                     : loginButton.hovered ? "#7ACBFF"
                                     : "#55BDFF"
                                border.color:
                                    loginButton.activeFocus
                                    ? "#FFFFFF" : "transparent"
                                border.width: 2
                            }

                            onClicked: signInScreen.submit(false)
                        }

                        Button {
                            id: registerButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            Layout.topMargin: 10

                            text: "Create account"
                            enabled: !root.authBusy
                            hoverEnabled: true
                            activeFocusOnTab: true
                            opacity: enabled ? 1 : 0.55

                            contentItem: Text {
                                text: registerButton.text
                                color: "#E8EEF5"
                                font.family: "Consolas"
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 6
                                color: registerButton.down ? "#253345"
                                     : registerButton.hovered ? "#1C2836"
                                     : "transparent"
                                border.color:
                                    registerButton.activeFocus
                                    ? "#55BDFF" : "#374456"
                                border.width:
                                    registerButton.activeFocus ? 2 : 1
                            }

                            onClicked: signInScreen.submit(true)
                        }
                    }
                }
            }
        }
    }
    // Main workspace.
    Rectangle {
        id: workspace
        anchors.fill: parent
        visible: root.loggedIn
        color: "#0B0F14"

        SplitView {
            anchors.fill: parent
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 6

                color: SplitHandle.pressed
                       ? "#55BDFF"
                       : SplitHandle.hovered
                         ? "#263D4D"
                         : "#182630"

                Behavior on color {
                    ColorAnimation {
                        duration: 100
                    }
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    anchors.centerIn: parent
                    color: "#304453"
                }
            }

            // Unified sidebar.
            Rectangle {
                id: sidebar
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: 235
                SplitView.maximumWidth: 440
                color: "#111820"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Q3 brand header.
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        color: "#0D141B"

                        gradient: Gradient {
                            orientation: Gradient.Horizontal

                            GradientStop {
                                position: 0.0
                                color: "#122230"
                            }

                            GradientStop {
                                position: 0.55
                                color: "#0D171F"
                            }

                            GradientStop {
                                position: 1.0
                                color: "#101922"
                            }
                        }

                        // Static constellation background.
                        // It only redraws if the header is resized.
                        Canvas {
                            id: headerStars
                            anchors.fill: parent
                            opacity: 0.55

                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                const points = [
                                    [0.08, 0.22],
                                    [0.24, 0.70],
                                    [0.43, 0.28],
                                    [0.61, 0.62],
                                    [0.78, 0.18],
                                    [0.93, 0.54]
                                ]

                                const links = [
                                    [0, 2],
                                    [1, 2],
                                    [2, 3],
                                    [3, 4],
                                    [3, 5]
                                ]

                                ctx.lineWidth = 1
                                ctx.strokeStyle = "#23506A"

                                for (let i = 0; i < links.length; ++i) {
                                    const first = points[links[i][0]]
                                    const second = points[links[i][1]]

                                    ctx.beginPath()
                                    ctx.moveTo(
                                        first[0] * width,
                                        first[1] * height
                                    )
                                    ctx.lineTo(
                                        second[0] * width,
                                        second[1] * height
                                    )
                                    ctx.stroke()
                                }

                                ctx.fillStyle = "#3A7597"

                                for (let j = 0; j < points.length; ++j) {
                                    ctx.beginPath()
                                    ctx.arc(
                                        points[j][0] * width,
                                        points[j][1] * height,
                                        j === 2 ? 2 : 1.4,
                                        0,
                                        Math.PI * 2
                                    )
                                    ctx.fill()
                                }
                            }
                        }

                        Rectangle {
                            width: 46
                            height: 46
                            radius: 8
                            anchors.left: parent.left
                            anchors.leftMargin: 22
                            anchors.verticalCenter: parent.verticalCenter
                            color: "#55BDFF"

                            Text {
                                anchors.centerIn: parent
                                text: "Q3"
                                color: "#07131D"
                                font.family: "ROG FONTS"
                                font.pixelSize: 17
                                font.weight: Font.Bold
                            }
                        }

                        Rectangle {
                            height: 1
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            color: "#1D2A35"
                        }
                    }

                    // Conversation section heading and add button.
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 22
                        Layout.rightMargin: 22
                        Layout.topMargin: 24
                        Layout.bottomMargin: 14
                        spacing: 12

                        Text {
                            Layout.fillWidth: true
                            text: "Messages"
                            color: "#E8EEF5"
                            font.family: "Consolas"
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }

                        Button {
                            id: addConversationButton
                            Layout.preferredWidth: 34
                            Layout.preferredHeight: 34
                            hoverEnabled: true

                            contentItem: Text {
                                text: "+"
                                color: addConversationButton.down
                                       ? "#E8EEF5"
                                       : "#55BDFF"
                                font.family: "Consolas"
                                font.pixelSize: 22
                                font.weight: Font.Normal
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: width / 2
                                color: addConversationButton.down
                                       ? "#2B4354"
                                       : addConversationButton.hovered
                                         ? "#223340"
                                         : "#192630"

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 100
                                    }
                                }
                            }

                            Accessible.name: "New conversation"

                            ToolTip.visible: hovered
                            ToolTip.text: "New conversation"
                            ToolTip.delay: 500

                            onClicked: {
                                root.chatStatus = ""
                                newChatDialog.open()
                                recipientField.forceActiveFocus()
                            }
                        }
                    }

                    TextField {
                        id: search
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        Layout.leftMargin: 22
                        Layout.rightMargin: 22
                        Layout.bottomMargin: 20

                        placeholderText: "Search conversations"
                        placeholderTextColor: "#71869A"
                        color: "#E8EEF5"
                        selectionColor: "#245F86"
                        selectedTextColor: "#FFFFFF"
                        font.family: "Consolas"
                        font.pixelSize: 13

                        leftPadding: 13
                        rightPadding: 13
                        topPadding: 0
                        bottomPadding: 0
                        verticalAlignment: TextInput.AlignVCenter

                        background: Rectangle {
                            radius: 6
                            color: "#18222C"
                            border.color: search.activeFocus
                                          ? "#55BDFF"
                                          : "transparent"
                            border.width: 2
                        }

                        Accessible.name: "Search conversations"
                    }

                    ListView {
                        id: conversationList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12

                        clip: true
                        spacing: 3
                        model: root.conversations

                        ScrollBar.vertical: ScrollBar {}

                        // Deliberate empty state.
                        Column {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 62
                            spacing: 14
                            visible: conversationList.count === 0

                            Item {
                                width: 52
                                height: 42
                                anchors.horizontalCenter: parent.horizontalCenter

                                Rectangle {
                                    id: conversationBubble
                                    anchors.top: parent.top
                                    anchors.horizontalCenter:
                                        parent.horizontalCenter
                                    width: 48
                                    height: 34
                                    radius: 9
                                    color: "#1B2B38"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "•••"
                                        color: "#55BDFF"
                                        font.family: "Consolas"
                                        font.pixelSize: 14
                                        font.letterSpacing: 2
                                    }
                                }

                                Rectangle {
                                    anchors.left: conversationBubble.left
                                    anchors.leftMargin: 8
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 4
                                    width: 11
                                    height: 11
                                    rotation: 45
                                    color: "#1B2B38"
                                }
                            }

                            Text {
                                width: 220
                                text: search.text.length > 0
                                      ? "No matching conversations"
                                      : "No conversations yet"
                                color: "#A3B3C3"
                                font.family: "Consolas"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                            }
                        }

                        delegate: ItemDelegate {
                            id: conversationRow

                            required property int index
                            required property var modelData

                            readonly property bool matchesSearch:
                                conversationRow.modelData.name
                                    .toLowerCase()
                                    .includes(search.text.toLowerCase())

                            width: ListView.view.width
                            height: matchesSearch ? 64 : 0
                            visible: matchesSearch
                            enabled: !root.sendBusy
                            padding: 8
                            hoverEnabled: true

                            onClicked: {
                                root.selectedChat = conversationRow.index
                                root.messageStatus = ""
                                composer.clear()

                                Qt.callLater(function() {
                                    messageList.positionViewAtEnd()
                                })
                            }

                            background: Rectangle {
                                radius: 6
                                color:
                                    conversationRow.index
                                    === root.selectedChat
                                    ? "#1C2B38"
                                    : conversationRow.hovered
                                      ? "#17222C"
                                      : "transparent"

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.verticalCenter:
                                        parent.verticalCenter
                                    width: 3
                                    height: 24
                                    radius: 2
                                    color: "#55BDFF"

                                    visible:
                                        conversationRow.index
                                        === root.selectedChat
                                }
                            }

                            contentItem: RowLayout {
                                spacing: 10

                                Rectangle {
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    radius: 18
                                    color: "#24394A"

                                    Text {
                                        anchors.centerIn: parent
                                        text:
                                            conversationRow.modelData.name
                                                .substring(0, 2)
                                                .toUpperCase()
                                        color: "#70CBFF"
                                        font.family: "Consolas"
                                        font.pixelSize: 11
                                        font.weight: Font.Bold
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    Text {
                                        Layout.fillWidth: true
                                        text:
                                            conversationRow.modelData.name
                                        textFormat: Text.PlainText
                                        color: "#E8EEF5"
                                        font.family: "Consolas"
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text:
                                            conversationRow.modelData.preview
                                        textFormat: Text.PlainText
                                        color: "#8193A5"
                                        font.family: "Consolas"
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: "#263440"
                    }

                    // Current account.
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        Layout.topMargin: 15
                        Layout.bottomMargin: 15
                        spacing: 11

                        Rectangle {
                            Layout.preferredWidth: 38
                            Layout.preferredHeight: 38
                            radius: 19
                            color: "#24394A"

                            Text {
                                anchors.centerIn: parent
                                text: root.accountName
                                      .substring(0, 2)
                                      .toUpperCase()
                                color: "#70CBFF"
                                font.family: "Consolas"
                                font.pixelSize: 11
                                font.weight: Font.Bold
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Text {
                                Layout.fillWidth: true
                                text: root.accountName
                                textFormat: Text.PlainText
                                color: "#E8EEF5"
                                font.family: "Consolas"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }

                            RowLayout {
                                spacing: 6

                                Rectangle {
                                    Layout.preferredWidth: 7
                                    Layout.preferredHeight: 7
                                    radius: 4
                                    color: "#52F28A"
                                }

                                Text {
                                    text: "CONNECTED"
                                    color: "#52F28A"
                                    font.family: "Consolas"
                                    font.pixelSize: 9
                                    font.letterSpacing: 0.8
                                }
                            }
                        }
                    }
                }
            }

            // Conversation area.
            ColumnLayout {
                id: conversationPane
                SplitView.fillWidth: true
                SplitView.minimumWidth: 520
                spacing: 0
                // Deep-space conversation header.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight:
                        root.currentChat ? 82 : 0

                    visible: root.currentChat !== null
                    color: "#101216"

                    gradient: Gradient {
                        orientation: Gradient.Horizontal

                        GradientStop {
                            position: 0.0
                            color: "#171A1F"
                        }

                        GradientStop {
                            position: 0.55
                            color: "#101217"
                        }

                        GradientStop {
                            position: 1.0
                            color: "#090B0F"
                        }
                    }

                    Canvas {
                        anchors.fill: parent
                        opacity: 0.55

                        Component.onCompleted: requestPaint()
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)

                            const points = [
                                [0.08, 0.28],
                                [0.31, 0.68],
                                [0.52, 0.21],
                                [0.73, 0.62],
                                [0.91, 0.25]
                            ]

                            for (let i = 0;
                                 i < points.length;
                                 ++i) {
                                ctx.fillStyle =
                                    i === 3
                                    ? "rgba(239,203,139,0.42)"
                                    : "rgba(205,215,224,0.26)"

                                ctx.beginPath()
                                ctx.arc(
                                    points[i][0] * width,
                                    points[i][1] * height,
                                    i === 1 ? 1.3 : 0.85,
                                    0,
                                    Math.PI * 2
                                )
                                ctx.fill()
                            }
                        }
                    }

                    RowLayout {
                        anchors.left: parent.left
                        anchors.leftMargin: 28
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 9

                        Text {
                            Layout.alignment: Qt.AlignBaseline
                            text: "Messages"
                            color: "#85909B"
                            font.family: "Consolas"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            Layout.alignment: Qt.AlignBaseline
                            text: "/"
                            color: "#AAB4BE"
                            font.family: "Consolas"
                            font.pixelSize: 12
                            font.weight: Font.Bold
                        }

                        Text {
                            Layout.alignment: Qt.AlignBaseline

                            text: root.currentChat
                                  ? root.currentChat.name
                                  : ""

                            textFormat: Text.PlainText
                            color: "#E8EEF5"
                            font.family: "Consolas"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: "#252B32"
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // Static deep-space conversation background.
                    Canvas {
                        id: conversationSpace
                        anchors.fill: parent

                        readonly property var stars: [
                            [0.04, 0.10], [0.12, 0.32],
                            [0.18, 0.72], [0.24, 0.18],
                            [0.31, 0.51], [0.38, 0.86],
                            [0.44, 0.27], [0.49, 0.64],
                            [0.55, 0.12], [0.61, 0.43],
                            [0.67, 0.79], [0.72, 0.23],
                            [0.78, 0.57], [0.83, 0.89],
                            [0.88, 0.35], [0.94, 0.68],
                            [0.98, 0.16], [0.08, 0.91],
                            [0.35, 0.07], [0.58, 0.94],
                            [0.75, 0.06], [0.91, 0.82]
                        ]

                        Component.onCompleted: requestPaint()
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)

                            // Near-black purple base.
                            const background =
                                ctx.createLinearGradient(
                                    0,
                                    0,
                                    width,
                                    height
                                )

                            background.addColorStop(
                                0.0,
                                "#0D0F12"
                            )

                            background.addColorStop(
                                0.55,
                                "#090B0E"
                            )

                            background.addColorStop(
                                1.0,
                                "#060709"
                            )

                            ctx.fillStyle = background
                            ctx.fillRect(0, 0, width, height)

                            // Very subtle plum nebula.
                            const nebula =
                                ctx.createRadialGradient(
                                    width * 0.72,
                                    height * 0.24,
                                    0,
                                    width * 0.72,
                                    height * 0.24,
                                    Math.max(width, height) * 0.72
                                )

                            nebula.addColorStop(
                                0.0,
                                "rgba(78,86,96,0.10)"
                            )

                            nebula.addColorStop(
                                0.42,
                                "rgba(52,58,66,0.05)"
                            )

                            nebula.addColorStop(
                                1.0,
                                "rgba(0,0,0,0)"
                            )

                            ctx.fillStyle = nebula
                            ctx.fillRect(0, 0, width, height)

                            // Sparse, deterministic stars.
                            for (let i = 0;
                                 i < stars.length;
                                 ++i) {
                                const x = stars[i][0] * width
                                const y = stars[i][1] * height
                                const warm = i % 7 === 0
                                const radius =
                                    i % 5 === 0 ? 1.25 : 0.75

                                ctx.fillStyle = warm
                                    ? "rgba(239,203,139,0.40)"
                                    : "rgba(205,215,224,0.24)"

                                ctx.beginPath()
                                ctx.arc(
                                    x,
                                    y,
                                    radius,
                                    0,
                                    Math.PI * 2
                                )
                                ctx.fill()
                            }
                        }
                    }
                    // Empty workspace.
                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: root.currentChat === null
                        spacing: 14

                        Canvas {
                            id: cougarMark
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 115
                            Layout.alignment: Qt.AlignHCenter
                            opacity: 0.9

                            Component.onCompleted: requestPaint()
                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                ctx.strokeStyle =
                                    "rgba(88,24,34,0.92)"
                                ctx.fillStyle =
                                    "rgba(108,28,40,0.88)"

                                ctx.shadowColor =
                                    "rgba(128,28,43,0.34)"
                                ctx.shadowBlur = 10
                                ctx.shadowOffsetX = 0
                                ctx.shadowOffsetY = 0

                                ctx.lineWidth = 2.5
                                ctx.lineCap = "round"
                                ctx.lineJoin = "round"

                                ctx.beginPath()
                                ctx.moveTo(26, 38)
                                ctx.lineTo(42, 18)
                                ctx.lineTo(52, 34)
                                ctx.quadraticCurveTo(70, 27, 88, 34)
                                ctx.lineTo(98, 18)
                                ctx.lineTo(114, 38)
                                ctx.quadraticCurveTo(119, 61, 105, 79)
                                ctx.quadraticCurveTo(89, 98, 70, 101)
                                ctx.quadraticCurveTo(51, 98, 35, 79)
                                ctx.quadraticCurveTo(21, 61, 26, 38)
                                ctx.stroke()

                                ctx.beginPath()
                                ctx.moveTo(34, 38)
                                ctx.lineTo(42, 27)
                                ctx.lineTo(48, 38)
                                ctx.moveTo(92, 38)
                                ctx.lineTo(98, 27)
                                ctx.lineTo(106, 38)
                                ctx.stroke()

                                ctx.beginPath()
                                ctx.moveTo(42, 53)
                                ctx.lineTo(59, 58)
                                ctx.moveTo(81, 58)
                                ctx.lineTo(98, 53)
                                ctx.stroke()

                                ctx.beginPath()
                                ctx.arc(54, 60, 2.5, 0, Math.PI * 2)
                                ctx.arc(86, 60, 2.5, 0, Math.PI * 2)
                                ctx.fill()

                                ctx.beginPath()
                                ctx.moveTo(64, 70)
                                ctx.lineTo(76, 70)
                                ctx.lineTo(70, 77)
                                ctx.closePath()
                                ctx.fill()

                                ctx.beginPath()
                                ctx.moveTo(70, 77)
                                ctx.lineTo(70, 84)
                                ctx.quadraticCurveTo(62, 89, 55, 84)
                                ctx.moveTo(70, 84)
                                ctx.quadraticCurveTo(78, 89, 85, 84)
                                ctx.stroke()
                            }
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "WSU DATA SCIENCE LAB"
                            color: "#5E6872"
                            font.family: "Consolas"
                            font.pixelSize: 9
                            font.letterSpacing: 1.4
                        }
                    }

                    ListView {
                        id: messageList
                        anchors.fill: parent
                        anchors.leftMargin: 28
                        anchors.rightMargin: 28
                        anchors.topMargin: 22
                        anchors.bottomMargin: 18

                        visible: root.currentChat !== null
                        clip: true
                        spacing: 22
                        model:
                            root.currentChat
                            ? root.currentChat.messages
                            : []

                        ScrollBar.vertical: ScrollBar {}

                        Text {
                            anchors.centerIn: parent
                            visible: messageList.count === 0
                            text: "No messages yet"
                            color: "#7D8791"
                            font.family: "Consolas"
                            font.pixelSize: 11
                        }

                        delegate: RowLayout {
                            id: messageRow

                            required property var modelData

                            width: ListView.view.width
                            spacing: 12

                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 34
                                Layout.alignment: Qt.AlignTop
                                radius: 17
                                color: "#24394A"

                                Text {
                                    anchors.centerIn: parent
                                    text: String(
                                              messageRow.modelData.sender
                                              || root.accountName
                                          ).substring(0, 2).toUpperCase()
                                    color: "#70CBFF"
                                    font.family: "Consolas"
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                RowLayout {
                                    spacing: 9

                                    Text {
                                        text: messageRow.modelData.sender
                                              || root.accountName
                                        textFormat: Text.PlainText
                                        color: "#70CBFF"
                                        font.family: "Consolas"
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }

                                    Text {
                                        text:
                                            messageRow.modelData.time
                                        color: "#65798B"
                                        font.family: "Consolas"
                                        font.pixelSize: 9
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: messageRow.modelData.body
                                    textFormat: Text.PlainText
                                    color: "#DDE6EE"
                                    font.family: "Consolas"
                                    font.pixelSize: 13
                                    wrapMode: Text.Wrap
                                    lineHeight: 1.3
                                }
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 28
                    Layout.rightMargin: 28
                    Layout.bottomMargin: visible ? 8 : 0
                    visible: text.length > 0

                    text: root.messageStatus
                    textFormat: Text.PlainText
                    color: "#FFB86B"
                    font.family: "Consolas"
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight:
                        root.currentChat ? 60 : 0
                    Layout.leftMargin: 28
                    Layout.rightMargin: 28
                    Layout.bottomMargin:
                        root.currentChat ? 18 : 0
                    visible: root.currentChat !== null

                    radius: 7
                    color: "#18222C"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 9

                        TextField {
                            id: composer
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            enabled: !root.sendBusy
                            placeholderText:
                                root.currentChat
                                ? "Message @" + root.currentChat.name
                                : ""
                            placeholderTextColor: "#71869A"
                            color: "#E8EEF5"
                            selectionColor: "#245F86"
                            selectedTextColor: "#FFFFFF"
                            font.family: "Consolas"
                            font.pixelSize: 13

                            leftPadding: 6
                            rightPadding: 6
                            topPadding: 0
                            bottomPadding: 0
                            verticalAlignment: TextInput.AlignVCenter
                            background: Item {}

                            Accessible.name: "Message"

                            onAccepted:
                                root.sendCurrentMessage()
                        }

                        Button {
                            id: sendButton
                            Layout.preferredWidth: 82
                            Layout.fillHeight: true

                            enabled:
                                !root.sendBusy
                                && composer.text.trim().length > 0

                            hoverEnabled: enabled
                            opacity: 1

                            contentItem: Text {
                                text: root.sendBusy ? "..." : "SEND"

                                color: sendButton.enabled
                                       ? "#07131D"
                                       : "#9FB1C0"

                                font.family: "Consolas"
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 6

                                color: !sendButton.enabled
                                       ? "#1D2B36"
                                       : sendButton.down
                                         ? "#329FDE"
                                         : sendButton.hovered
                                           ? "#7ACBFF"
                                           : "#55BDFF"

                                border.width:
                                    sendButton.enabled ? 0 : 1

                                border.color: "#304452"

                                Behavior on color {
                                    ColorAnimation {
                                        duration: 100
                                    }
                                }
                            }

                            onClicked:
                                root.sendCurrentMessage()
                        }
                    }
                }
            }
        }
    }

    // New-conversation dialog.
    Popup {
        id: newChatDialog
        parent: Overlay.overlay
        anchors.centerIn: parent

        width: Math.min(
            400,
            parent.width - 48
        )
        padding: 26
        modal: true
        focus: true

        function submit() {
            if (recipientField.text.trim().length === 0) {
                root.chatStatus = "Enter a username."
                recipientPulse.restart()
                recipientField.forceActiveFocus()
                return
            }

            root.beginChat()
        }

        closePolicy:
            root.chatBusy
            ? Popup.NoAutoClose
            : Popup.CloseOnEscape
              | Popup.CloseOnPressOutside

        Overlay.modal: Rectangle {
            color: "#B305090D"
        }

        background: Rectangle {
            color: "#141B24"
            radius: 10
        }

        contentItem: ColumnLayout {
            spacing: 0

            Text {
                text: "New conversation"
                color: "#E8EEF5"
                font.family: "Consolas"
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Text {
                Layout.topMargin: 8
                Layout.bottomMargin: 22
                text: "Enter a username"
                color: "#8193A5"
                font.family: "Consolas"
                font.pixelSize: 11
            }

            TextField {
                id: recipientField
                Layout.fillWidth: true
                Layout.preferredHeight: 46

                enabled: !root.chatBusy
                placeholderText: "Username"
                placeholderTextColor: "#71869A"
                color: "#E8EEF5"
                selectionColor: "#245F86"
                selectedTextColor: "#FFFFFF"
                font.family: "Consolas"
                font.pixelSize: 13

                leftPadding: 13
                rightPadding: 13
                topPadding: 0
                bottomPadding: 0
                verticalAlignment: TextInput.AlignVCenter

                background: Rectangle {
                    radius: 6
                    color: "#1A2530"
                    border.color:
                        recipientField.activeFocus
                        ? "#55BDFF" : "transparent"
                    border.width: 2
                }

                SequentialAnimation {
                    id: recipientPulse
                    loops: 2

                    NumberAnimation {
                        target: recipientField
                        property: "scale"
                        from: 1
                        to: 1.02
                        duration: 90
                    }

                    NumberAnimation {
                        target: recipientField
                        property: "scale"
                        from: 1.02
                        to: 1
                        duration: 130
                    }
                }

                Accessible.name: "Recipient username"

                onTextEdited: root.chatStatus = ""
                onAccepted: newChatDialog.submit()
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: visible ? 12 : 0
                visible: root.chatStatus.length > 0

                text: root.chatStatus
                textFormat: Text.PlainText
                color: "#FFB86B"
                font.family: "Consolas"
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 22
                spacing: 10

                Button {
                    id: cancelChatButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42

                    enabled: !root.chatBusy
                    hoverEnabled: true
                    opacity: enabled ? 1 : 0.45

                    contentItem: Text {
                        text: "Cancel"
                        color: "#E8EEF5"
                        font.family: "Consolas"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 6
                        color: cancelChatButton.down
                               ? "#202D39"
                               : cancelChatButton.hovered
                                 ? "#243441"
                                 : "#1A2530"
                    }

                    onClicked: newChatDialog.close()
                }

                Button {
                    id: startButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42

                    enabled: !root.chatBusy && !root.sendBusy
                    hoverEnabled: true
                    opacity: enabled ? 1 : 0.45

                    contentItem: Text {
                        text: root.chatBusy
                              ? "Starting..."
                              : "Start chat"
                        color: "#07131D"
                        font.family: "Consolas"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 6
                        color: startButton.down
                               ? "#329FDE"
                               : startButton.hovered
                                 ? "#7ACBFF"
                                 : "#55BDFF"
                    }

                    onClicked: newChatDialog.submit()
                }
            }
        }
    }
}