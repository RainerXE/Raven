// Raven for Zig: .xmlzig files (an interface in Raven XML with its Zig code)
// compiled by ravenc from build.zig. Written by hand, not by bindgen.
//
//   // build.zig
//   const xmlzig = @import("path/to/bindings/zig/xmlzig.zig");
//
//   pub fn build(b: *std.Build) void {
//       const target = b.standardTargetOptions(.{});
//       const optimize = b.standardOptimizeOption(.{});
//       const raven = b.createModule(.{ .root_source_file = b.path("path/to/bindings/zig/raven.zig") });
//       const exe = b.addExecutable(.{
//           .name = "app",
//           .root_module = b.createModule(.{
//               .root_source_file = xmlzig.compile(b, "src/main.xmlzig"),   // the .xmlzig is the program
//               .target = target,
//               .optimize = optimize,
//           }),
//       });
//       exe.root_module.addImport("raven", raven);
//       exe.linkSystemLibrary("raven");
//       exe.linkLibC();
//       b.installArtifact(exe);
//   }
//
// The generated file imports the module "raven". ravenc is RAVENC, or found in
// the PATH. The scripts keep their lines: the errors point into the .xmlzig.

const std = @import("std");

/// The Zig file ravenc makes from an .xmlzig of the project (a path relative to build.zig).
pub fn compile(b: *std.Build, path: []const u8) std.Build.LazyPath {
    const ravenc = b.graph.env_map.get("RAVENC") orelse "ravenc";
    const run = b.addSystemCommand(&.{ravenc});
    run.addFileArg(b.path(path));
    run.addArg("-o");
    const output = run.addOutputFileArg(b.fmt("{s}.zig", .{std.fs.path.basename(path)}));
    run.addArgs(&.{ "--lang", "zig" });
    return output;
}
