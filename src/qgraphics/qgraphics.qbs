import qbs
import qbs.FileInfo

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

    cpp.includePaths: [".", "qgraphics2"]

    files: [
        "qgraphics2/circle.cpp",
        "qgraphics2/circle.h",
        "qgraphics2/drag_circle.cpp",
        "qgraphics2/drag_circle.h",
        "qgraphics2/line.cpp",
        "qgraphics2/line.h",
        "qgraphics2/point.cpp",
        "qgraphics2/point.h",
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
        cpp.includePaths: [
            FileInfo.joinPaths(exportingProduct.sourceDirectory, "."),
            FileInfo.joinPaths(exportingProduct.sourceDirectory, "qgraphics2")
        ]
    }
}
