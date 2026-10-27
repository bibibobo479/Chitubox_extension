// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SSI
import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    objectName: "ChituboxLineSupportSidebar"
    width: parent ? parent.width - 20 : 270
    height: column.height + 16
    color: "#282828"
    radius: 5
    property bool expanded: true
    property bool armed: false
    property real initialStep: 2
    property bool includeEnd: true
    property int startRequest: 0
    property int cancelRequest: 0
    property string resultText: "Выберите две точки на поверхности модели."
    property real stepValue: Number(stepField.text.replace(",", "."))

    Column {
        id: column
        x: 8; y: 8
        width: parent.width - 16
        spacing: 8
        Button {
            objectName: "LineSupportToggle"
            width: parent.width; height: 36
            text: (root.expanded ? "▾  " : "▸  ") + "Поддержки по линии" + (root.armed ? " •" : "")
            onClicked: root.expanded = !root.expanded
            contentItem: Text { text: parent.text; color: "#eeeeee"; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.hovered ? "#383838" : "#282828"; radius: 3 }
        }
        Column {
            width: parent.width
            visible: root.expanded
            height: visible ? implicitHeight : 0
            spacing: 10
            Row {
                width: parent.width; spacing: 10
                Text { width: parent.width - 100; height: 32; text: "Шаг, мм"; color: "#dddddd"; font.pixelSize: 13; verticalAlignment: Text.AlignVCenter }
                TextField {
                    id: stepField
                    objectName: "LineSupportStep"
                    width: 90; height: 32
                    text: root.initialStep.toFixed(2)
                    color: "#eeeeee"; font.pixelSize: 13
                    selectByMouse: true
                    validator: RegularExpressionValidator { regularExpression: /[0-9]{1,3}([.,][0-9]{0,2})?/ }
                    background: Rectangle { color: "#383838"; radius: 3; border.color: stepField.activeFocus ? "#008bcb" : "#444444" }
                }
            }
            CheckBox {
                objectName: "LineSupportEndpoint"
                width: parent.width
                checked: root.includeEnd
                onToggled: root.includeEnd = checked
                text: "Добавить конечную точку"
                contentItem: Text { text: parent.text; leftPadding: parent.indicator.width + parent.spacing; color: "#dddddd"; font.pixelSize: 12; wrapMode: Text.WordWrap; verticalAlignment: Text.AlignVCenter }
            }
            Text {
                width: parent.width
                text: "Выберите инструмент один раз, затем ставьте линии парами точек. Тип и размеры — настройки CHITUBOX."
                color: "#bbbbbb"; font.pixelSize: 12; wrapMode: Text.WordWrap
            }
            Button {
                objectName: "LineSupportStart"
                width: parent.width; height: 38
                enabled: isFinite(root.stepValue) && root.stepValue >= 0.2 && root.stepValue <= 100
                text: root.armed ? "✓ Линия · перейти к ручным" : "Инструмент «Линия»"
                onClicked: root.startRequest++
                contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: !parent.enabled ? "#555555" : parent.hovered ? "#1599de" : "#007fc4"; radius: 4 }
            }
            Text {
                objectName: "LineSupportStatus"
                width: parent.width
                text: root.resultText
                color: "#dddddd"; font.pixelSize: 12; wrapMode: Text.WordWrap
            }
        }
    }
}
