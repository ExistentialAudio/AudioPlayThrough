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
            name: "AudioPlayThroughCPP",
            targets: ["AudioPlayThroughCPP"]),
        .library(
            name: "AudioPlayThroughC",
            targets: ["AudioPlayThroughC"]),
        .library(
            name: "AudioPlayThroughObjC",
            targets: ["AudioPlayThroughObjC"]),
        .library(
            name: "RequestMicrophoneAuthorization",
            targets: ["RequestMicrophoneAuthorization"])
    ],
    dependencies: [
    ],
    targets: [
        .target(
            name: "AudioPlayThroughCPP",
            dependencies: ["RequestMicrophoneAuthorization"]),
        .target(
            name: "AudioPlayThroughC",
            dependencies: ["AudioPlayThroughCPP"]),
        .target(
            name: "AudioPlayThroughObjC",
            dependencies: ["AudioPlayThroughCPP"]),
        .target(
            name: "RequestMicrophoneAuthorization",
            dependencies: []),
    ],
    cLanguageStandard: .c11,
    cxxLanguageStandard: .cxx14
)

// Routing
// Default Mic -> Record Driver (hidden)
// Default Output -> Driver -> Output and Record Driver
// Record Driver -> Recorded
