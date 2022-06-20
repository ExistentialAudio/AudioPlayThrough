// swift-tools-version: 5.6
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "AudioPlayThrough",
    products: [
        .library(
            name: "AudioPlayThroughCPP",
            targets: ["AudioPlayThroughCPP"]),
        .library(
            name: "AudioPlayThroughObjC",
            targets: ["AudioPlayThroughObjC"]),
    ],
    dependencies: [
    ],
    targets: [
        .target(
            name: "AudioPlayThroughCPP",
            dependencies: []),
        .target(
            name: "AudioPlayThroughObjC",
            dependencies: ["AudioPlayThroughCPP"]),
        .testTarget(
            name: "AudioPlayThroughTests",
            dependencies: ["AudioPlayThroughObjC"]),
    ],
    cLanguageStandard: .c11,
    cxxLanguageStandard: .cxx14
)

// Routing
// Default Mic -> Record Driver (hidden)
// Default Output -> Driver -> Output and Record Driver
// Record Driver -> Recorded
