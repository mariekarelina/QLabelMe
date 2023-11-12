import qbs
import "qlabelme_base.qbs" as QLabelMeBase

QLabelMeBase {
    name: "QLabelMe (Project)"

    references: [
        "src/qlabelme/qlabelme.qbs",
        "src/pproto/pproto.qbs",
        "src/rapidjson/rapidjson.qbs",
        "src/shared/shared.qbs",
        "src/yaml/yaml.qbs",
        //"setup/package_build.qbs",
    ]
}
