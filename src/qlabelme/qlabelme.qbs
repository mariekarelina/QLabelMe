import qbs
import QbsUtl

Product {
    name: "QLabelMe"
    targetName: "qlabelme"
    condition: true

    type: "application"
    destinationDirectory: "./bin"

    Depends { name: "cpp" }
    Depends { name: "PProto" }
    Depends { name: "QGraphics" }
    //Depends { name: "QUtils" }
    Depends { name: "RapidJson" }
    Depends { name: "SharedLib" }
    Depends { name: "Yaml" }
    Depends { name: "Qt"; submodules: ["core", "network", "widgets"] }

    cpp.defines: project.cppDefines
    cpp.cxxFlags: project.cxxFlags
    cpp.cxxLanguageVersion: project.cxxLanguageVersion

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.driverLinkerFlags: ["--machine-windows"]
    }

    cpp.includePaths: [
        "./",
        "./widgets",
    ]

    cpp.systemIncludePaths: QbsUtl.concatPaths(
        Qt.core.cpp.includePaths // Декларация для подавления Qt warning-ов
    )

    Group {
        name: "resources"
        files: [
            "qlabelme.qrc",
            //"qlabelme.rc",
        ]
    }

    Group {
        name: "widgets"
        prefix: "widgets/"
        files: [
            "main_window.cpp",
            "main_window.h",
            "main_window.ui",
        ]
    }

    cpp.dynamicLibraries: [
        "pthread"
    ]

    files: [
        "qlabelme.cpp",
    ]

//    property var test: {
//        console.info("=== Video.decklinkIncludePath ===");
//    }

}
