// swift-tools-version: 6.2
import PackageDescription

let package = Package(
  name: "IMKSwift",
  products: [
    .library(
      name: "IMKSwift",
      targets: ["IMKSwift"]
    ),
  ],
  targets: [
    .target(
      name: "IMKSwift",
      dependencies: [
        "IMKSwiftModernHeaders",
      ],
      resources: [],
      swiftSettings: [
        .defaultIsolation(MainActor.self),
      ]
    ),
    .target(
      name: "IMKSwiftModernHeaders",
      resources: []
    ),
  ]
)
