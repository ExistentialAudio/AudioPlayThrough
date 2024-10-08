// swift-tools-version: 5.6
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "AudioPlayThrough",
    platforms: [
        .macOS(.v10_15),
    ],
    products: [
        .library(
            name: "AudioPlayThrough",
            targets: ["AudioPlayThrough"]),
    ],
    dependencies: [
    ],
    targets: [
        .target(
            name: "AudioPlayThrough"),
    ],
    cLanguageStandard: .c11,
    cxxLanguageStandard: .cxx14
)
