# Maven

- **Data source:** the resolved dependency graph (`Project.getArtifacts()` /
  the reactor model).
- **Flag:** a core lifecycle binding or a first-party plugin.
- **Format:** protobuf (protobom). Java has protobuf support — use protobom's
  Java bindings if published, else generate from the `.proto`.
- **Gaps:** supplier is not modelled; license is POM-declared and frequently
  absent; purl is derivable from groupId/artifactId/version.
