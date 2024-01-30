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
        "./qgraphics",
    ]
    cpp.includePaths: includePaths

    cpp.systemIncludePaths: QbsUtl.concatPaths(
        Qt.core.cpp.includePaths // Декларация для подавления Qt warning-ов
    )

    files: [
        "qgraphics/circle.cpp",
        "qgraphics/circle.h",
        "qgraphics/drag_circle.cpp",
        "qgraphics/drag_circle.h",
        //"qgraphics/functions.cpp",
        //"qgraphics/functions.h",
        "qgraphics/rectangle.cpp",
        "qgraphics/rectangle.h",
        "qgraphics/shape.h",
        "qgraphics/square.cpp",
        "qgraphics/square.h",
        "qgraphics/user_type.h",
        "qgraphics/video_rect.cpp",
        "qgraphics/video_rect.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.includePaths
    }
}
