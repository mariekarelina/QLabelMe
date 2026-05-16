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
        "./undo"
    ]

    Group {
        name: "resources"
        files: [
            "qlabelme.qrc",
            "qlabelme.rc",
        ]
    }

    Group {
        name: "widgets"
        prefix: "widgets/"
        files: [
            "about_program.cpp",
            "about_program.h",
            "about_program.ui",
            "document.h",
            "graphics_view.cpp",
            "graphics_view.h",
            "main_window.cpp",
            "main_window.h",
            "main_window.ui",
            "message_box.cpp",
            "message_box.h",
            "project_settings.cpp",
            "project_settings.h",
            "project_settings.ui",
            "save_geometry.cpp",
            "save_geometry.h",
            "select_class.cpp",
            "select_class.h",
            "select_class.ui",
            "settings.cpp",
            "settings.h",
            "settings.ui",
            "unsaved_changes.cpp",
            "unsaved_changes.h",
            "unsaved_changes.ui",
            "user_guide.cpp",
            "user_guide.h",
            "user_guide.md",
            "user_guide.ui",
        ]
    }
    Group {
        name: "undo"
        prefix: "undo/"
        files: [
            "lambda_command.cpp",
            "lambda_command.h",
            "undo_stack.cpp",
            "undo_stack.h",
        ]
    }

    cpp.dynamicLibraries: [
        "pthread"
    ]

    files: [
        "qlabelme.cpp",
    ]
}
