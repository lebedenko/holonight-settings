pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Shapes
import Holonight.Controls
import Holonight.Core
import HolonightSettings

T.CheckDelegate {
    id: root

    required property string familyId
    required property string title
    required property string schemeId

    readonly property var tokens: ThemeSwatchTokens.getTokensForScheme(root.schemeId)
    readonly property color surfaceColor: root.tokens.surface
    readonly property color surfaceHoverColor: root.tokens.surfaceHover
    readonly property color surfacePressedColor: root.tokens.surfacePressed
    readonly property color borderPassiveColor: root.tokens.borderPassive
    readonly property color primaryColor: root.tokens.primary
    readonly property color onPrimaryColor: root.tokens.onPrimary
    readonly property color selectionIndicatorColor: root.tokens.selectionIndicator
    readonly property color disabledOverlayColor: root.tokens.disabledOverlay
    readonly property color accentColor: root.tokens.accent
    readonly property color secondaryAccentColor: root.tokens.secondaryAccent
    readonly property int cardPadding: 4

    objectName: "themeFamilyCard"
    implicitWidth: 102.4
    implicitHeight: 76.8
    padding: 0
    hoverEnabled: true
    Accessible.name: root.title
    Accessible.role: Accessible.CheckBox

    contentItem: Item {}

    background: Item {
        anchors.fill: parent

        HnSurfaceFrame {
            id: outerCard

            anchors.fill: parent
            surfaceRole: HnSurfaceRole.Card
            fillColor: root.down ? root.surfacePressedColor
                                 : (root.hovered ? root.surfaceHoverColor : root.surfaceColor)
            borderWidth: HnMetrics.borderWidth
            borderColor: root.checked ? root.selectionIndicatorColor : root.borderPassiveColor

            Rectangle {
                width: 44
                height: 18
                radius: 8
                antialiasing: true
                anchors {
                    top: parent.top
                    right: parent.right
                    topMargin: outerCard.normalizedBorderWidth + root.cardPadding
                    rightMargin: outerCard.normalizedBorderWidth + root.cardPadding
                }

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: root.surfaceColor }
                    GradientStop { position: 1.0; color: root.accentColor }
                }
            }

            Rectangle {
                width: 52
                height: 18
                radius: 8
                antialiasing: true
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    bottomMargin: outerCard.normalizedBorderWidth + root.cardPadding
                    leftMargin: outerCard.normalizedBorderWidth + root.cardPadding
                }

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: root.secondaryAccentColor }
                    GradientStop { position: 1.0; color: root.surfaceColor }
                }
            }
        }

        Item {
            visible: root.checked
            width: 22
            height: 22
            anchors {
                right: outerCard.right
                top: outerCard.top
                rightMargin: root.cardPadding
                topMargin: root.cardPadding
            }

            HnSurfaceFrame {
                anchors.fill: parent
                surfaceRole: HnSurfaceRole.Pill
                fillColor: root.primaryColor
                borderWidth: HnMetrics.borderWidth
                borderColor: root.selectionIndicatorColor
            }

            Shape {
                anchors.fill: parent
                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    strokeColor: root.onPrimaryColor
                    strokeWidth: HnMetrics.borderWidth * 2
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    startX: 7
                    startY: 13

                    PathLine { x: 11; y: 17 }
                    PathLine { x: 19; y: 8 }
                }
            }
        }

        HnSurfaceFrame {
            anchors.fill: parent
            visible: root.visualFocus
            surfaceRole: HnSurfaceRole.Card
            fillColor: "transparent"
            borderWidth: HnMetrics.focusBorderWidth
            borderColor: HoloniightPalette.borderFocus
        }

        HnSurfaceFrame {
            anchors.fill: parent
            visible: !root.enabled
            surfaceRole: HnSurfaceRole.Card
            fillColor: root.disabledOverlayColor
            borderWidth: 0
        }
    }
}
