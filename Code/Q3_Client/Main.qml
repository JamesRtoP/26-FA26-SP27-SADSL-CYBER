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
    color: root.chatBackground
    font.family: "Segoe UI"
    // Session preferences.
    property bool lightChat: false
    property int chatFontSize: 16

    // Shared application colors: light theme first, dark theme second.
    readonly property color chatBackground:
        lightChat ? "#FFFFFF" : "#0D0F12"

    readonly property color panel:
        lightChat ? "#F4F6F9" : "#111820"

    readonly property color surface:
        lightChat ? "#FFFFFF" : "#18222C"

    readonly property color headerColor:
        lightChat ? "#F4F6F9" : "#101820"

    readonly property color foreground:
        lightChat ? "#17212B" : "#E8EEF5"

    readonly property color muted:
        lightChat ? "#526477" : "#9AAEBF"

    readonly property color divider:
        lightChat ? "#D5DEE8" : "#304452"

    readonly property color accent:
        lightChat ? "#096DAA" : "#55BDFF"

    readonly property color accentHover:
        lightChat ? "#075C91" : "#7ACBFF"

    readonly property color accentPressed:
        lightChat ? "#064E7B" : "#329FDE"

    readonly property color onAccent:
        lightChat ? "#FFFFFF" : "#07131D"

    readonly property color hoverSurface:
        lightChat ? "#E7EDF4" : "#243441"

    readonly property color pressedSurface:
        lightChat ? "#D7E2ED" : "#304452"

    readonly property color selectedSurface:
        lightChat ? "#DDEFFC" : "#1C2B38"

    readonly property color avatarSurface:
        lightChat ? "#DEEBF6" : "#24394A"

    readonly property color selection:
        lightChat ? "#096DAA" : "#245F86"

    readonly property color selectedText: "#FFFFFF"

    readonly property color success:
        lightChat ? "#17703D" : "#52F28A"

    readonly property color warning:
        lightChat ? "#964500" : "#FFB86B"

    readonly property color scrim:
        lightChat ? "#660F172A" : "#B305090D"

    readonly property color chatInputBackground:
        lightChat ? "#EDF2F7" : "#18222C"

    // Existing chat components use these names.
    readonly property color chatText: foreground
    readonly property color chatMuted: muted
    readonly property color chatSender: accent

    // Colors inherited by built-in Qt controls.
    palette.window: root.panel
    palette.windowText: root.foreground
    palette.base: root.chatBackground
    palette.alternateBase: root.surface
    palette.text: root.foreground
    palette.button: root.surface
    palette.buttonText: root.foreground
    palette.highlight: root.selection
    palette.highlightedText: root.selectedText
    palette.toolTipBase: root.surface
    palette.toolTipText: root.foreground
    palette.placeholderText: root.muted
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
    property double pendingSentAt: 0
    property string pendingMessageRecipient: ""
    property bool historyBusy: false

    property string authStatus: ""
    property string chatStatus: ""
    property string messageStatus: ""

    // UI session state only. No disk storage or history loading.
    property var conversations: []
    property int selectedChat: -1

    readonly property var currentChat:
        selectedChat >= 0 && selectedChat < conversations.length
        ? conversations[selectedChat] : null
    ListModel {
        id: conversationViewModel
        dynamicRoles: true
    }

    ListModel {
        id: messageViewModel
        dynamicRoles: true
    }

    onSelectedChatChanged: reloadMessageView()

    function reloadMessageView() {
        if (!messageViewModel || !messageList)
            return

        messageViewModel.clear()

        const chat = root.selectedChat >= 0
                     && root.selectedChat < root.conversations.length
                     ? root.conversations[root.selectedChat] : null

        if (!chat)
            return

        for (let i = 0; i < chat.messages.length; ++i)
            messageViewModel.append({ entry: chat.messages[i] })

        messageList.forceLayout()
        messageList.positionViewAtEnd()
    }
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
        if (!recipient.length || chatBusy || sendBusy || historyBusy)
            return

        // Reopen an existing session conversation.
        for (let i = 0; i < conversations.length; ++i) {
            if (conversations[i].name.toLowerCase() === recipient.toLowerCase()) {
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
        if (!currentChat || !body.length || sendBusy || chatBusy || historyBusy)
            return

        // Remember the target until its asynchronous response arrives.
        pendingMessageRecipient = currentChat.name
        pendingBody = body
        pendingSentAt = Date.now()
        messageStatus = ""
        sendBusy = true
        clientService.sendMessage(currentChat.name, body)
    }

    function makeMessage(body, sender, timestamp) {
        const missing = timestamp === undefined
                        || timestamp === null || timestamp === ""
        const date = missing ? new Date(NaN) : new Date(timestamp)
        const valid = !isNaN(date.getTime())

        return {
            body: body,
            sender: sender,
            timestamp: valid ? date.getTime() : null,
            time: valid ? Qt.formatTime(date, "h:mm AP") : "",
            dayKey: valid ? Qt.formatDate(date, "yyyy-MM-dd") : "unknown",
            dayLabel: valid ? Qt.formatDate(date, "MMMM d, yyyy")
                            : "Date unavailable"
        }
    }

    function sortMessages(messages) {
        messages.sort(function(a, b) {
            if (a.timestamp === null)
                return b.timestamp === null ? 0 : -1
            if (b.timestamp === null)
                return 1
            return a.timestamp - b.timestamp
        })
    }
    function populateConversations(events) {
        const loadedChats = []
        const chatIndices = Object.create(null)
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
                    loadedChats[chatIndex].messages.push(
                        root.makeMessage(body, sender, event.timestamp)
                    )

                    loadedChats[chatIndex].preview = body
                }
            }
        }
        // Put each conversation's messages in chronological order.
        for (let t = 0; t < loadedChats.length; ++t) {
            const messages = loadedChats[t].messages
            root.sortMessages(messages)

            if (messages.length > 0)
                loadedChats[t].preview = messages[messages.length - 1].body
        }
        root.conversations = loadedChats

        conversationViewModel.clear()
        for (let h = 0; h < loadedChats.length; ++h) {
            conversationViewModel.append({
                entry: {
                    name: loadedChats[h].name,
                    preview: loadedChats[h].preview
                }
            })
        }

        root.selectedChat = loadedChats.length > 0 ? 0 : -1
        root.reloadMessageView()
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
            root.historyBusy = true
            root.loggedIn = true
            root.conversations = []
            conversationViewModel.clear()
            messageViewModel.clear()
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
            root.historyBusy = false
        }

        function onEventsLoadFailed(message) {
            root.historyBusy = false
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
            conversationViewModel.append({
                entry: {
                    name: root.pendingRecipient,
                    preview: "No messages yet"
                }
            })
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
            if (!root.sendBusy || !root.pendingMessageRecipient.length)
                return

            const recipient = root.pendingMessageRecipient
            const key = recipient.toLowerCase()
            const message = root.makeMessage(
                root.pendingBody, root.accountName, root.pendingSentAt
            )

            // Resolve the recipient now; history may have reordered the list.
            const updated = root.conversations.slice()
            let chatIndex = -1
            for (let i = 0; i < updated.length; ++i) {
                if (updated[i].name.toLowerCase() === key) {
                    chatIndex = i
                    break
                }
            }

            const newChat = chatIndex < 0
            const chat = newChat
                         ? { name: recipient, preview: "", messages: [] }
                         : Object.assign({}, updated[chatIndex])
            chat.messages = chat.messages.concat([message])
            chat.preview = message.body

            if (newChat) {
                chatIndex = updated.length
                updated.push(chat)
            } else {
                updated[chatIndex] = chat
            }
            root.conversations = updated

            const entry = { name: chat.name, preview: chat.preview }
            if (newChat)
                conversationViewModel.append({ entry: entry })
            else
                conversationViewModel.setProperty(chatIndex, "entry", entry)

            if (newChat && root.selectedChat < 0) {
                // Changing selection loads the message once.
                root.selectedChat = chatIndex
            } else if (root.selectedChat === chatIndex) {
                messageViewModel.append({ entry: message })
                messageList.forceLayout()
                messageList.positionViewAtEnd()
            }

            root.sendBusy = false
            root.pendingMessageRecipient = ""
            root.pendingBody = ""
            root.pendingSentAt = 0
            composer.clear()
        }

        function onMessageFailed(message) {
            root.sendBusy = false
            root.pendingMessageRecipient = ""
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
            color: control.primary ? (root.lightChat ? "#FFFFFF" : "#07131D") : root.foreground
            font.pixelSize: 13
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            opacity: control.enabled ? 1 : 0.45
        }

        background: Rectangle {
            radius: 6
            opacity: control.enabled ? 1 : 0.45

            color: control.primary
                   ? (control.down ? root.accentPressed
                      : control.hovered ? root.accentHover
                      : root.accent)
                   : (control.down ? root.pressedSurface
                      : control.hovered ? root.hoverSurface
                      : root.surface)

            border.color: control.activeFocus
                          ? root.foreground
                          : control.primary ? root.accent : root.divider
            border.width: control.activeFocus ? 2 : 1
        }
    }

    component AppField: TextField {
        id: control

        implicitHeight: 44
        leftPadding: 13
        rightPadding: 13
        color: root.foreground
        placeholderTextColor: root.muted
        selectionColor: root.selection
        selectedTextColor: root.selectedText
        font.pixelSize: 14

        background: Rectangle {
            radius: 6
            color: root.chatBackground
            border.color: control.activeFocus ? root.accent : root.divider
        }
    }

    component Avatar: Rectangle {
        property string name: ""
        implicitWidth: 40
        implicitHeight: 40
        radius: 7
        color: root.avatarSurface

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
                    color: root.foreground
                    font.family: "Segoe UI Symbol"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: minimizeWindowButton.down
                           ? root.pressedSurface
                           : minimizeWindowButton.hovered
                             ? root.hoverSurface
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
                    color: root.foreground
                    font.family: "Segoe UI Symbol"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: expandWindowButton.down
                           ? root.pressedSurface
                           : expandWindowButton.hovered
                             ? root.hoverSurface
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
                    color: closeWindowButton.down || closeWindowButton.hovered ? root.selectedText : root.foreground
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
        color: root.chatBackground

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
            visible: !root.lightChat

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
                root.height,
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
                    color: root.surface
                    border.color: root.divider
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
                            color: root.accent
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
                                color: root.divider
                            }

                            Text {
                                text: "SIGN IN"
                                color: root.muted
                                font.family: "Consolas"
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                font.letterSpacing: 2
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: root.divider
                            }
                        }

                        Text {
                            text: "Username"
                            color: root.foreground
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
                            color: root.foreground
                            selectionColor: root.selection
                            selectedTextColor: root.selectedText
                            opacity: enabled ? 1 : 0.65
                            Accessible.name: "Username"

                            background: Rectangle {
                                radius: 6
                                color: root.chatBackground
                                border.color:
                                    usernameField.activeFocus
                                    ? root.accent : root.divider
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
                            color: root.foreground
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
                            color: root.foreground
                            selectionColor: root.selection
                            selectedTextColor: root.selectedText
                            opacity: enabled ? 1 : 0.65
                            Accessible.name: "Password"

                            background: Rectangle {
                                radius: 6
                                color: root.chatBackground
                                border.color:
                                    passwordField.activeFocus
                                    ? root.accent : root.divider
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
                            color: root.foreground
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
                                color: root.lightChat ? "#FFFFFF" : "#07131D"
                                font.family: "Consolas"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 6
                                color: loginButton.down ? root.accentPressed
                                     : loginButton.hovered ? root.accentHover
                                     : root.accent
                                border.color:
                                    loginButton.activeFocus
                                    ? root.foreground : "transparent"
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
                                color: root.foreground
                                font.family: "Consolas"
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 6
                                color: registerButton.down ? root.pressedSurface
                                     : registerButton.hovered ? root.hoverSurface
                                     : "transparent"
                                border.color:
                                    registerButton.activeFocus
                                    ? root.accent : root.divider
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
        color: root.chatBackground

        SplitView {
            anchors.fill: parent
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 6

                color: SplitHandle.pressed
                       ? root.accent
                       : SplitHandle.hovered
                         ? root.hoverSurface
                         : root.surface

                Behavior on color {
                    ColorAnimation {
                        duration: 100
                    }
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    anchors.centerIn: parent
                    color: root.divider
                }
            }

            // Unified sidebar.
            Rectangle {
                id: sidebar
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: 235
                SplitView.maximumWidth: 440
                color: root.panel

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Q3 brand header.
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 82
                        color: root.headerColor

                        gradient: Gradient {
                            orientation: Gradient.Horizontal

                            GradientStop {
                                position: 0.0
                                color: root.lightChat ? root.headerColor : "#122230"
                            }

                            GradientStop {
                                position: 0.55
                                color: root.lightChat ? root.headerColor : "#0D171F"
                            }

                            GradientStop {
                                position: 1.0
                                color: root.lightChat ? root.headerColor : "#101922"
                            }
                        }

                        // Static constellation background.
                        // It only redraws if the header is resized.
                        Canvas {
                            id: headerStars
                            anchors.fill: parent
                            visible: true

                            onVisibleChanged: {
                                if (visible)
                                    requestPaint()
                            }
                            opacity: root.lightChat ? 0.75 : 0.55

                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()
                            Component.onCompleted: requestPaint()

                            Connections {
                                target: root

                                function onLightChatChanged() {
                                    headerStars.requestPaint()
                                }
                            }
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
                                ctx.strokeStyle = root.lightChat ? "#475569" : "#23506A"

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

                                ctx.fillStyle = root.lightChat ? "#334155" : "#3A7597"

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

                        Button {
                            id: homeButton
                            width: 46
                            height: 46
                            anchors.left: parent.left
                            anchors.leftMargin: 22
                            anchors.verticalCenter: parent.verticalCenter
                            enabled: !root.sendBusy && !root.chatBusy && !root.historyBusy
                            hoverEnabled: true
                            Accessible.name: "Home"

                            ToolTip.visible: hovered
                            ToolTip.text: "Home"
                            ToolTip.delay: 500

                            background: Rectangle {
                                radius: 8
                                color: homeButton.down
                                       ? root.accentPressed
                                       : homeButton.hovered
                                         ? root.accentHover
                                         : root.accent
                                border.width: homeButton.visualFocus ? 2 : 0
                                border.color: root.foreground
                            }

                            contentItem: Text {
                                text: "Q3"
                                color: root.lightChat ? "#FFFFFF" : "#07131D"
                                font.family: "ROG FONTS"
                                font.pixelSize: 17
                                font.weight: Font.Bold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                root.selectedChat = -1
                                root.messageStatus = ""
                                composer.clear()
                            }
                        }

                        Rectangle {
                            height: 1
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            color: root.divider
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
                            color: root.foreground
                            font.family: "Consolas"
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }

                        Button {
                            id: addConversationButton
                            Layout.preferredWidth: 34
                            Layout.preferredHeight: 34
                            enabled: !root.historyBusy && !root.chatBusy && !root.sendBusy

                            contentItem: Text {
                                text: "+"
                                color: addConversationButton.down
                                       ? root.foreground
                                       : root.accent
                                font.family: "Consolas"
                                font.pixelSize: 22
                                font.weight: Font.Normal
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: width / 2
                                color: addConversationButton.down
                                       ? root.pressedSurface
                                       : addConversationButton.hovered
                                         ? root.hoverSurface
                                         : root.surface

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
                        placeholderTextColor: root.muted
                        color: root.foreground
                        selectionColor: root.selection
                        selectedTextColor: root.selectedText
                        font.family: "Consolas"
                        font.pixelSize: 13

                        leftPadding: 13
                        rightPadding: 13
                        topPadding: 0
                        bottomPadding: 0
                        verticalAlignment: TextInput.AlignVCenter

                        background: Rectangle {
                            radius: 6
                            color: root.chatInputBackground
                            border.color: search.activeFocus
                                          ? root.accent
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
                        model: conversationViewModel

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
                                    color: root.avatarSurface

                                    Text {
                                        anchors.centerIn: parent
                                        text: "•••"
                                        color: root.accent
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
                                    color: root.avatarSurface
                                }
                            }

                            Text {
                                width: 220
                                text: root.historyBusy
                                      ? "Loading conversations..."
                                      : search.text.length > 0
                                        ? "No matching conversations"
                                        : "No conversations yet"
                                color: root.muted
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
                            required property var entry
                            readonly property var modelData: entry

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
                                    ? root.selectedSurface
                                    : conversationRow.hovered
                                      ? root.hoverSurface
                                      : "transparent"

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.verticalCenter:
                                        parent.verticalCenter
                                    width: 3
                                    height: 24
                                    radius: 2
                                    color: root.accent

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
                                    color: root.avatarSurface

                                    Text {
                                        anchors.centerIn: parent
                                        text:
                                            conversationRow.modelData.name
                                                .substring(0, 2)
                                                .toUpperCase()
                                        color: root.accent
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
                                        color: root.foreground
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
                                        color: root.muted
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
                        color: root.divider
                    }

                    // Current account.
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        Layout.topMargin: 24
                        Layout.bottomMargin: 15
                        spacing: 11

                        Rectangle {
                            Layout.preferredWidth: 38
                            Layout.preferredHeight: 38
                            radius: 19
                            color: root.avatarSurface

                            Text {
                                anchors.centerIn: parent
                                text: root.accountName
                                      .substring(0, 2)
                                      .toUpperCase()
                                color: root.accent
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
                                color: root.foreground
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
                                    color: root.success
                                }

                                Text {
                                    text: "CONNECTED"
                                    color: root.success
                                    font.family: "Consolas"
                                    font.pixelSize: 9
                                    font.letterSpacing: 0.8
                                }
                            }
                        }
                        Button {
                            id: settingsButton
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            hoverEnabled: true

                            contentItem: Text {
                                text: "\u2699"
                                color: root.foreground
                                font.family: "Segoe UI Symbol"
                                font.pixelSize: 22
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 6
                                color: settingsButton.down
                                       ? root.pressedSurface
                                       : settingsButton.hovered
                                         ? root.hoverSurface
                                         : "transparent"
                                border.color: settingsButton.activeFocus
                                              ? root.accent
                                              : "transparent"
                            }

                            Accessible.name: "Open settings"
                            ToolTip.visible: hovered
                            ToolTip.text: "Settings"
                            ToolTip.delay: 500

                            onClicked: settingsDialog.open()
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
                    color: root.headerColor

                    gradient: Gradient {
                        orientation: Gradient.Horizontal

                        GradientStop {
                            position: 0.0
                            color: root.lightChat ? root.headerColor : "#171A1F"
                        }

                        GradientStop {
                            position: 0.55
                            color: root.lightChat ? root.headerColor : "#101217"
                        }

                        GradientStop {
                            position: 1.0
                            color: root.lightChat ? root.headerColor : "#090B0F"
                        }
                    }

                    Canvas {
                        anchors.fill: parent
                        visible: !root.lightChat

                        onVisibleChanged: {
                            if (visible)
                                requestPaint()
                        }
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
                            color: root.muted
                            font.family: "Consolas"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            Layout.alignment: Qt.AlignBaseline
                            text: "/"
                            color: root.muted
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
                            color: root.foreground
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
                        color: root.divider
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        anchors.fill: parent
                        color: root.chatBackground
                    }

                    // Static deep-space conversation background.
                    Canvas {
                        id: conversationSpace
                        visible: !root.lightChat
                        anchors.fill: parent
                        onVisibleChanged: {
                            if (visible)
                                requestPaint()
                        }

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
                            color: root.muted
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
                        model: messageViewModel

                        ScrollBar.vertical: ScrollBar {}

                        Text {
                            anchors.centerIn: parent
                            visible: messageList.count === 0
                            text: "No messages yet"
                            color: root.chatMuted
                            font.family: "Consolas"
                            font.pixelSize: 11
                        }

                        delegate: Column {
                            id: messageRow

                            required property int index
                            required property var entry
                            readonly property var modelData: entry

                            readonly property bool startsDay:
                                index === 0
                                || (index > 0 && index < messageViewModel.count
                                    && modelData.dayKey !==
                                       messageViewModel.get(index - 1).entry.dayKey)

                            width: ListView.view.width
                            spacing: 12

                            RowLayout {
                                width: parent.width
                                visible: messageRow.startsDay
                                spacing: 12

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: root.divider
                                }

                                Text {
                                    text: messageRow.modelData.dayLabel
                                    color: root.muted
                                    font.family: "Consolas"
                                    font.pixelSize: 12
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: root.divider
                                }
                            }

                            RowLayout {
                                width: parent.width
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 34
                                    Layout.preferredHeight: 34
                                    Layout.alignment: Qt.AlignTop
                                    radius: 17
                                    color: root.avatarSurface

                                    Text {
                                        anchors.centerIn: parent
                                        text: String(
                                            messageRow.modelData.sender
                                            || root.accountName
                                        ).substring(0, 2).toUpperCase()
                                        color: root.accent
                                        font.family: "Consolas"
                                        font.pixelSize: 10
                                        font.weight: Font.Bold
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 9

                                        Text {
                                            Layout.maximumWidth: Math.max(
                                                0,
                                                messageRow.width - 46
                                                - messageTimestamp.implicitWidth - 18
                                            )
                                            text: messageRow.modelData.sender
                                                  || root.accountName
                                            textFormat: Text.PlainText
                                            color: root.chatSender
                                            font.family: "Consolas"
                                            font.pixelSize: 14
                                            font.weight: Font.Bold
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            id: messageTimestamp
                                            Layout.minimumWidth: implicitWidth
                                            text: messageRow.modelData.time
                                            color: root.chatMuted
                                            font.family: "Consolas"
                                            font.pixelSize: 11
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: messageRow.modelData.body
                                        textFormat: Text.PlainText
                                        color: root.chatText
                                        font.family: "Consolas"
                                        font.pixelSize: root.chatFontSize
                                        wrapMode: Text.Wrap
                                        lineHeight: 1.3
                                    }
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
                    color: root.warning
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
                    color: root.chatInputBackground

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 9

                        TextField {
                            id: composer
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            readOnly: root.sendBusy || root.historyBusy
                            placeholderText:
                                root.currentChat
                                ? "Message @" + root.currentChat.name
                                : ""
                            placeholderTextColor: root.chatMuted
                            color: root.chatText
                            selectionColor: root.selection
                            selectedTextColor: root.selectedText
                            font.family: "Consolas"
                            font.pixelSize: root.chatFontSize

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
                                !root.sendBusy && !root.historyBusy && !root.chatBusy
                                && composer.text.trim().length > 0

                            hoverEnabled: enabled
                            opacity: 1

                            contentItem: Text {
                                text: root.sendBusy ? "..." : "SEND"

                                color: sendButton.enabled
                                       ? (root.lightChat ? "#FFFFFF" : "#07131D")
                                       : root.muted

                                font.family: "Consolas"
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 6

                                color: !sendButton.enabled
                                       ? root.surface
                                       : sendButton.down
                                         ? root.accentPressed
                                         : sendButton.hovered
                                           ? root.accentHover
                                           : root.accent

                                border.width:
                                    sendButton.enabled ? 0 : 1

                                border.color: root.divider

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

    Popup {
        id: settingsDialog
        parent: Overlay.overlay
        anchors.centerIn: parent

        width: Math.min(420, parent.width - 40)
        padding: 24
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Overlay.modal: Rectangle {
            color: root.scrim
        }

        background: Rectangle {
            color: root.surface
            radius: 10
            border.color: root.divider
        }

        contentItem: ColumnLayout {
            spacing: 14

            Text {
                text: "Settings"
                color: root.foreground
                font.family: "Consolas"
                font.pixelSize: 22
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: "Customize your appearance."
                color: root.muted
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }

            Text {
                text: "Appearance"
                color: root.foreground
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true

                AppButton {
                    Layout.fillWidth: true
                    text: "Dark"
                    primary: !root.lightChat
                    onClicked: root.lightChat = false
                }

                AppButton {
                    Layout.fillWidth: true
                    text: "Light"
                    primary: root.lightChat
                    onClicked: root.lightChat = true
                }
            }

            Text {
                text: "Message font size"
                color: root.foreground
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true

                AppButton {
                    Layout.fillWidth: true
                    text: "Small"
                    primary: root.chatFontSize === 13
                    onClicked: root.chatFontSize = 13
                }

                AppButton {
                    Layout.fillWidth: true
                    text: "Default"
                    primary: root.chatFontSize === 16
                    onClicked: root.chatFontSize = 16
                }

                AppButton {
                    Layout.fillWidth: true
                    text: "Large"
                    primary: root.chatFontSize === 20
                    onClicked: root.chatFontSize = 20
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: previewText.implicitHeight + 32
                radius: 6
                color: root.chatBackground

                Text {
                    id: previewText
                    anchors.fill: parent
                    anchors.margins: 16
                    text: "This is how your messages will look."
                    color: root.chatText
                    font.family: "Consolas"
                    font.pixelSize: root.chatFontSize
                    wrapMode: Text.Wrap
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Preferences reset when you close the app."
                color: root.muted
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true

                AppButton {
                    text: "Reset"
                    onClicked: {
                        root.lightChat = false
                        root.chatFontSize = 16
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                AppButton {
                    text: "Done"
                    primary: true
                    onClicked: settingsDialog.close()
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
            color: root.scrim
        }

        background: Rectangle {
            color: root.surface
            radius: 10
        }

        contentItem: ColumnLayout {
            spacing: 0

            Text {
                text: "New conversation"
                color: root.foreground
                font.family: "Consolas"
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Text {
                Layout.topMargin: 8
                Layout.bottomMargin: 22
                text: "Enter a username"
                color: root.muted
                font.family: "Consolas"
                font.pixelSize: 11
            }

            TextField {
                id: recipientField
                Layout.fillWidth: true
                Layout.preferredHeight: 46

                enabled: !root.chatBusy
                placeholderText: "Username"
                placeholderTextColor: root.muted
                color: root.foreground
                selectionColor: root.selection
                selectedTextColor: root.selectedText
                font.family: "Consolas"
                font.pixelSize: 13

                leftPadding: 13
                rightPadding: 13
                topPadding: 0
                bottomPadding: 0
                verticalAlignment: TextInput.AlignVCenter

                background: Rectangle {
                    radius: 6
                    color: root.chatInputBackground
                    border.color:
                        recipientField.activeFocus
                        ? root.accent : "transparent"
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
                color: root.warning
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
                        color: root.foreground
                        font.family: "Consolas"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 6
                        color: cancelChatButton.down
                               ? root.pressedSurface
                               : cancelChatButton.hovered
                                 ? root.hoverSurface
                                 : root.surface
                    }

                    onClicked: newChatDialog.close()
                }

                Button {
                    id: startButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42

                    enabled: !root.chatBusy && !root.sendBusy && !root.historyBusy
                    hoverEnabled: true
                    opacity: enabled ? 1 : 0.45

                    contentItem: Text {
                        text: root.chatBusy
                              ? "Starting..."
                              : "Start chat"
                        color: root.lightChat ? "#FFFFFF" : "#07131D"
                        font.family: "Consolas"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 6
                        color: startButton.down
                               ? root.accentPressed
                               : startButton.hovered
                                 ? root.accentHover
                                 : root.accent
                    }

                    onClicked: newChatDialog.submit()
                }
            }
        }
    }
}
