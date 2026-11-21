# Gradle

- **Data source:** the resolved configuration graph.
- **Flag:** a built-in task `generateSbom`.
- **Format:** protobuf (protobom). Java has protobuf support — use protobom's
  Java bindings if published, else generate from the `.proto`.
- **Gaps:** supplier is not a first-class Gradle concept; license comes from POM
  metadata and is often missing or wrong; hashes require artifact resolution.
