// swift-tools-version: 5.6
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "AudioPlayThrough",
    products: [
        .library(
            name: "AudioPlayThrough",
            targets: ["AudioPlayThrough"]),
        .library(
            name: "AudioPlayThroughObjC",
            targets: ["AudioPlayThroughObjC"]),
    ],
    dependencies: [
    ],
    targets: [
        .target(
            name: "AudioPlayThrough",
            dependencies: []),
        .target(
            name: "AudioPlayThroughObjC",
            dependencies: ["AudioPlayThrough"]),
    ],
    cLanguageStandard: .c11,
    cxxLanguageStandard: .cxx14
)
