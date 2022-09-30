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
            name: "AudioPlayThroughObjC",
            targets: ["AudioPlayThroughObjC"]),
    ],
    dependencies: [
        .package(url: "https://github.com/ExistentialAudio/AEC3.git", exact: "1.1.0")
    ],
    targets: [
        .target(
            name: "AudioPlayThroughCPP",
            dependencies: ["RequestMicrophoneAuthorization", "AEC3"]),
        .target(
            name: "AudioPlayThroughObjC",
            dependencies: ["AudioPlayThroughCPP"]),
        .target(
            name: "RequestMicrophoneAuthorization",
            dependencies: []),
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
