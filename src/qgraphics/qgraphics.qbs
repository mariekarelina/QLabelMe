import qbs
import QbsUtl
import ProbExt

Product {
    name: "QGraphics"
    targetName: "qgraphics"

    type: "staticlibrary"

    Depends { name: "cpp" }
    Depends { name: "SharedLib" }
    Depends { name: "Qt"; submodules: ["core", "gui", "widgets"] }

    cpp.defines: project.cppDefines
    cpp.cxxFlags: project.cxxFlags
    cpp.cxxLanguageVersion: project.cxxLanguageVersion

    property var includePaths: [
        "./",
        "./qgraphics2",
    ]
    cpp.includePaths: includePaths

    cpp.systemIncludePaths: QbsUtl.concatPaths(
        Qt.core.cpp.includePaths // Декларация для подавления Qt warning-ов
    )

    files: [
        "qgraphics2/circle.cpp",
        "qgraphics2/circle.h",
        "qgraphics2/drag_circle.cpp",
        "qgraphics2/drag_circle.h",
        "qgraphics2/polyline.cpp",
        "qgraphics2/polyline.h",
        "qgraphics2/rectangle.cpp",
        "qgraphics2/rectangle.h",
        "qgraphics2/shape.h",
        "qgraphics2/square.cpp",
        "qgraphics2/square.h",
        "qgraphics2/user_type.h",
        "qgraphics2/video_rect.cpp",
        "qgraphics2/video_rect.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.includePaths
    }
}
