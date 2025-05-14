{
  "targets": [
    {
      "target_name": "gstzerocopy",
      "type": "loadable_module",
      "toolsets": ["host", "target"],
      "cmake-js": {
        "runtime": "node",
        "runtimeVersion": "18.0.0",
        "arch": "x64",
        "cmakeToolchain": "cmake/NodeJS.cmake"
      }
    }
  ]
}